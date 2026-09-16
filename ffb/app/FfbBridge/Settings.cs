// ============================================================================
//  YurFFB — settings aplikasi (JSON, disimpan di %APPDATA%\FfbBridge).
// ============================================================================

using System.Text.Json;

namespace FfbBridge;

public class AppSettings
{
    public string ComPort { get; set; } = "";
    public int Baud { get; set; } = 115200;
    public uint VjoyId { get; set; } = 1;

    // gain & feeling
    public double GlobalGain { get; set; } = 1.0;      // 0..2
    public double MinForcePct { get; set; } = 0;       // 0..30
    public int MaxTorquePct { get; set; } = 100;       // 10..100
    public int RotationDeg { get; set; } = 900;        // 180..1440
    public int AutoCenterPct { get; set; } = 12;       // 0..100
    public double ConstantGain { get; set; } = 1.0;
    public double RampGain { get; set; } = 1.0;
    public double PeriodicGain { get; set; } = 1.0;
    public double SpringGain { get; set; } = 1.0;
    public double DamperGain { get; set; } = 1.0;
    public double FrictionGain { get; set; } = 1.0;

    // config perangkat (dikirim ke firmware)
    public int SoftEndstopDeg { get; set; } = 420;
    public int EndstopK { get; set; } = 80;
    public int EndstopD { get; set; } = 20;
    public int LocalDamper { get; set; } = 3;
    public int Slew { get; set; } = 40;
    public int MaxTorqueMPct { get; set; } = 1000;
    public bool InvertMotor { get; set; }
    public bool InvertEncoder { get; set; }

    private static string PathFor() => System.IO.Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "FfbBridge", "settings.json");

    public static AppSettings Load()
    {
        try
        {
            string p = PathFor();
            if (File.Exists(p))
                return JsonSerializer.Deserialize<AppSettings>(File.ReadAllText(p)) ?? new AppSettings();
        }
        catch { }
        return new AppSettings();
    }

    public void Save()
    {
        try
        {
            string p = PathFor();
            Directory.CreateDirectory(System.IO.Path.GetDirectoryName(p)!);
            File.WriteAllText(p, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch { /* settings gagal disimpan bukan masalah fatal */ }
    }

    public EngineSettings ToEngineSettings() => new()
    {
        GlobalGain = GlobalGain,
        MinForce = MinForcePct / 100.0,
        MaxTorquePct = MaxTorquePct,
        RotationDeg = RotationDeg,
        ConstantGain = ConstantGain,
        RampGain = RampGain,
        PeriodicGain = PeriodicGain,
        SpringGain = SpringGain,
        DamperGain = DamperGain,
        FrictionGain = FrictionGain,
        AutoCenterPct = AutoCenterPct,
        Enabled = true,   // dikelola runtime oleh MainForm
    };

    public DeviceConfig ToDeviceConfig() => new()
    {
        MaxTorque = (ushort)Math.Clamp(MaxTorqueMPct, 1, 1000),
        Slew = (short)Math.Clamp(Slew, 0, 1000),
        SoftMin = (short)(-Math.Abs(SoftEndstopDeg)),
        SoftMax = (short)Math.Abs(SoftEndstopDeg),
        EndstopK = (short)Math.Clamp(EndstopK, 0, 32767),
        EndstopD = (short)Math.Clamp(EndstopD, 0, 32767),
        Damper = (short)Math.Clamp(LocalDamper, 0, 32767),
        Flags = (byte)((InvertMotor ? Protocol.CfgInvMotor : 0) |
                       (InvertEncoder ? Protocol.CfgInvEncoder : 0)),
        Reserved = 0,
    };
}
