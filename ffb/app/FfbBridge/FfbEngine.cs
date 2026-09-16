// ============================================================================
//  YurFFB — engine FFB: mengubah event FFB DirectInput (dari vJoy) menjadi
//  perintah torsi ke firmware, dan membalik telemetri sudut menjadi sumbu X
//  di vJoy supaya game "melihat" setir bergerak.
//
//  Loop engine 500 Hz di thread sendiri:
//    1. drain antrean event FFB (thread-safe dari callback vJoy)
//    2. hitung gaya total dari semua efek aktif
//    3. terapkan gain global/per-tipe, min-force, clamp
//    4. kirim torsi (mPct) ke firmware via serial
//    5. update sumbu X vJoy dari telemetri sudut terbaru
// ============================================================================

using System.Diagnostics;
using System.Runtime.InteropServices;

namespace FfbBridge;

public enum TestMode { None, ConstCw, ConstCcw, Sine, Spring }

public struct EngineSettings
{
    public double GlobalGain;      // 0..2
    public double MinForce;        // 0..0.3 (kompensasi deadband motor)
    public int MaxTorquePct;       // 10..100
    public int RotationDeg;        // 180..1440 (rentang sumbu game)
    public double ConstantGain;
    public double RampGain;
    public double PeriodicGain;
    public double SpringGain;
    public double DamperGain;
    public double FrictionGain;
    public int AutoCenterPct;      // 0..100, pegas center saat game idle
    public bool Enabled;           // gate master (mengirim ENABLE ke device)

    public static EngineSettings Default => new()
    {
        GlobalGain = 1.0, MinForce = 0.0, MaxTorquePct = 100, RotationDeg = 900,
        ConstantGain = 1.0, RampGain = 1.0, PeriodicGain = 1.0,
        SpringGain = 1.0, DamperGain = 1.0, FrictionGain = 1.0,
        AutoCenterPct = 12, Enabled = false,
    };
}

/// <summary>Snapshot telemetri utk UI (immutable, dibuat engine).</summary>
public sealed class EngineStats
{
    public double AngleDeg;
    public double VelDegS;
    public int TorqueOut;          // mPct terakhir
    public byte DeviceFlags;
    public bool DeviceConnected;
    public int PlayingCount;
    public string PlayingDesc = "";
    public double DeviceGainPct = 100;
    public double LoopMs;
    public int FfbQueueDepth;
}

public sealed class FfbEngine : IDisposable
{
    private const double TickMs = 2.0;          // 500 Hz
    private const double VelUnitScale = 10.0;   // 1000 deg/s  -> 10000 unit DI
    private const double AccUnitScale = 0.2;    // 50000 deg/s^2 -> 10000 unit DI

    private class Cond
    {
        public int Cp, PosK, NegK, Dead;
        public uint PosSat = 10000, NegSat = 10000;
    }

    private class Slot
    {
        public bool Downloaded;
        public FfbEffectType Type = FfbEffectType.EtNone;
        public ushort DurationMs; public bool Infinite = true;
        public byte Gain255 = 255;
        public bool Polar = true; public byte Direction; public byte DirY;
        public uint AtkLvl, FadLvl; public double AtkMs, FadMs;
        public int ConstMag;
        public int RampStart, RampEnd;
        public uint PerMag, PerPhase, PerPeriodMs = 100; public int PerOffset;
        public readonly Cond CondX = new(), CondY = new();
        public bool Playing; public double StartMs; public byte Loops = 0; // 0/255 = infinite
        public string Desc => $"{Type} {(Playing ? "PLAY" : "idle")}";
    }

    private readonly SerialLink _serial;
    private VJoyDevice? _vjoy;
    private readonly Slot[] _slots = new Slot[17];   // index 1..16
    private readonly Stopwatch _sw = Stopwatch.StartNew();
    private readonly object _setLock = new();

    private Thread? _thread;
    private volatile bool _running;
    private EngineSettings _settings = EngineSettings.Default;
    private bool _lastEnableSent;

    // telemetri terbaru (di-update thread RX serial)
    private readonly object _telLock = new();
    private DeviceState _tel;
    private bool _telValid;

    // derivasi kecepatan utk inersia
    private double _lastVelDegS;
    private double _accDegS2;

    private byte _deviceGain255 = 255;
    private bool _actuatorsOn = true;
    private bool _paused;

    public volatile TestMode Test = TestMode.None;
    public volatile EngineStats? LatestStats;

