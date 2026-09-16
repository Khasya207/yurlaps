// ============================================================================
//  FfbBridge — UI utama.
//  Panel kiri: koneksi firmware, vJoy, pengaturan gaya & perangkat.
//  Panel kanan: telemetri, uji coba, E-STOP, log.
// ============================================================================

namespace FfbBridge;

public sealed class MainForm : Form
{
    private readonly AppSettings _settings = AppSettings.Load();
    private readonly SerialLink _serial = new();
    private readonly VJoyDevice _vjoy = new();
    private FfbEngine? _engine;

    // ---- kontrol kiri ----
    private readonly ComboBox _cmbPort = new() { DropDownStyle = ComboBoxStyle.DropDownList };
    private readonly ComboBox _cmbBaud = new() { DropDownStyle = ComboBoxStyle.DropDownList };
    private readonly Button _btnRefresh = new() { Text = "↻", Width = 32 };
    private readonly Button _btnDetect = new() { Text = "Deteksi Otomatis" };
    private readonly Button _btnConnect = new() { Text = "Hubungkan" };
    private readonly Label _lblDevice = new() { Text = "— belum terhubung —", ForeColor = Color.Gray };

    private readonly NumericUpDown _numVjoyId = new() { Minimum = 1, Maximum = 16, Value = 1 };
    private readonly Button _btnVjoy = new() { Text = "Aktifkan vJoy" };
    private readonly Label _lblVjoy = new() { Text = "vJoy belum aktif", ForeColor = Color.Gray };

    private readonly CheckBox _chkActive = new() { Text = "FFB aktif (motor enable)", Checked = false };

    // ---- kontrol kanan ----
    private readonly Label _lblAngle = new();
    private readonly Label _lblVel = new();
    private readonly Label _lblTorque = new();
    private readonly Label _lblFlags = new();
    private readonly Label _lblEffects = new();
    private readonly Label _lblLink = new();
    private readonly ProgressBar _barTorque = new() { Minimum = -100, Maximum = 100 };

    private readonly Button _btnEStop = new()
    {
        Text = "⛔ E-STOP (Esc)",
        BackColor = Color.Firebrick,
        ForeColor = Color.White,
        FlatStyle = FlatStyle.Flat,
        Height = 56,
    };

    private readonly TextBox _txtLog = new()
    {
        Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical,
        Dock = DockStyle.Fill, BackColor = Color.FromArgb(20, 20, 20), ForeColor = Color.Gainsboro,
        Font = new Font("Consolas", 8.5f),
    };

    private readonly System.Windows.Forms.Timer _uiTimer = new() { Interval = 200 };

    private readonly List<(TrackBar bar, Label val, Func<int> get, Action<int> set)> _trackRows = new();

    public MainForm()
    {
        Text = "FfbBridge — YurFFB (DIY Force Feedback)";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(980, 680);
        Size = new Size(1120, 760);
        KeyPreview = true;

        BuildLayout();
        LoadSettingsToUi();

        _btnRefresh.Click += (_, _) => RefreshPorts();
        _btnDetect.Click += async (_, _) => await DetectAsync();
        _btnConnect.Click += (_, _) => ToggleConnect();
        _btnVjoy.Click += (_, _) => ToggleVjoy();
        _chkActive.CheckedChanged += (_, _) => ApplyEngineSettings();
        _btnEStop.Click += (_, _) => EmergencyStop();
        KeyDown += (_, e) => { if (e.KeyCode == Keys.Escape) EmergencyStop(); };

        _serial.OnPong += p => BeginInvoke(new Action(() => OnPong(p)));
        _serial.OnLog += m => BeginInvoke(new Action(() => Log(m)));
        _serial.OnConfig += _ => { };
        _serial.OnState += st => _engine?.FeedState(st);
        _serial.OnDisconnected += why => BeginInvoke(new Action(() => Disconnected(why)));

        _uiTimer.Tick += (_, _) => UpdateTelemetry();
        _uiTimer.Start();

        RefreshPorts();
        FormClosing += (_, _) => Shutdown();
    }

