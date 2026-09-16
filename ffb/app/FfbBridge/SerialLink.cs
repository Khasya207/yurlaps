// ============================================================================
//  YurFFB — link serial ke firmware (framing + parser, mirror link.cpp).
//  Thread RX sendiri; event dipanggil dari thread RX (marshal sendiri di UI).
// ============================================================================

using System.IO.Ports;

namespace FfbBridge;

public sealed class SerialLink : IDisposable
{
    private SerialPort? _port;
    private Thread? _rxThread;
    private volatile bool _open;
    private readonly object _txLock = new();

    // parser RX
    private enum RxState { WaitSof1, WaitSof2, Len, Body }
    private RxState _state = RxState.WaitSof1;
    private byte _len;
    private byte _pos;
    private readonly byte[] _buf = new byte[Protocol.MaxLen + 2];

    public bool IsOpen => _open;

    // event (thread RX)
    public event Action<PongInfo>? OnPong;
    public event Action<DeviceState>? OnState;
    public event Action<DeviceConfig>? OnConfig;
    public event Action<string>? OnLog;
    public event Action<string>? OnDisconnected;

    public void Open(string portName, int baud)
    {
        Close();
        var p = new SerialPort(portName, baud, Parity.None, 8, StopBits.One)
        {
            ReadTimeout = 200,
            WriteTimeout = 200,
            ReadBufferSize = 4096,
            WriteBufferSize = 4096,
            DtrEnable = true,     // reset Arduino saat port dibuka (auto-reset)
            RtsEnable = false,
        };
        p.Open();
        _port = p;
        _open = true;
        _state = RxState.WaitSof1;
        _rxThread = new Thread(ReadLoop) { IsBackground = true, Name = "YurFfbRx" };
        _rxThread.Start();
    }

    public void Close()
    {
        _open = false;
        try { _rxThread?.Join(500); } catch { }
        _rxThread = null;
        try { _port?.Close(); } catch { }
        _port = null;
    }

    public void Dispose() => Close();

    // ------------------------------------------------------------------ TX --
    private void Send(byte cmd, ReadOnlySpan<byte> payload)
    {
        var port = _port;
        if (port is null || !_open) return;
        byte[] frame = Protocol.BuildFrame(cmd, payload);
        lock (_txLock)
        {
            try { port.Write(frame, 0, frame.Length); }
            catch (Exception ex) { OnDisconnected?.Invoke(ex.Message); }
        }
    }

    public void Ping() => Send(Protocol.CmdPing, ReadOnlySpan<byte>.Empty);
    public void Enable(bool on) => Send(Protocol.CmdEnable, new[] { (byte)(on ? 1 : 0) });
    public void Torque(short mPct)
    { Send(Protocol.CmdTorque, new[] { (byte)mPct, (byte)((ushort)mPct >> 8) }); }
    public void SetConfig(DeviceConfig c) => Send(Protocol.CmdSetConfig, c.Pack());
    public void GetState() => Send(Protocol.CmdGetState, ReadOnlySpan<byte>.Empty);
    public void SetCenter() => Send(Protocol.CmdSetCenter, ReadOnlySpan<byte>.Empty);
    public void SetTelemetry(ushort periodMs)
        => Send(Protocol.CmdSetTelem, new[] { (byte)periodMs, (byte)(periodMs >> 8) });
    public void SaveConfig() => Send(Protocol.CmdSaveConfig, ReadOnlySpan<byte>.Empty);
    public void ResetConfig() => Send(Protocol.CmdResetConfig, ReadOnlySpan<byte>.Empty);

    // ------------------------------------------------------------------ RX --
    private void ReadLoop()
    {
        var buf = new byte[512];
        while (_open)
        {
            try
            {
                int n = _port!.Read(buf, 0, buf.Length);
                for (int i = 0; i < n; i++) ParseByte(buf[i]);
            }
            catch (TimeoutException) { /* normal */ }
            catch (InvalidOperationException) { break; }
            catch (IOException) { break; }
            catch (Exception) { break; }
        }
        if (_open) OnDisconnected?.Invoke("port terputus");
    }

    private void ParseByte(byte b)
    {
        switch (_state)
        {
            case RxState.WaitSof1:
                if (b == Protocol.Sof1) _state = RxState.WaitSof2;
                break;
            case RxState.WaitSof2:
                if (b == Protocol.Sof2) _state = RxState.Len;
                else if (b != Protocol.Sof1) _state = RxState.WaitSof1;
                break;
            case RxState.Len:
                if (b < 2 || b > Protocol.MaxLen) { _state = RxState.WaitSof1; break; }
                _len = b;
                _pos = 0;
                _state = RxState.Body;
                break;
            case RxState.Body:
                _buf[_pos++] = b;
                if (_pos >= _len)
                {
                    HandleFrame();
                    _state = RxState.WaitSof1;
                }
                break;
        }
    }

    private void HandleFrame()
    {
        // [_buf[0]=cmd][_buf[1..len-2]=payload][_buf[len-1]=crc]
        byte cmd = _buf[0];
        ReadOnlySpan<byte> frame = _buf.AsSpan(0, _len);
        if (Protocol.Crc8(frame.Slice(0, _len - 1)) != _buf[_len - 1]) return; // crc salah
        ReadOnlySpan<byte> payload = frame.Slice(1, _len - 2);

        try
        {
            switch (cmd)
            {
                case Protocol.MsgPong when payload.Length == 11:
                    OnPong?.Invoke(new PongInfo(payload)); break;
                case Protocol.MsgState when payload.Length == 15:
                    OnState?.Invoke(new DeviceState(payload)); break;
                case Protocol.MsgConfig when payload.Length == 16:
                    OnConfig?.Invoke(DeviceConfig.Unpack(payload)); break;
                case Protocol.MsgLog:
                    OnLog?.Invoke(System.Text.Encoding.ASCII.GetString(
                        payload).TrimEnd('\0'));
                    break;
            }
        }
        catch (Exception ex)
        {
            OnLog?.Invoke($"RX parse error: {ex.Message}");
        }
    }

    // -------------------------------------------------------------- deteksi --
    /// <summary>Cari port YurFFB (blokir). Return nama port atau null.</summary>
    public static string? AutoDetect(Action<string>? progress = null)
    {
        foreach (string name in SerialPort.GetPortNames().Distinct().OrderBy(s => s))
        {
            progress?.Invoke($"mencoba {name}...");
            try
            {
                PongInfo? pong = null;
                using var link = new SerialLink();
                link.OnPong += p => pong = p;
                link.Open(name, 115200);
                Thread.Sleep(1800);            // tunggu bootloader/reset board
                for (int i = 0; i < 5 && pong is null; i++)
                {
                    link.Ping();
                    Thread.Sleep(200);
                }
                link.Close();
                if (pong is PongInfo info && info.IsYurFfb)
                {
                    progress?.Invoke($"ditemukan YurFFB di {name}");
                    return name;
                }
            }
            catch (Exception) { /* port sibuk/bukan YurFFB */ }
        }
        progress?.Invoke("tidak ditemukan");
        return null;
    }
}