    public FfbEngine(SerialLink serial, VJoyDevice? vjoy)
    {
        _serial = serial;
        _vjoy = vjoy;
        for (int i = 0; i < _slots.Length; i++) _slots[i] = new Slot();
    }

    public void UpdateSettings(EngineSettings s)
    {
        lock (_setLock) _settings = s;
    }

    /// <summary>Dipasang/dicabut setelah vJoy diaktifkan (thread-safe).</summary>
    public void AttachVjoy(VJoyDevice? v) => _vjoy = v;

    /// <summary>Dipanggil dari event OnState milik SerialLink.</summary>
    public void FeedState(DeviceState st)
    {
        lock (_telLock) { _tel = st; _telValid = true; }
    }

    public void Start()
    {
        if (_thread is not null) return;
        _running = true;
        _thread = new Thread(EngineLoop) { IsBackground = true, Name = "FfbEngine", Priority = ThreadPriority.AboveNormal };
        _thread.Start();
    }

    public void StopAll()
    {
        foreach (var s in _slots) { s.Playing = false; s.Downloaded = false; }
        Test = TestMode.None;
    }

    public void Dispose()
    {
        _running = false;
        try { _thread?.Join(1000); } catch { }
        _thread = null;
        try { _serial.Torque(0); _serial.Enable(false); } catch { }
    }

    // ------------------------------------------------------------------ loop -
    private void EngineLoop()
    {
        // timer 1 ms supaya Thread.Sleep presisi (Windows default 15.6 ms)
        WinmmTime.BeginPeriod(1);
        double next = _sw.Elapsed.TotalMilliseconds;
        var statTimer = 0.0;

        while (_running)
        {
            double now = _sw.Elapsed.TotalMilliseconds;
            if (now < next)
            {
                Thread.Sleep(1);
                continue;
            }
            double dtMs = Math.Min(now - (next - TickMs), 50);   // guard lonjakan
            next += TickMs;

            EngineSettings st = CurrentSettings;
            DeviceState tel = LatestTelemetry(out bool telValid);

            // 1. drain event FFB
            DrainFfbEvents();

            // 2. hitung gaya
            double force = 0;
            int playing = 0;
            var desc = new List<string>();

            if (Test != TestMode.None)
            {
                force = ComputeTestForce(tel, st);
                playing = 1;
                desc.Add($"[TEST {Test}]");
            }
            else if (_actuatorsOn && !_paused)
            {
                double posU = PosUnits(tel, st);
                double velU = VelUnits(tel);
                double accU = AccUnits(tel, dtMs);

                foreach (var s in _slots)
                {
                    if (!s.Downloaded || !s.Playing) continue;
                    double f = ComputeSlot(s, now, posU, velU, accU, st);
                    if (s.Playing) { force += f; playing++; desc.Add(s.Desc); }
                }
                if (playing == 0 && st.AutoCenterPct > 0 && telValid)
                    force = IdleCenterForce(tel, st);
            }

            // 3. pipeline gain
            double devGain = _deviceGain255 / 255.0;
            force *= devGain * st.GlobalGain;
            if (st.MinForce > 0 && Math.Abs(force) > 1e-4 && Math.Abs(force) < st.MinForce)
                force = Math.Sign(force) * st.MinForce;
            force = Math.Clamp(force, -1.0, 1.0);

            // 4. kirim torsi
            bool enable = st.Enabled && _serial.IsOpen;
            if (enable != _lastEnableSent)
            {
                _serial.Enable(enable);
                _lastEnableSent = enable;
            }
            if (enable)
            {
                short mPct = (short)Math.Round(force * 1000.0 * st.MaxTorquePct / 100.0);
                _serial.Torque(mPct);
                _lastTorque = mPct;
            }
            else if (_lastTorque != 0)
            {
                _serial.Torque(0);
                _lastTorque = 0;
            }

            // 5. update sumbu X vJoy
            if (_vjoy is { Active: true } && telValid)
                _vjoy.SetAxisX(AngleToAxis(tel.AngleDeg, st));

            // 6. stats utk UI (10 Hz cukup)
            statTimer += TickMs;
            if (statTimer >= 100)
            {
                statTimer = 0;
                LatestStats = new EngineStats
                {
                    AngleDeg = telValid ? tel.AngleDeg : 0,
                    VelDegS = telValid ? tel.VelDegS : 0,
                    TorqueOut = _lastTorque,
                    DeviceFlags = telValid ? tel.Flags : (byte)0,
                    DeviceConnected = _serial.IsOpen && telValid,
                    PlayingCount = playing,
                    PlayingDesc = string.Join(", ", desc),
                    DeviceGainPct = devGain * st.GlobalGain * 100,
                    LoopMs = dtMs,
                    FfbQueueDepth = _vjoy?.FfbEvents.Count ?? 0,
                };
            }
        }
        WinmmTime.EndPeriod(1);
    }

