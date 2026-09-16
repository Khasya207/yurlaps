// ============================================================================
//  YurFFB — P/Invoke vJoyInterface.dll (vJoy >= 2.1.8, x64/x86 sesuai OS).
//  Deklarasi mengikuti inc/vjoyinterface.h dari repo vJoy (shauleiz/vJoy).
//  Semua fungsi ekspor __cdecl; callback FFB __stdcall (CALLBACK).
// ============================================================================

using System.Runtime.InteropServices;

namespace FfbBridge;

public enum VjdStat { Own = 0, Free = 1, Busy = 2, Miss = 3, Unkn = 4 }

// Tipe paket FFB (HID_ID_* dari public.h vJoy)
public enum FfbPType
{
    PtEffrep = 0x01,   // Set Effect Report
    PtEnvrep = 0x02,   // Set Envelope Report
    PtCondrep = 0x03,  // Set Condition Report
    PtPridrep = 0x04,  // Set Periodic Report
    PtConstrep = 0x05, // Set Constant Force Report
    PtRamprep = 0x06,  // Set Ramp Force Report
    PtCstmrep = 0x07,  // Custom Force Data Report
    PtSmplrep = 0x08,  // Download Force Sample
    PtEfoprep = 0x0A,  // Effect Operation Report
    PtBlkfrrep = 0x0B, // PID Block Free Report
    PtCtrlrep = 0x0C,  // PID Device Control
    PtGainrep = 0x0D,  // Device Gain Report
    PtSetcrep = 0x0E,  // Set Custom Force Report
    PtNewefrep = 0x11, // Create New Effect Report (0x01 + 0x10)
    PtBlkldrep = 0x12, // Block Load Report
    PtPoolrep = 0x13,  // PID Pool Report
}

// Tipe efek (FFBEType dari vjoyinterface.h)
public enum FfbEffectType
{
    EtNone = 0, EtConst = 1, EtRamp = 2, EtSqr = 3, EtSine = 4, EtTrngl = 5,
    EtStup = 6, EtStdn = 7, EtSprng = 8, EtDmpr = 9, EtInrt = 10,
    EtFrctn = 11, EtCstm = 12,
}

public enum FfbOp { EffStart = 1, EffSolo = 2, EffStop = 3 }

public enum FfbCtrl
{
    CtrlEnact = 1, CtrlDisact = 2, CtrlStopall = 3,
    CtrlDevrst = 4, CtrlDevpause = 5, CtrlDevcont = 6,
}

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate void FfbGenCB(IntPtr packet, IntPtr userData);

internal static class VJoyNative
{
    public const uint HidUsageX = 0x30;
    public const int AxisMin = 0x0001;
    public const int AxisMax = 0x8000;   // 32768
    public const int AxisCenter = 0x4000;

    const string Dll = "vJoyInterface.dll";
    const CallingConvention Cc = CallingConvention.Cdecl;

    // ---- umum ----
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool vJoyEnabled();
    [DllImport(Dll, CallingConvention = Cc)] public static extern short GetvJoyVersion();
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool DriverMatch(ref ushort dllVer, ref ushort drvVer);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool GetNumberExistingVJD(out int n);
    [DllImport(Dll, CallingConvention = Cc)] public static extern int GetVJDStatus(uint rId);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool AcquireVJD(uint rId);
    [DllImport(Dll, CallingConvention = Cc)] public static extern void RelinquishVJD(uint rId);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool ResetVJD(uint rId);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool SetAxis(int value, uint rId, uint axis);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool GetVJDAxisExist(uint rId, uint axis);
    [DllImport(Dll, CallingConvention = Cc)] public static extern bool IsDeviceFfb(uint rId);
    [DllImport(Dll, CallingConvention = Cc)] public static extern int GetOwnerPid(uint rId);