    // ---------------------------------------------------------------- layout -
    private void BuildLayout()
    {
        var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2 };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 400));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        // ================= panel kiri =================
        var left = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 5, AutoScroll = true };
        left.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        var gConn = Group("1. Firmware (Arduino)");
        var connRows = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true };
        connRows.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 70));
        connRows.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));
        connRows.Controls.Add(Lbl("Port COM:"), 0, 0);
        connRows.Controls.Add(_cmbPort, 1, 0);
        connRows.Controls.Add(Lbl("Baud:"), 0, 1);
        connRows.Controls.Add(_cmbBaud, 1, 1);
        connRows.Controls.Add(_btnRefresh, 0, 2);
        connRows.Controls.Add(_btnDetect, 1, 2);
        connRows.SetColumnSpan(_btnDetect, 1);
        var btnRow = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2 };
        btnRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        _btnConnect.Dock = DockStyle.Fill;
        _btnDetect.Dock = DockStyle.Fill;
        _btnRefresh.Dock = DockStyle.Fill;
        btnRow.Controls.Add(_btnConnect, 0, 0);
        _chkActive.Dock = DockStyle.Fill;
        btnRow.Controls.Add(_chkActive, 1, 0);
        btnRow.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        connRows.Controls.Add(btnRow, 0, 3);
        connRows.SetColumnSpan(btnRow, 2);
        _lblDevice.Dock = DockStyle.Fill;
        connRows.Controls.Add(_lblDevice, 0, 4);
        connRows.SetColumnSpan(_lblDevice, 2);
        gConn.Controls.Add(connRows);
        left.Controls.Add(gConn, 0, 0);

        var gVjoy = Group("2. vJoy — perangkat virtual untuk game");
        var vjoyRows = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true };
        vjoyRows.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 70));
        vjoyRows.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));
        vjoyRows.Controls.Add(Lbl("Device vJoy (1-16):"), 0, 0);
        _numVjoyId.Dock = DockStyle.Fill;
        vjoyRows.Controls.Add(_numVjoyId, 1, 0);
        _btnVjoy.Dock = DockStyle.Fill;
        vjoyRows.Controls.Add(_btnVjoy, 0, 1);
        vjoyRows.SetColumnSpan(_btnVjoy, 2);
        _lblVjoy.Dock = DockStyle.Fill;
        vjoyRows.Controls.Add(_lblVjoy, 0, 2);
        vjoyRows.SetColumnSpan(_lblVjoy, 2);
        gVjoy.Controls.Add(vjoyRows);
        left.Controls.Add(gVjoy, 0, 1);

        var gFeel = Group("3. Gaya / FFB");
        var feel = new TableLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        feel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        AddTrack(feel, "Gain global (%)", 0, 200,
            () => (int)(_settings.GlobalGain * 100), v => _settings.GlobalGain = v / 100.0);
        AddTrack(feel, "Min force (%)", 0, 30,
            () => (int)(_settings.MinForcePct), v => _settings.MinForcePct = v);
        AddTrack(feel, "Max torsi (%)", 10, 100,
            () => _settings.MaxTorquePct, v => _settings.MaxTorquePct = v);
        AddCombo(feel, "Rotasi kemudi (°)", [240, 360, 450, 540, 720, 900, 1080, 1440],
            () => _settings.RotationDeg, v => _settings.RotationDeg = v);
        AddTrack(feel, "Auto-center idle (%)", 0, 100,
            () => _settings.AutoCenterPct, v => _settings.AutoCenterPct = v);
        AddTrack(feel, "Constant gain (%)", 0, 200,
            () => (int)(_settings.ConstantGain * 100), v => _settings.ConstantGain = v / 100.0);
        AddTrack(feel, "Spring gain (%)", 0, 200,
            () => (int)(_settings.SpringGain * 100), v => _settings.SpringGain = v / 100.0);
        AddTrack(feel, "Damper gain (%)", 0, 200,
            () => (int)(_settings.DamperGain * 100), v => _settings.DamperGain = v / 100.0);
        AddTrack(feel, "Friction gain (%)", 0, 200,
            () => (int)(_settings.FrictionGain * 100), v => _settings.FrictionGain = v / 100.0);
        AddTrack(feel, "Periodic gain (%)", 0, 200,
            () => (int)(_settings.PeriodicGain * 100), v => _settings.PeriodicGain = v / 100.0);
        gFeel.Controls.Add(feel);
        left.Controls.Add(gFeel, 0, 2);

        var gDev = Group("4. Perangkat (dikirim ke firmware)");
        var dev = new TableLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        dev.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        AddTrack(dev, "Endstop lunak (±°)", 90, 720,
            () => _settings.SoftEndstopDeg, v => _settings.SoftEndstopDeg = v);
        AddTrack(dev, "Endstop K (mPct/°)", 0, 300,
            () => _settings.EndstopK, v => _settings.EndstopK = v);
        AddTrack(dev, "Endstop D (redaman)", 0, 100,
            () => _settings.EndstopD, v => _settings.EndstopD = v);
        AddTrack(dev, "Damper lokal MCU", 0, 30,
            () => _settings.LocalDamper, v => _settings.LocalDamper = v);
        AddTrack(dev, "Slew rate (mPct/ms)", 5, 200,
            () => _settings.Slew, v => _settings.Slew = v);
        var invRow = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true };
        var chkInvM = new CheckBox { Text = "Invert motor", AutoSize = true };
        var chkInvE = new CheckBox { Text = "Invert encoder", AutoSize = true };
        chkInvM.DataBindings.Add("Checked", _settings, nameof(AppSettings.InvertMotor), false,
            DataSourceUpdateMode.OnPropertyChanged);
        chkInvE.DataBindings.Add("Checked", _settings, nameof(AppSettings.InvertEncoder), false,
            DataSourceUpdateMode.OnPropertyChanged);
        invRow.Controls.Add(chkInvM);
        invRow.Controls.Add(chkInvE);
        dev.Controls.Add(invRow);
        var devBtns = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 3, AutoSize = true };
        var btnSendCfg = new Button { Text = "Kirim config", Dock = DockStyle.Fill };
        var btnStoreCfg = new Button { Text = "Simpan EEPROM", Dock = DockStyle.Fill };
        var btnCenter = new Button { Text = "Set center", Dock = DockStyle.Fill };
        btnSendCfg.Click += (_, _) => SendDeviceConfig();
        btnStoreCfg.Click += (_, _) => { SendDeviceConfig(); _serial.SaveConfig(); };
        btnCenter.Click += (_, _) => _serial.SetCenter();
        devBtns.Controls.Add(btnSendCfg, 0, 0);
        devBtns.Controls.Add(btnStoreCfg, 1, 0);
        devBtns.Controls.Add(btnCenter, 2, 0);
        dev.Controls.Add(devBtns);
        gDev.Controls.Add(dev);
        left.Controls.Add(gDev, 0, 3);

        left.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        left.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        left.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        left.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        left.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.Controls.Add(left, 0, 0);

        // ================= panel kanan =================
        var right = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 4 };
        right.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        right.RowStyles.Add(new RowStyle(SizeType.Percent, 100));

        right.Controls.Add(_btnEStop, 0, 0);
        _btnEStop.Dock = DockStyle.Fill;
        _btnEStop.Font = new Font(Font, FontStyle.Bold);

        var gTele = Group("Telemetri");
        var tele = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true };
        tele.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140));
        tele.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        _lblAngle.Dock = _lblVel.Dock = _lblTorque.Dock = _lblFlags.Dock =
            _lblEffects.Dock = _lblLink.Dock = DockStyle.Fill;
        tele.Controls.Add(Lbl("Sudut:"), 0, 0); tele.Controls.Add(_lblAngle, 1, 0);
        tele.Controls.Add(Lbl("Kecepatan:"), 0, 1); tele.Controls.Add(_lblVel, 1, 1);
        tele.Controls.Add(Lbl("Torsi (mPct):"), 0, 2); tele.Controls.Add(_lblTorque, 1, 2);
        tele.Controls.Add(Lbl("Status:"), 0, 3); tele.Controls.Add(_lblFlags, 1, 3);
        tele.Controls.Add(Lbl("Efek aktif:"), 0, 4); tele.Controls.Add(_lblEffects, 1, 4);
        tele.Controls.Add(Lbl("Link:"), 0, 5); tele.Controls.Add(_lblLink, 1, 5);
        _barTorque.Height = 16;
        var barHost = new TableLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        barHost.Controls.Add(_barTorque, 0, 0);
        tele.Controls.Add(Lbl(""), 0, 6); tele.Controls.Add(barHost, 1, 6);
        gTele.Controls.Add(tele);
        right.Controls.Add(gTele, 0, 1);

        var gTest = Group("Uji coba (tanpa game)");
        var test = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 5, AutoSize = true };
        var btnCw = new Button { Text = "Torsi ➡", Dock = DockStyle.Fill };
        var btnCcw = new Button { Text = "⬅ Torsi", Dock = DockStyle.Fill };
        var btnSine = new Button { Text = "Sine", Dock = DockStyle.Fill };
        var btnSpring = new Button { Text = "Spring", Dock = DockStyle.Fill };
        var btnTestOff = new Button { Text = "Stop", Dock = DockStyle.Fill };
        btnCw.Click += (_, _) => { if (_engine != null) _engine.Test = TestMode.ConstCw; };
        btnCcw.Click += (_, _) => { if (_engine != null) _engine.Test = TestMode.ConstCcw; };
        btnSine.Click += (_, _) => { if (_engine != null) _engine.Test = TestMode.Sine; };
        btnSpring.Click += (_, _) => { if (_engine != null) _engine.Test = TestMode.Spring; };
        btnTestOff.Click += (_, _) => { if (_engine != null) _engine.Test = TestMode.None; };
        test.Controls.Add(btnCw, 0, 0); test.Controls.Add(btnCcw, 1, 0);
        test.Controls.Add(btnSine, 2, 0); test.Controls.Add(btnSpring, 3, 0);
        test.Controls.Add(btnTestOff, 4, 0);
        gTest.Controls.Add(test);
        right.Controls.Add(gTest, 0, 2);

        right.Controls.Add(_txtLog, 0, 3);
        root.Controls.Add(right, 1, 0);

        Controls.Add(root);
    }

    private static GroupBox Group(string title)
    {
        var g = new GroupBox
        {
            Text = title,
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(8, 4, 8, 8),
        };
        return g;
    }

    private static Label Lbl(string text) => new()
    { Text = text, AutoSize = true, Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };

    private void AddTrack(Control parent, string name, int min, int max,
                          Func<int> get, Action<int> set)
    {
        var row = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true, Height = 30 };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 160));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        var bar = new TrackBar
        {
            Minimum = min, Maximum = Math.Max(min + 1, max), TickFrequency = Math.Max(1, (max - min) / 10),
            SmallChange = 1, LargeChange = Math.Max(1, (max - min) / 10),
            Dock = DockStyle.Fill, Height = 26,
        };
        var val = new Label { Text = "", AutoSize = false, Width = 52, TextAlign = ContentAlignment.MiddleRight };
        bar.Value = Math.Clamp(get(), min, Math.Max(min + 1, max));
        val.Text = bar.Value.ToString();
        row.Controls.Add(Lbl(name), 0, 0);
        var barHost = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2 };
        barHost.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        barHost.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 56));
        barHost.Controls.Add(bar, 0, 0);
        barHost.Controls.Add(val, 1, 0);
        row.Controls.Add(barHost, 1, 0);
        parent.Controls.Add(row);
        bar.ValueChanged += (_, _) =>
        {
            val.Text = bar.Value.ToString();
            set(bar.Value);
            ApplyEngineSettings();
        };
        _trackRows.Add((bar, val, get, set));
    }

    private void AddCombo(Control parent, string name, int[] options,
                          Func<int> get, Action<int> set)
    {
        var row = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 160));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        var cmb = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Dock = DockStyle.Fill };
        foreach (int o in options) cmb.Items.Add(o.ToString());
        int cur = get();
        int sel = Array.IndexOf(options, cur);
        if (sel < 0) sel = Array.IndexOf(options, 900);
        cmb.SelectedIndex = sel >= 0 ? sel : 0;
        row.Controls.Add(Lbl(name), 0, 0);
        row.Controls.Add(cmb, 1, 0);
        parent.Controls.Add(row);
        cmb.SelectedIndexChanged += (_, _) =>
        {
            if (int.TryParse(cmb.SelectedItem?.ToString(), out int v)) { set(v); ApplyEngineSettings(); }
        };
    }

    // ------------------------------------------------------------ settings ---
    private void LoadSettingsToUi()
    {
        foreach (var (bar, val, get, _) in _trackRows)
        {
            bar.Value = Math.Clamp(get(), bar.Minimum, bar.Maximum);
            val.Text = bar.Value.ToString();
        }
        _numVjoyId.Value = Math.Clamp((int)_settings.VjoyId, 1, 16);
        _cmbBaud.Items.AddRange(new object[] { "115200", "250000", "500000" });
        _cmbBaud.SelectedIndex = _settings.Baud switch
        {
            250000 => 1, 500000 => 2, _ => 0,
        };
    }

    private void ApplyEngineSettings()
    {
        var es = _settings.ToEngineSettings();
        es.Enabled = _chkActive.Checked && _serial.IsOpen;
        _engine?.UpdateSettings(es);
        _settings.Save();
    }

    private void SendDeviceConfig()
    {
        if (!_serial.IsOpen) { Log("hubungkan firmware dulu"); return; }
        _serial.SetConfig(_settings.ToDeviceConfig());
    }

    // ------------------------------------------------------------- koneksi ---
    private void RefreshPorts()
    {
        _cmbPort.Items.Clear();
        _cmbPort.Items.AddRange(System.IO.Ports.SerialPort.GetPortNames()
            .Distinct().OrderBy(p => p).ToArray());
        if (_cmbPort.Items.Count > 0)
        {
            int idx = _cmbPort.Items.IndexOf(_settings.ComPort);
            _cmbPort.SelectedIndex = idx >= 0 ? idx : 0;
        }
    }

    private async Task DetectAsync()
    {
        _btnDetect.Enabled = false;
        try
        {
            string? port = await Task.Run(() => SerialLink.AutoDetect(m =>
                BeginInvoke(new Action(() => Log(m)))));
            if (port != null)
            {
                _settings.ComPort = port;
                RefreshPorts();
                Log($"YurFFB ditemukan di {port} — klik Hubungkan");
            }
            else Log("tidak ada YurFFB ditemukan (cek kabel USB & power driver motor)");
        }
        finally { _btnDetect.Enabled = true; }
    }

    private void ToggleConnect()
    {
        if (_serial.IsOpen) { Disconnected("diputus manual"); _serial.Close(); return; }

        if (_cmbPort.SelectedItem is not string port)
        { Log("pilih port COM dulu"); return; }
        int baud = int.TryParse(_cmbBaud.SelectedItem?.ToString(), out int b) ? b : 115200;

        try
        {
            _serial.Open(port, baud);
        }
        catch (Exception ex)
        {
            Log($"gagal membuka {port}: {ex.Message}");
            return;
        }

        _settings.ComPort = port;
        _settings.Baud = baud;
        _settings.Save();
        _btnConnect.Text = "Putuskan";
        Log($"terhubung ke {port} @ {baud} — menunggu firmware...");
        _pongSeen = false;

        _engine ??= CreateEngine();
        ApplyEngineSettings();

        // board baru di-reset oleh DTR saat port dibuka — ping berkali-kali
        // sampai firmware menjawab (bootloader 32U4/Uno bisa makan 1-2 detik)
        _ = Task.Run(async () =>
        {
            for (int i = 0; i < 12 && _serial.IsOpen && !_pongSeen; i++)
            {
                _serial.Ping();
                await Task.Delay(700);
            }
        });
    }

    private volatile bool _pongSeen;

    private FfbEngine CreateEngine()
    {
        var eng = new FfbEngine(_serial, _vjoy.Active ? _vjoy : null);
        eng.Start();
        ApplyEngineSettings();
        Log("engine FFB berjalan (500 Hz)");
        return eng;
    }

    private void OnPong(PongInfo p)
    {
        _pongSeen = true;
        if (!p.IsYurFfb)
        {
            _lblDevice.Text = $"perangkat lain di port ini ({p.Signature})";
            _lblDevice.ForeColor = Color.OrangeRed;
            return;
        }
        string enc = p.EncoderType switch { 1 => "AS5600", 2 => "quadrature", 3 => "potensiometer", _ => "?" };
        string drv = p.DriverType switch { 1 => "BTS7960", 2 => "PWM+DIR", _ => "?" };
        _lblDevice.Text = $"YurFFB fw {p.FwMajor}.{p.FwMinor} · {enc} · {drv}";
        _lblDevice.ForeColor = Color.Green;
        Log($"firmware menjawab: {enc} + {drv}");

        if (_engine is null) _engine = CreateEngine();
        ApplyEngineSettings();
        _serial.SetConfig(_settings.ToDeviceConfig());
        _serial.SetTelemetry(3);
        if (_chkActive.Checked) _serial.Enable(true);
    }

    private void Disconnected(string why)
    {
        if (!_serial.IsOpen) return;
        _serial.Close();
        _btnConnect.Text = "Hubungkan";
        _lblDevice.Text = "— terputus —";
        _lblDevice.ForeColor = Color.Gray;
        Log($"serial terputus ({why})");
        var es = _settings.ToEngineSettings();
        es.Enabled = false;
        _engine?.UpdateSettings(es);
    }

    // ---------------------------------------------------------------- vJoy ---
    private void ToggleVjoy()
    {
        if (_vjoy.Active)
        {
            _vjoy.Stop();
            _lblVjoy.Text = "vJoy berhenti";
            _lblVjoy.ForeColor = Color.Gray;
            _btnVjoy.Text = "Aktifkan vJoy";
            return;
        }

        uint id = (uint)_numVjoyId.Value;
        string? err = _vjoy.Start(id);
        if (err != null)
        {
            _lblVjoy.Text = err;
            _lblVjoy.ForeColor = Color.OrangeRed;
            Log($"vJoy error: {err}");
            return;
        }
        _settings.VjoyId = id;
        _settings.Save();
        _lblVjoy.Text = $"AKTIF — game melihat \"vJoy Device\" #{id} (DirectInput + FFB)";
        _lblVjoy.ForeColor = Color.Green;
        _btnVjoy.Text = "Matikan vJoy";
        Log($"vJoy #{id} aktif — pilih \"vJoy Device\" di dalam game");
        _engine?.AttachVjoy(_vjoy);
    }

    // ------------------------------------------------------------ telemetry --
    private void UpdateTelemetry()
    {
        var st = _engine?.LatestStats;
        if (st is null)
        {
            _lblAngle.Text = _lblVel.Text = _lblTorque.Text = "—";
            _lblFlags.Text = _lblEffects.Text = "";
            return;
        }
        _lblAngle.Text = $"{st.AngleDeg,8:F1}°";
        _lblVel.Text = $"{st.VelDegS,8:F0}°/s";
        _lblTorque.Text = $"{st.TorqueOut,6} / ±1000";
        _barTorque.Value = Math.Clamp(st.TorqueOut / 10, -100, 100);

        var f = new List<string>();
        if ((st.DeviceFlags & Protocol.FlWatchdog) != 0) f.Add("WATCHDOG");
        if ((st.DeviceFlags & Protocol.FlDisabled) != 0) f.Add("DISABLED");
        if ((st.DeviceFlags & Protocol.FlEndstop) != 0) f.Add("ENDSTOP");
        if ((st.DeviceFlags & Protocol.FlEncFault) != 0) f.Add("ENC-FAULT");
        if ((st.DeviceFlags & Protocol.FlClamped) != 0) f.Add("CLAMP");
        _lblFlags.Text = f.Count > 0 ? string.Join(" · ", f) : "OK";
        _lblFlags.ForeColor = f.Contains("ENC-FAULT") ? Color.OrangeRed :
                              f.Count > 0 ? Color.Orange : Color.Green;

        _lblEffects.Text = st.PlayingCount > 0 ? $"{st.PlayingCount}: {st.PlayingDesc}" : "idle (auto-center)";
        _lblLink.Text = st.DeviceConnected
            ? $"OK · loop {st.LoopMs:F1} ms · antre FFB {st.FfbQueueDepth}"
            : "menunggu telemetri...";
    }

    // -------------------------------------------------------------- util ----
    private void EmergencyStop()
    {
        _chkActive.Checked = false;
        if (_engine != null) { _engine.Test = TestMode.None; _engine.StopAll(); }
        if (_serial.IsOpen) { _serial.Torque(0); _serial.Enable(false); }
        Log("E-STOP — motor dimatikan");
    }

    private void Log(string msg)
    {
        if (InvokeRequired) { BeginInvoke(new Action(() => Log(msg))); return; }
        _txtLog.AppendText($"[{DateTime.Now:HH:mm:ss}] {msg}\r\n");
        if (_txtLog.Text.Length > 9000)
            _txtLog.Text = _txtLog.Text[^8000..];
    }

    private void Shutdown()
    {
        _uiTimer.Stop();
        try { _serial.Torque(0); _serial.Enable(false); } catch { }
        _engine?.Dispose();
        _serial.Dispose();
        _vjoy.Dispose();
        _settings.Save();
    }
}