    private short _lastTorque;
    private EngineSettings CurrentSettings { get { lock (_setLock) return _settings; } }
    private DeviceState LatestTelemetry(out bool valid)
    { lock (_telLock) { valid = _telValid; return _tel; } }

    // ------------------------------------------------------------- events ----
    private void DrainFfbEvents()
    {
        if (_vjoy is null) return;
        while (_vjoy.FfbEvents.TryDequeue(out FfbEvent? ev))
            ApplyEvent(ev, _sw.Elapsed.TotalMilliseconds);
    }

    private void ApplyEvent(FfbEvent ev, double nowMs)
    {
        byte idx = (byte)Math.Clamp(ev.Index, 1, 16);
        switch (ev)
        {
            case FfbSetEffect e:
                {
                    var s = _slots[idx];
                    bool wasPlaying = s.Playing;
                    s.Downloaded = true;
                    s.Type = e.Type;
                    s.Infinite = e.DurationMs == 0xFFFF || e.DurationMs == 0;
                    s.DurationMs = e.DurationMs;
                    s.Gain255 = e.Gain255 == 0 && e.Type != FfbEffectType.EtNone ? (byte)255 : e.Gain255;
                    s.Polar = e.Polar;
                    s.Direction = e.Direction;
                    s.DirY = e.DirY;
                    // game men-download ulang efek yang sama -> restart (perilaku umum
                    // DirectInput: download baru = efek baru meski index sama)
                    if (wasPlaying) { s.StartMs = nowMs; }
                    break;
                }
            case FfbSetConstant e:
                {
                    var s = _slots[idx];
                    s.Downloaded = true;
                    // update magnitude pada efek constant force (game racing
                    // mengirim ini setiap frame utk gaya utama FFB)
                    s.Type = FfbEffectType.EtConst;
                    s.Infinite = true;
                    s.ConstMag = Math.Clamp(e.Magnitude, -10000, 10000);
                    break;
                }
            case FfbSetRamp e:
                _slots[idx].Downloaded = true;
                _slots[idx].RampStart = e.Start;
                _slots[idx].RampEnd = e.End;
                break;
            case FfbSetPeriodic e:
                {
                    var s = _slots[idx];
                    s.Downloaded = true;
                    s.PerMag = e.Magnitude;
                    s.PerOffset = e.Offset;
                    s.PerPhase = e.Phase;
                    s.PerPeriodMs = Math.Max(e.PeriodMs, 1);
                    break;
                }
            case FfbSetEnvelope e:
                {
                    var s = _slots[idx];
                    s.AtkLvl = e.AttackLevel;
                    s.FadLvl = e.FadeLevel;
                    s.AtkMs = e.AttackTimeMs;
                    s.FadMs = e.FadeTimeMs;
                    break;
                }
            case FfbSetCondition e:
                {
                    var s = _slots[idx];
                    var c = e.IsY ? s.CondY : s.CondX;
                    c.Cp = e.CpOffset; c.PosK = e.PosCoeff; c.NegK = e.NegCoeff;
                    c.PosSat = e.PosSat; c.NegSat = e.NegSat; c.Dead = e.DeadBand;
                    break;
                }
            case FfbEffectOp e:
                {
                    var s = _slots[idx];
                    switch (e.Op)
                    {
                        case FfbOp.EffStart:
                        case FfbOp.EffSolo:
                            if (e.Op == FfbOp.EffSolo)
                                foreach (var o in _slots) o.Playing = false;
                            s.Playing = s.Downloaded;
                            s.StartMs = nowMs;
                            s.Loops = e.Loops;
                            break;
                        case FfbOp.EffStop:
                            s.Playing = false;
                            break;
                    }
                    break;
                }
            case FfbBlockFree e:
                {
                    var s = _slots[(byte)Math.Clamp(e.Index, 1, 16)];
                    s.Playing = false;
                    s.Downloaded = false;
                    break;
                }
            case FfbDeviceControl e:
                switch (e.Ctrl)
                {
                    case FfbCtrl.CtrlEnact: _actuatorsOn = true; break;
                    case FfbCtrl.CtrlDisact: _actuatorsOn = false; break;
                    case FfbCtrl.CtrlStopall: foreach (var s in _slots) s.Playing = false; break;
                    case FfbCtrl.CtrlDevrst:
                        foreach (var s in _slots) { s.Playing = false; s.Downloaded = false; }
                        _deviceGain255 = 255;
                        _actuatorsOn = true;
                        _paused = false;
                        break;
                    case FfbCtrl.CtrlDevpause: _paused = true; break;
                    case FfbCtrl.CtrlDevcont: _paused = false; break;
                }
                break;
            case FfbDeviceGain e:
                _deviceGain255 = e.Gain255 == 0 ? (byte)255 : e.Gain255;
                break;
        }
    }