    // ---- FFB ----
    [DllImport(Dll, CallingConvention = Cc)] public static extern void FfbRegisterGenCB(FfbGenCB cb, IntPtr data);
    [DllImport(Dll, CallingConvention = Cc)] public static extern FfbEffectType FfbGetEffect();

    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Type(IntPtr packet, out FfbPType type);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_EBI(IntPtr packet, out int index);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_DevCtrl(IntPtr packet, out FfbCtrl ctrl);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_DevGain(IntPtr packet, out byte gain);

    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Report(IntPtr packet, ref FFB_EFF_REPORT r);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_EffOp(IntPtr packet, ref FFB_EFF_OP op);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Ramp(IntPtr packet, ref FFB_EFF_RAMP r);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Constant(IntPtr packet, ref FFB_EFF_CONSTANT c);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Period(IntPtr packet, ref FFB_EFF_PERIOD p);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Cond(IntPtr packet, ref FFB_EFF_COND c);
    [DllImport(Dll, CallingConvention = Cc)] public static extern uint Ffb_h_Eff_Envlp(IntPtr packet, ref FFB_EFF_ENVLP e);

    // ---- struct (layout = vjoyinterface.h, pack natural 4) ----
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_REPORT
    {
        public byte EffectBlockIndex;
        public int EffectType;      // FfbEffectType
        public ushort Duration;     // ms; 0xFFFF = infinite
        public ushort TrigerRpt;
        public ushort SamplePrd;
        public byte Gain;           // 0..255
        public byte TrigerBtn;
        public int Polar;           // BOOL: 1 = polar, 0 = kartesian
        public byte Direction;      // polar: 0..255 = 0..360 derajat (union dgn DirX)
        public byte DirY;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_OP
    {
        public byte EffectBlockIndex;
        public int EffectOp;        // FfbOp
        public byte LoopCount;      // 0 / 255 = infinite
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_CONSTANT
    {
        public byte EffectBlockIndex;
        public int Magnitude;       // -10000..10000
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_RAMP
    {
        public byte EffectBlockIndex;
        public int Start;           // -10000..10000
        public int End;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_PERIOD
    {
        public byte EffectBlockIndex;
        public uint Magnitude;      // 0..10000
        public int Offset;          // -10000..10000
        public uint Phase;          // 0..35999 (centidegrees)
        public uint Period;         // ms
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_COND
    {
        public byte EffectBlockIndex;
        public int IsY;             // BOOL: kondisi utk sumbu Y (kita hanya pakai X)
        public int CenterPointOffset; // -10000..10000
        public int PosCoeff;        // -10000..10000
        public int NegCoeff;        // -10000..10000
        public uint PosSatur;       // 0..10000
        public uint NegSatur;       // 0..10000
        public int DeadBand;        // 0..1000
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FFB_EFF_ENVLP
    {
        public byte EffectBlockIndex;
        public uint AttackLevel;    // 0..10000
        public uint FadeLevel;      // 0..10000
        public uint AttackTime;     // mikrodetik (raw HID PID)
        public uint FadeTime;       // mikrodetik
    }
}

/// <summary>
/// Pembungkus vJoy: acquire device, update sumbu X, dan terima callback FFB
/// yang diterjemahkan menjadi event domain lewat antrean thread-safe.
/// </summary>
public sealed class VJoyDevice : IDisposable
{
    private FfbGenCB? _keepAliveCb;   // cegah GC memindahkan delegate
    public uint Id { get; private set; }
    public bool Active { get; private set; }
    public readonly System.Collections.Concurrent.ConcurrentQueue<FfbEvent> FfbEvents = new();

    /// <summary>Return pesan error, atau null kalau oke.</summary>
    public static string? TestDriver()
    {
        try
        {
            if (!VJoyNative.vJoyEnabled()) return "driver vJoy tidak aktif";
            return null;
        }
        catch (DllNotFoundException) { return "vJoyInterface.dll tidak ditemukan — install vJoy"; }
        catch (Exception ex) { return ex.Message; }
    }

    /// <summary>Acquire perangkat + daftar callback FFB. Return error string / null.</summary>
    public string? Start(uint id)
    {
        string? err = TestDriver();
        if (err != null) return err;

        var st = (VjdStat)VJoyNative.GetVJDStatus(id);
        if (st == VjdStat.Busy) return $"vJoy #{id} dipakai proses lain (pid {VJoyNative.GetOwnerPid(id)})";
        if (st == VjdStat.Miss) return $"vJoy #{id} tidak ada — aktifkan lewat 'Configure vJoy'";
        if (!VJoyNative.AcquireVJD(id)) return $"gagal acquire vJoy #{id}";
        Id = id;
        Active = true;

        if (!VJoyNative.GetVJDAxisExist(id, VJoyNative.HidUsageX))
        {
            Stop();
            return $"vJoy #{id} tidak punya sumbu X — centang 'Axes: X' di Configure vJoy";
        }
        if (!VJoyNative.IsDeviceFfb(id))
        {
            Stop();
            return $"vJoy #{id} tidak mendukung FFB";
        }

        VJoyNative.ResetVJD(id);
        VJoyNative.SetAxis(VJoyNative.AxisCenter, id, VJoyNative.HidUsageX);

        _keepAliveCb = OnFfbPacket;
        VJoyNative.FfbRegisterGenCB(_keepAliveCb, IntPtr.Zero);
        return null;
    }

    public void Stop()
    {
        if (!Active) return;
        Active = false;
        try { VJoyNative.RelinquishVJD(Id); } catch { }
    }

    public void Dispose() => Stop();

    public void SetAxisX(int value)
    {
        if (Active) VJoyNative.SetAxis(value, Id, VJoyNative.HidUsageX);
    }

    // ---------------------------------------------------- callback FFB ------
    // Dipanggil di thread internal vJoy: parse dengan helper DLL, lalu
    // masukkan event ke antrean utk diproses engine.
    private void OnFfbPacket(IntPtr packet, IntPtr userData)
    {
        try
        {
            if (VJoyNative.Ffb_h_Type(packet, out FfbPType type) == 0)
            {
                switch (type)
                {
                    case FfbPType.PtEffrep:
                        {
                            var r = new VJoyNative.FFB_EFF_REPORT();
                            if (VJoyNative.Ffb_h_Eff_Report(packet, ref r) == 0)
                                FfbEvents.Enqueue(new FfbSetEffect
                                {
                                    Index = r.EffectBlockIndex,
                                    Type = (FfbEffectType)r.EffectType,
                                    DurationMs = r.Duration,
                                    Gain255 = r.Gain,
                                    Polar = r.Polar != 0,
                                    Direction = r.Direction,
                                    DirY = r.DirY,
                                });
                            break;
                        }
                    case FfbPType.PtConstrep:
                        {
                            var c = new VJoyNative.FFB_EFF_CONSTANT();
                            if (VJoyNative.Ffb_h_Eff_Constant(packet, ref c) == 0)
                                FfbEvents.Enqueue(new FfbSetConstant { Index = c.EffectBlockIndex, Magnitude = c.Magnitude });
                            break;
                        }
                    case FfbPType.PtRamprep:
                        {
                            var r = new VJoyNative.FFB_EFF_RAMP();
                            if (VJoyNative.Ffb_h_Eff_Ramp(packet, ref r) == 0)
                                FfbEvents.Enqueue(new FfbSetRamp { Index = r.EffectBlockIndex, Start = r.Start, End = r.End });
                            break;
                        }
                    case FfbPType.PtPridrep:
                        {
                            var p = new VJoyNative.FFB_EFF_PERIOD();
                            if (VJoyNative.Ffb_h_Eff_Period(packet, ref p) == 0)
                                FfbEvents.Enqueue(new FfbSetPeriodic
                                {
                                    Index = p.EffectBlockIndex,
                                    Magnitude = p.Magnitude,
                                    Offset = p.Offset,
                                    Phase = p.Phase,
                                    PeriodMs = p.Period,
                                });
                            break;
                        }
                    case FfbPType.PtEnvrep:
                        {
                            var e = new VJoyNative.FFB_EFF_ENVLP();
                            if (VJoyNative.Ffb_h_Eff_Envlp(packet, ref e) == 0)
                                FfbEvents.Enqueue(new FfbSetEnvelope
                                {
                                    Index = e.EffectBlockIndex,
                                    AttackLevel = e.AttackLevel,
                                    FadeLevel = e.FadeLevel,
                                    AttackTimeMs = e.AttackTime / 1000.0,   // HID: us
                                    FadeTimeMs = e.FadeTime / 1000.0,
                                });
                            break;
                        }
                    case FfbPType.PtCondrep:
                        {
                            var c = new VJoyNative.FFB_EFF_COND();
                            if (VJoyNative.Ffb_h_Eff_Cond(packet, ref c) == 0)
                                FfbEvents.Enqueue(new FfbSetCondition
                                {
                                    Index = c.EffectBlockIndex,
                                    IsY = c.IsY != 0,
                                    CpOffset = c.CenterPointOffset,
                                    PosCoeff = c.PosCoeff,
                                    NegCoeff = c.NegCoeff,
                                    PosSat = c.PosSatur,
                                    NegSat = c.NegSatur,
                                    DeadBand = c.DeadBand,
                                });
                            break;
                        }
                    case FfbPType.PtEfoprep:
                        {
                            var o = new VJoyNative.FFB_EFF_OP();
                            if (VJoyNative.Ffb_h_EffOp(packet, ref o) == 0)
                                FfbEvents.Enqueue(new FfbEffectOp
                                {
                                    Index = o.EffectBlockIndex,
                                    Op = (FfbOp)o.EffectOp,
                                    Loops = o.LoopCount,
                                });
                            break;
                        }
                    case FfbPType.PtBlkfrrep:
                        {
                            if (VJoyNative.Ffb_h_EBI(packet, out int idx) == 0)
                                FfbEvents.Enqueue(new FfbBlockFree { Index = idx });
                            break;
                        }
                    case FfbPType.PtCtrlrep:
                        {
                            if (VJoyNative.Ffb_h_DevCtrl(packet, out FfbCtrl ctrl) == 0)
                                FfbEvents.Enqueue(new FfbDeviceControl { Ctrl = ctrl });
                            break;
                        }
                    case FfbPType.PtGainrep:
                        {
                            if (VJoyNative.Ffb_h_DevGain(packet, out byte g) == 0)
                                FfbEvents.Enqueue(new FfbDeviceGain { Gain255 = g });
                            break;
                        }
                }
            }
        }
        catch { /* jangan pernah lempar exception ke native */ }
    }
}

// ---------------------------------------------------------------- events ----
public abstract class FfbEvent { public byte Index; }
public sealed class FfbSetEffect : FfbEvent
{
    public FfbEffectType Type; public ushort DurationMs; public byte Gain255;
    public bool Polar; public byte Direction; public byte DirY;
}
public sealed class FfbSetConstant : FfbEvent { public int Magnitude; }
public sealed class FfbSetRamp : FfbEvent { public int Start, End; }
public sealed class FfbSetPeriodic : FfbEvent
{
    public uint Magnitude; public int Offset; public uint Phase; public uint PeriodMs;
}
public sealed class FfbSetEnvelope : FfbEvent
{
    public uint AttackLevel, FadeLevel; public double AttackTimeMs, FadeTimeMs;
}
public sealed class FfbSetCondition : FfbEvent
{
    public bool IsY; public int CpOffset, PosCoeff, NegCoeff, DeadBand;
    public uint PosSat, NegSat;
}
public sealed class FfbEffectOp : FfbEvent { public FfbOp Op; public byte Loops; }
public sealed class FfbBlockFree : FfbEvent { }
public sealed class FfbDeviceControl : FfbEvent { public FfbCtrl Ctrl; }
public sealed class FfbDeviceGain : FfbEvent { public byte Gain255; }
