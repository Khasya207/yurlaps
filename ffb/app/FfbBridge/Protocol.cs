// ============================================================================
//  YurFFB — protokol serial v1 (sisi host). HARUS identik dengan
//  ffb/firmware/ffb_wheel/protocol.h (lihat docs/04-protokol.md).
//  Frame: [0xAA][0x55][LEN][CMD][PAYLOAD...][CRC8]
//    LEN = 1 (CMD) + N (payload) + 1 (CRC); CRC-8 poly 0x07 init 0x00.
// ============================================================================

namespace FfbBridge;

public static class Protocol
{
    public const byte Sof1 = 0xAA;
    public const byte Sof2 = 0x55;
    public const int MaxLen = 40;

    // host -> device
    public const byte CmdPing = 0x01;
    public const byte CmdEnable = 0x02;
    public const byte CmdTorque = 0x03;
    public const byte CmdSetConfig = 0x04;
    public const byte CmdGetState = 0x05;
    public const byte CmdSetCenter = 0x06;
    public const byte CmdSetTelem = 0x07;
    public const byte CmdSaveConfig = 0x08;
    public const byte CmdResetConfig = 0x09;

    // device -> host
    public const byte MsgPong = 0x81;
    public const byte MsgState = 0x82;
    public const byte MsgLog = 0x83;
    public const byte MsgConfig = 0x84;

    public const int ConfigPayloadLen = 16;

    // flag STATE
    public const byte FlWatchdog = 0x01;
    public const byte FlDisabled = 0x02;
    public const byte FlEndstop = 0x04;
    public const byte FlEncFault = 0x08;
    public const byte FlClamped = 0x10;

    // flag config
    public const byte CfgInvMotor = 0x01;
    public const byte CfgInvEncoder = 0x02;

    public const byte ProtoVer = 1;

    /// <summary>CRC-8: poly 0x07, init 0x00, no-reflect, no-xorout (CRC-8/SMBUS).</summary>
    public static byte Crc8(ReadOnlySpan<byte> data)
    {
        byte crc = 0x00;
        foreach (byte b in data)
        {
            crc ^= b;
            for (int i = 0; i < 8; i++)
                crc = (byte)((crc & 0x80) != 0 ? (crc << 1) ^ 0x07 : crc << 1);
        }
        return crc;
    }

    /// <summary>Bangun frame lengkap (termasuk SOF & CRC).</summary>
    public static byte[] BuildFrame(byte cmd, ReadOnlySpan<byte> payload)
    {
        if (payload.Length + 2 > MaxLen) throw new ArgumentException("payload terlalu panjang");
        byte len = (byte)(payload.Length + 2);
        byte[] frame = new byte[3 + len];
        frame[0] = Sof1;
        frame[1] = Sof2;
        frame[2] = len;
        frame[3] = cmd;
        payload.CopyTo(frame.AsSpan(4));
        frame[^1] = Crc8(frame.AsSpan(3, len - 1));
        return frame;
    }
}

/// <summary>Config perangkat (16 byte, little-endian) — layout = protocol.h.</summary>
public struct DeviceConfig
{
    public ushort MaxTorque;   // mPct 1..1000
    public short Slew;         // mPct per ms
    public short SoftMin;      // derajat
    public short SoftMax;      // derajat
    public short EndstopK;     // mPct/deg
    public short EndstopD;     // mPct per deg/s
    public short Damper;       // mPct per deg/s (lokal di MCU)
    public byte Flags;         // CfgInvMotor | CfgInvEncoder
    public byte Reserved;

    public static DeviceConfig Default => new()
    {
        MaxTorque = 1000, Slew = 40,
        SoftMin = -420, SoftMax = 420,
        EndstopK = 80, EndstopD = 20, Damper = 3,
        Flags = 0, Reserved = 0,
    };

    public readonly byte[] Pack()
    {
        byte[] p = new byte[16];
        WriteU16(p, 0, MaxTorque);
        WriteI16(p, 2, Slew);
        WriteI16(p, 4, SoftMin);
        WriteI16(p, 6, SoftMax);
        WriteI16(p, 8, EndstopK);
        WriteI16(p, 10, EndstopD);
        WriteI16(p, 12, Damper);
        p[14] = Flags;
        p[15] = Reserved;
        return p;
    }

    public static DeviceConfig Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length != 16) throw new ArgumentException("config harus 16 byte");
        return new DeviceConfig
        {
            MaxTorque = ReadU16(p, 0),
            Slew = ReadI16(p, 2),
            SoftMin = ReadI16(p, 4),
            SoftMax = ReadI16(p, 6),
            EndstopK = ReadI16(p, 8),
            EndstopD = ReadI16(p, 10),
            Damper = ReadI16(p, 12),
            Flags = p[14],
            Reserved = p[15],
        };
    }

    internal static void WriteU16(byte[] b, int o, ushort v)
    { b[o] = (byte)v; b[o + 1] = (byte)(v >> 8); }
    internal static void WriteI16(byte[] b, int o, short v) => WriteU16(b, o, (ushort)v);
    internal static ushort ReadU16(ReadOnlySpan<byte> b, int o)
        => (ushort)(b[o] | (b[o + 1] << 8));
    internal static short ReadI16(ReadOnlySpan<byte> b, int o)
        => (short)ReadU16(b, o);
}

/// <summary>Telemetri STATE (15 byte) — layout = protocol.h.</summary>
public readonly struct DeviceState
{
    public readonly uint Seq;
    public readonly int AngleMdeg;     // multi-turn
    public readonly short VelDdegS;    // deci-derajat/detik
    public readonly short TorqueMPct;  // perintah terakhir ke motor
    public readonly byte Flags;
    public readonly ushort LoopUs;

    public DeviceState(ReadOnlySpan<byte> p)
    {
        Seq = (uint)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
        AngleMdeg = (int)(p[4] | (p[5] << 8) | (p[6] << 16) | (p[7] << 24));
        VelDdegS = (short)(p[8] | (p[9] << 8));
        TorqueMPct = (short)(p[10] | (p[11] << 8));
        Flags = p[12];
        LoopUs = (ushort)(p[13] | (p[14] << 8));
    }

    public double AngleDeg => AngleMdeg / 1000.0;
    public double VelDegS => VelDdegS / 10.0;
}

/// <summary>Jawaban PONG (11 byte).</summary>
public readonly struct PongInfo
{
    public readonly string Signature;
    public readonly byte ProtoVer;
    public readonly byte FwMajor;
    public readonly byte FwMinor;
    public readonly byte EncoderType;   // 1=AS5600, 2=quadrature, 3=pot
    public readonly byte DriverType;    // 1=BTS7960, 2=PWM+DIR

    public PongInfo(ReadOnlySpan<byte> p)
    {
        Signature = System.Text.Encoding.ASCII.GetString(p.Slice(0, 6));
        ProtoVer = p[6];
        FwMajor = p[7];
        FwMinor = p[8];
        EncoderType = p[9];
        DriverType = p[10];
    }

    public bool IsYurFfb => Signature == "YURFFB" && ProtoVer == Protocol.ProtoVer;
}