    // -------------------------------------------------------------- fisika ---
    private static double PosUnits(DeviceState tel, EngineSettings st)
        => Math.Clamp(tel.AngleDeg / (st.RotationDeg / 2.0) * 10000.0, -10000, 10000);

    private static double VelUnits(DeviceState tel)
        => Math.Clamp(tel.VelDegS * VelUnitScale, -10000, 10000);

    private double AccUnits(DeviceState tel, double dtMs)
    {
        double dt = Math.Max(dtMs / 1000.0, 0.001);
        double a = (tel.VelDegS - _lastVelDegS) / dt;
        _lastVelDegS = tel.VelDegS;
        _accDegS2 += (a - _accDegS2) * 0.2;      // LPF ringan
        return Math.Clamp(_accDegS2 * AccUnitScale, -10000, 10000);
    }

    private static double DirSign(Slot s)
    {
        if (s.Polar)
        {
            double deg = s.Direction * 360.0 / 255.0;
            return (deg >= 90.0 && deg < 270.0) ? -1.0 : 1.0;
        }
        // kartesian: DirX two's complement byte
        return (sbyte)s.Direction >= 0 ? 1.0 : -1.0;
    }

    private static double CondForce(Cond c, double units, double gain)
    {
        double err = units - c.Cp;
        double dead = c.Dead;
        if (Math.Abs(err) <= dead) return 0;
        double k = err > 0 ? c.PosK : c.NegK;
        double mag = Math.Abs(err) - dead;
        double f = -(k / 10000.0) * mag * Math.Sign(err);
        double sat = (err > 0 ? c.PosSat : c.NegSat) / 10000.0;
        if (sat <= 0) sat = 1.0;
        return Math.Clamp(f, -sat, sat) * gain;
    }

    private static double Wave(FfbEffectType t, double phaseDeg)
    {
        double ph = phaseDeg % 360.0;
        if (ph < 0) ph += 360.0;
        return t switch
        {
            FfbEffectType.EtSine => Math.Sin(ph * Math.PI / 180.0),
            FfbEffectType.EtSqr => ph < 180.0 ? 1.0 : -1.0,
            FfbEffectType.EtTrngl => ph < 180.0 ? (ph / 90.0 - 1.0) : (3.0 - ph / 90.0),
            FfbEffectType.EtStup => ph / 180.0 - 1.0,
            FfbEffectType.EtStdn => 1.0 - ph / 180.0,
            _ => 0.0,
        };
    }

    private static double EnvelopeScale(Slot s, double tMs)
    {
        if (s.AtkMs > 0 && tMs < s.AtkMs)
            return MathUtils.Lerp(s.AtkLvl / 10000.0, 1.0, tMs / s.AtkMs);
        if (!s.Infinite && s.FadMs > 0 && tMs > s.DurationMs - s.FadMs)
            return MathUtils.Lerp(1.0, s.FadLvl / 10000.0,
                (tMs - (s.DurationMs - s.FadMs)) / s.FadMs);
        return 1.0;
    }

    private double ComputeSlot(Slot s, double nowMs, double posU, double velU, double accU,
                               EngineSettings st)
    {
        double t = nowMs - s.StartMs;

        // durasi & loop
        if (!s.Infinite)
        {
            int loops = s.Loops is 0 or 255 ? int.MaxValue : s.Loops;
            double total = (double)s.DurationMs * loops;
            if (t >= total) { s.Playing = false; return 0; }
        }

        double dir = DirSign(s);
        double raw = 0;

        switch (s.Type)
        {
            case FfbEffectType.EtConst:
                raw = s.ConstMag / 10000.0 * dir * st.ConstantGain;
                raw *= EnvelopeScale(s, t);
                break;
            case FfbEffectType.EtRamp:
                {
                    double prog = s.Infinite
                        ? Math.Clamp(t / 1000.0, 0, 1)
                        : Math.Clamp(t / Math.Max(s.DurationMs, 1), 0, 1);
                    raw = MathUtils.Lerp(s.RampStart / 10000.0, s.RampEnd / 10000.0, prog)
                          * dir * st.RampGain;
                    raw *= EnvelopeScale(s, t);
                    break;
                }
            case FfbEffectType.EtSine:
            case FfbEffectType.EtSqr:
            case FfbEffectType.EtTrngl:
            case FfbEffectType.EtStup:
            case FfbEffectType.EtStdn:
                {
                    double phase = s.PerPhase / 100.0 + (t / s.PerPeriodMs) * 360.0;
                    double wave = Wave(s.Type, phase);
                    raw = (s.PerOffset / 10000.0 + (s.PerMag / 10000.0) * wave) * dir
                          * st.PeriodicGain;
                    raw = Math.Clamp(raw, -1.25, 1.25);
                    raw *= EnvelopeScale(s, t);
                    break;
                }
            case FfbEffectType.EtSprng:
                raw = CondForce(s.CondX, posU, st.SpringGain);
                break;
            case FfbEffectType.EtDmpr:
                raw = CondForce(s.CondX, velU, st.DamperGain);
                break;
            case FfbEffectType.EtInrt:
                raw = CondForce(s.CondX, accU, st.DamperGain);
                break;
            case FfbEffectType.EtFrctn:
                {
                    if (Math.Abs(velU) <= s.CondX.Dead) { raw = 0; break; }
                    double k = velU > 0 ? s.CondX.PosK : s.CondX.NegK;
                    raw = -(k / 10000.0) * Math.Sign(velU) * st.FrictionGain;
                    double sat = (velU > 0 ? s.CondX.PosSat : s.CondX.NegSat) / 10000.0;
                    if (sat > 0) raw = Math.Clamp(raw, -sat, sat);
                    break;
                }
            default:
                s.Playing = false;
                break;
        }

        raw *= s.Gain255 / 255.0;
        return raw;
    }

    private static double IdleCenterForce(DeviceState tel, EngineSettings st)
    {
        double posNorm = Math.Clamp(tel.AngleDeg / (st.RotationDeg / 2.0), -1, 1);
        double k = st.AutoCenterPct / 100.0 * 0.5;
        double damp = st.AutoCenterPct / 100.0 * 0.08;
        return -posNorm * k - (tel.VelDegS / 1000.0) * damp;
    }

    private double ComputeTestForce(DeviceState tel, EngineSettings st)
    {
        double t = _sw.Elapsed.TotalMilliseconds;
        return Test switch
        {
            TestMode.ConstCw => 0.4,
            TestMode.ConstCcw => -0.4,
            TestMode.Sine => 0.6 * Math.Sin(t / 1000.0 * 2.0 * Math.PI * 0.7),
            TestMode.Spring => Math.Clamp(
                -(tel.AngleDeg / (st.RotationDeg / 2.0)) * 0.5
                - (tel.VelDegS / 1000.0) * 0.05, -1, 1),
            _ => 0,
        };
    }

    private static int AngleToAxis(double angleDeg, EngineSettings st)
    {
        double norm = Math.Clamp(angleDeg / (st.RotationDeg / 2.0), -1, 1);
        int v = VJoyNative.AxisCenter + (int)(norm * (VJoyNative.AxisMax - VJoyNative.AxisCenter));
        return Math.Clamp(v, VJoyNative.AxisMin, VJoyNative.AxisMax);
    }
}

internal static class MathUtils
{
    public static double Lerp(double a, double b, double t) => a + (b - a) * Math.Clamp(t, 0, 1);
}

/// <summary>timeBeginPeriod/timeEndPeriod (winmm) — timer 1 ms untuk loop engine.</summary>
internal static class WinmmTime
{
    [DllImport("winmm.dll")] private static extern uint timeBeginPeriod(uint ms);
    [DllImport("winmm.dll")] private static extern uint timeEndPeriod(uint ms);

    public static void BeginPeriod(uint ms) => timeBeginPeriod(ms);
    public static void EndPeriod(uint ms) => timeEndPeriod(ms);
}
