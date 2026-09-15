// Menaikkan stack loop task dari default (biasanya 8KB) ke 16KB.
// Perlu karena beberapa fungsi (handleHistory, handleQualifyHistory,
// cleanupOldRaceHistory) memakai array String[] lumayan besar di stack,
// dan RAM ESP32-C3 lebih ketat dibanding S3 sehingga margin stack juga
// lebih ketat. Harus didefinisikan SEBELUM #include <Arduino.h>.
#define ARDUINO_LOOP_STACK_SIZE (16 * 1024)

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <FFat.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "UI.h"

// =============================================================
// v4.2 -- FIX AKURASI LAPTIME (baca CHANGELOG_v4.2.md untuk detail lengkap)
// =============================================================
// Root cause utama laptime tidak akurat di v4.1 (semua di sisi ESP32,
// PSoC dianggap benar/di luar cakupan investigasi ini):
//
// 1. Timestamp lap (millis()) diambil TERLALU BELAKANGAN -- di dalam
//    addLap()/addQualifyLap(), setelah parsing string DAN setelah
//    broadcastTerminalEntry() (kirim WebSocket) sempat jalan duluan di
//    processPsocLine(). webSocket.broadcastTXT() itu blocking (nunggu
//    TCP send selesai/gagal) -- di WiFi (apalagi softAP dgn banyak
//    client/gangguan sinyal di venue race), ini bisa nunggu dari
//    beberapa ms sampai puluhan/ratusan ms, TIDAK KONSTAN. Setiap kali
//    itu terjadi SEBELUM timestamp diambil, durasi blocking itu ikut
//    "numpang" masuk ke laptime yang tercatat.
//
// 2. loop() memanggil server.handleClient() & webSocket.loop() (juga
//    bisa makan waktu tidak menentu kalau ada request HTTP berat/client
//    lambat) SEBELUM membaca psocSerial -- kalau itu lama, pembacaan
//    baris dari PSoC ikut telat, dan telatnya TIDAK KONSTAN antar lap.
//
// 3. USB CDC (Serial bawaan ESP32-C3 lewat port USB-C) dipakai untuk
//    banyak Serial.print/println/printf debug, termasuk di jalur
//    "panas" tiap ada pembacaan transponder. Kalau device dipakai TANPA
//    kabel USB tersambung ke PC/Serial Monitor (misal cuma dikasih daya
//    powerbank/adaptor, sementara laptop konek ke device via WiFi --
//    ini SETUP NORMAL YurLaps), TX buffer CDC tidak pernah dikosongkan
//    host manapun. Begitu buffer itu penuh, panggilan Serial.print
//    berikutnya bisa BLOCKING lama (bahkan dilaporkan bisa hang di
//    beberapa versi core kalau ada Serial.flush()) karena tidak ada
//    yang "menarik" datanya keluar. Referensi issue resmi core
//    arduino-esp32: #9172, #9545, #8088 (semua soal HW-CDC ESP32-C3/S3
//    yang macet/hang kalau tidak ada host CDC yang benar-benar terbuka).
//
// FIX v4.2:
// A. Timestamp diambil di titik SEDINI DAN SESTABIL mungkin: lewat
//    HardwareSerial::onReceive() callback, yang dieksekusi oleh task
//    FreeRTOS terpisah milik driver UART (BUKAN loop() Arduino) begitu
//    UART event (RX timeout / FIFO penuh) terjadi -- jadi waktunya tidak
//    tersandera antrean loop() sama sekali, walau loop() lagi sibuk
//    ngirim WebSocket / handle HTTP. Baris + timestamp langsung
//    dimasukkan ke FreeRTOS queue (psocLineQueue); loop() cuma
//    men-drain-nya (operasi cepat, non-blocking) lalu memproses laptime
//    pakai timestamp yang SUDAH diambil itu -- bukan memanggil millis()
//    ulang belakangan.
// B. Semua panggilan jaringan (broadcast WebSocket) di jalur pembacaan
//    PSoC DIPINDAH ke SETELAH lap tercatat, dan malah DIKUMPULKAN dulu
//    (pendingStateBroadcast flag, terminal entry di-queue) supaya kalau
//    beberapa baris PSoC numpuk di satu siklus loop() (mis. crossing
//    hampir bersamaan/banyak "hit" mentah), cuma ada SATU broadcastState()
//    di akhir -- bukan satu broadcast blocking per baris.
// C. Semua Serial.print/println/printf (CDC) diganti helper dbgPrint*()
//    yang cuma benar-benar menulis kalau `Serial` memang lagi tersambung
//    ke host (operator bool() bawaan HWCDC). Kalau tidak ada host sama
//    sekali (kondisi normal pas race, laptop cuma nyambung WiFi), TIDAK
//    ADA percobaan tulis ke CDC sama sekali -- nol risiko blocking dari
//    situ.
// D. Buffer RX psocSerial dinaikkan (512 byte) sebagai jaring pengaman
//    tambahan kalau loop() sempat telat sebentar karena hal lain.
// =============================================================

// Helper debug print yang aman dari CDC block/hang (lihat catatan di
// atas). `if (Serial)` mengecek apakah host USB-CDC benar2 terbuka --
// kalau tidak, isi fungsi ini jadi no-op super cepat (bukan blocking).
template<typename T>
inline void dbgPrint(T v) { if (Serial) Serial.print(v); }
template<typename T>
inline void dbgPrintln(T v) { if (Serial) Serial.println(v); }
inline void dbgPrintln() { if (Serial) Serial.println(); }
template<typename... Args>
inline void dbgPrintf(const char *fmt, Args... args) { if (Serial) Serial.printf(fmt, args...); }

// Watchdog level-aplikasi: kalau loop() macet total (misal blocking I/O ke
// client WebSocket yang koneksinya sudah mati/zombie tapi socket belum
// ke-close bersih -- kasus umum kalau HP pengguna dikunci/ditinggal lama),
// device auto-restart sendiri setelah WATCHDOG_TIMEOUT_SEC detik alih-alih
// nge-freeze permanen sampai dicabut-colok manual.
#define WATCHDOG_TIMEOUT_SEC 10

// PENTING (ESP32-C3 Super Mini):
// GPIO17 (dan GPIO11-16) TIDAK exist / tidak di-broncok ke header board ini
// -- pin-pin itu dipakai internal untuk SPI flash. Board S3 punya GPIO17,
// tapi C3 Super Mini tidak. Pin di bawah sudah dipilih aman:
// - bukan strapping pin (hindari GPIO2, GPIO8, GPIO9)
// - general purpose murni, tidak dipakai untuk fungsi khusus (USB/JTAG/flash)
#define PSOC_RX_PIN 2

// v4.2: versi firmware, ditampilkan di UI (header & halaman Setting) dan
// dikirim ke browser lewat buildFullDataJson() -- satu sumber kebenaran,
// tidak di-hardcode terpisah di UI.h.
#define FIRMWARE_VERSION "v4.8"
#define PSOC_TX_PIN 5

HardwareSerial psocSerial(1);
WebServer server(80);
WebSocketsServer webSocket(81);
// Dipakai untuk push data realtime (lap baru, race start/stop, dll) ke
// semua browser yang terhubung, tanpa browser harus polling terus-menerus.

// v4.2: baris PSoC + timestamp-nya dikirim dari onReceive() (task UART
// terpisah) ke loop() lewat queue FreeRTOS ini. Timestamp SUDAH final
// begitu masuk queue -- loop() tinggal pakai, tidak pernah panggil
// millis() ulang buat itu.
#define PSOC_LINE_MAXLEN 100
struct PsocLineEvent {
  char line[PSOC_LINE_MAXLEN + 1];
  unsigned long ts;
};
QueueHandle_t psocLineQueue = NULL;

// Dipanggil oleh task UART internal (bukan loop()!) tiap ada event RX
// (timeout singkat setelah byte terakhir suatu baris, atau FIFO penuh).
// HANYA kerja ringan & cepat di sini: baca byte yang tersedia, deteksi
// akhir baris ('\n'), catat timestamp PERSIS saat itu, taruh ke queue.
// Tidak ada parsing berat / broadcast WebSocket / Serial CDC di sini.
void onPsocReceive() {
  static char buf[PSOC_LINE_MAXLEN + 1];
  static int len = 0;

  while (psocSerial.available()) {
    char c = psocSerial.read();

    if (c == '\n') {
      unsigned long ts = millis(); // <-- titik pengambilan timestamp yang benar
      buf[len] = '\0';

      if (len > 0 && psocLineQueue != NULL) {
        PsocLineEvent ev;
        memcpy(ev.line, buf, len + 1);
        ev.ts = ts;
        // Non-blocking (timeout 0): kalau queue penuh (tidak akan
        // terjadi kecuali psocSerial mengirim SANGAT deras), baris ini
        // di-drop drpd nge-block task UART -- lebih aman drpd bikin
        // event lain ikut telat.
        xQueueSend(psocLineQueue, &ev, 0);
      }
      len = 0;
    } else if (c != '\r') {
      if (len < PSOC_LINE_MAXLEN) {
        buf[len++] = c;
      } else {
        len = 0; // baris kepanjangan/rusak, buang & mulai ulang
      }
    }
  }
}

// v4.2: broadcastState() (JSON lengkap) tidak lagi dipanggil langsung di
// dalam addLap()/addQualifyLap() -- supaya kalau ada beberapa lap
// tercatat dalam satu siklus drain queue, cuma perlu SATU broadcast di
// akhir, dan supaya proses jaringan tidak pernah ikut menunda pembacaan
// baris PSoC berikutnya.
bool pendingStateBroadcast = false;

// v4.2: entri Terminal (tiap "hit" mentah dari PSoC, termasuk yang
// difilter cooldown) dulu langsung di-broadcast satu-satu -- kalau
// transponder "nongkrong" dekat antena & ngirim banyak hit beruntun,
// itu jadi banyak panggilan webSocket.broadcastTXT() beruntun juga,
// dan tiap panggilan itu bisa blocking. Sekarang ditampung dulu, baru
// dikirim (di-batch) dari loop(), setelah semua baris yang lagi
// tersedia selesai diproses -- tampilan Terminal di browser cuma
// telat maksimal satu siklus loop(), tidak terlihat oleh mata.
#define TERMINAL_QUEUE_SIZE 24
struct PendingTerminalEntry {
  uint32_t idDecimal;
  int hits;
  int quality;
};
PendingTerminalEntry terminalBroadcastQueue[TERMINAL_QUEUE_SIZE];
int terminalBroadcastCount = 0;

void queueTerminalBroadcast(uint32_t idDecimal, int hits, int quality) {
  if (terminalBroadcastCount >= TERMINAL_QUEUE_SIZE) {
    // Antrian penuh (kasus ekstrem) -- geser buang entri paling lama.
    // Lebih baik kehilangan satu baris tampilan Terminal drpd numpuk.
    for (int i = 1; i < TERMINAL_QUEUE_SIZE; i++) {
      terminalBroadcastQueue[i - 1] = terminalBroadcastQueue[i];
    }
    terminalBroadcastCount--;
  }
  terminalBroadcastQueue[terminalBroadcastCount].idDecimal = idDecimal;
  terminalBroadcastQueue[terminalBroadcastCount].hits = hits;
  terminalBroadcastQueue[terminalBroadcastCount].quality = quality;
  terminalBroadcastCount++;
}

void flushTerminalBroadcastQueue() {
  if (terminalBroadcastCount == 0) return;

  if (webSocket.connectedClients() == 0) {
    terminalBroadcastCount = 0;
    return;
  }

  for (int i = 0; i < terminalBroadcastCount; i++) {
    String json = "{\"type\":\"terminalEntry\",\"idDecimal\":" +
                  String(terminalBroadcastQueue[i].idDecimal) +
                  ",\"hits\":" + String(terminalBroadcastQueue[i].hits) +
                  ",\"quality\":" + String(terminalBroadcastQueue[i].quality) + "}";
    webSocket.broadcastTXT(json);
  }
  terminalBroadcastCount = 0;
}

// SSID sekarang dibuat unik per device (pakai MAC address) supaya beberapa
// unit tidak bentrok nama WiFi kalau dipakai berdekatan. Password default
// dipakai kalau belum pernah diganti user lewat menu Settings; begitu
// diganti, disimpan ke /wifi.cfg dan dipakai terus setelah reboot.
String apSsid = "YurLaps Decoder";
String apPassword = "12345678";

unsigned long minLapInterval = 3000;
int lapTarget = 10;

// Mode race sekarang tetap (tidak ada pilihan/setting lagi): begitu race
// mulai (green flag), semua racer otomatis "In Race", dan lintasan
// PERTAMA yang kebaca sensor langsung dihitung Lap 1. Cooldown (minLapInterval)
// aktif dari detik green flag juga -- pembacaan sensor yang masuk sebelum
// cooldown lewat dianggap noise dan diabaikan. Ini paling aman untuk
// berbagai posisi sensor (termasuk kalau sensor ada di tengah barisan
// start), tanpa perlu user pusing pilih mode.

enum RaceState {
  IDLE,
  COUNTDOWN,
  RUNNING,
  STOPPED
};

RaceState raceState = IDLE;

unsigned long countdownStartMillis = 0;

// v4.7: dulu const tetap 5 detik -- sekarang bisa diatur dari UI
// (Settings > Race Countdown), disimpan ke flash lewat saveConfig().
int countdownStartNumber = 5;

// v4.7: delay TAMBAHAN setelah angka countdown habis (sampai "GO")
// sebelum horn beneran bunyi & race mulai -- fitur anti-anticipation
// (driver tidak bisa nebak persis kapan start kalau delay-nya random).
// Fixed = delay tetap (hornDelayFixedMs, default 0 = langsung seperti
// perilaku lama). Random = delay diundi ulang SETIAP kali Start ditekan,
// di rentang [hornDelayMinMs, hornDelayMaxMs].
bool hornRandomDelayEnabled = false;
unsigned long hornDelayFixedMs = 0;
unsigned long hornDelayMinMs = 5000;
unsigned long hornDelayMaxMs = 10000;

// Nilai AKTUAL yang dipakai buat race start yang SEDANG berjalan --
// diundi/ditentukan sekali di handleStart(), supaya tidak berubah-ubah
// selama satu race (kalau random, driver benar2 tidak tahu angkanya
// sampai horn beneran bunyi).
unsigned long countdownExtraDelayMs = 0;

unsigned long raceStartMillis = 0;
unsigned long raceElapsedBeforeStop = 0;

#define MAX_DRIVERS 100
#define MAX_RACERS 50
#define MAX_LAPS_PER_RACER 30
#define MAX_SAVED_RACES 100
#define MAX_TERMINAL_LOG 50

File uploadFile;

struct Driver {
  String name;
  uint32_t idDecimal;
  bool active;
};

// Log mentah 50 pembacaan sensor terakhir (buat "Terminal" di UI) --
// mencatat SEMUA pembacaan, terlepas dari itu kehitung lap atau tidak
// (kena cooldown, dsb). Ring buffer sederhana: terminalLogHead nunjuk ke
// slot berikutnya yang bakal ditimpa.
struct TerminalEntry {
  uint32_t idDecimal;
  int hits;
  int quality;
};

TerminalEntry terminalLog[MAX_TERMINAL_LOG];
int terminalLogCount = 0;
int terminalLogHead = 0;

struct Racer {
  String name;
  uint32_t idDecimal;
  int laps;
  bool hasStarted;
  bool finished;

  unsigned long lastReadMillis;
  unsigned long lastLapAchievedMillis;

  unsigned long lastLapTime;
  unsigned long bestLapTime;
  unsigned long totalLapTime;
  int lapTimeCount;

  unsigned long lapLogs[MAX_LAPS_PER_RACER];
};

// v4.2 fix compile: Arduino auto-generate prototype fungsi dan
// menyisipkannya SEBELUM baris kode "asli" pertama di file (tepat
// setelah blok #include). Di v4.1 ini aman karena tidak ada function
// definition sebelum `struct Racer`. Di v4.2, fungsi-fungsi baru
// (onPsocReceive, queueTerminalBroadcast, dll) ditaruh lebih awal di
// file -- itu jadi "function definition pertama", jadi titik sisip
// auto-prototype ikut maju ke atas struct Racer, dan prototype
// otomatis untuk getRacerStatus(Racer r) yang dihasilkan Arduino jadi
// invalid (Racer belum dikenal di titik itu). Fix: kasih prototype
// manual di sini (SETELAH struct Racer) -- begitu Arduino nemu
// prototype yang sudah ada persis begini, dia tidak generate ulang punya
// sendiri yang salah tempat.
String getRacerStatus(Racer r);

Driver drivers[MAX_DRIVERS];
Racer racers[MAX_RACERS];

int driverCount = 0;
int racerCount = 0;

uint32_t latestIdDecimal = 0;
int latestHits = 0;
int latestQuality = 0;

#define MAX_QUALIFIERS 50
#define MAX_QUALIFY_LAPS 30

// Semua struct (Driver, TerminalEntry, Racer, Qualifier) sengaja
// dikelompokkan di atas SINI -- sebelum fungsi apapun didefinisikan.
// Ini penting: Arduino IDE auto-generate prototype fungsi dan
// menyisipkannya tepat SEBELUM fungsi pertama di file. Kalau ada struct
// yang baru didefinisikan SETELAH fungsi pertama, tapi dipakai sebagai
// parameter fungsi lain di bawahnya, prototype otomatis itu jadi rusak
// (struct belum dikenal di titik sisipannya) -- persis bug yang sempat
// terjadi di getRacerStatus(Racer r). Taruh SEMUA struct di sini supaya
// aman dari masalah ini.
struct Qualifier {
  String name;
  uint32_t idDecimal;
  int laps;
  bool hasStarted;

  unsigned long lastReadMillis;
  unsigned long lastLapAchievedMillis;

  unsigned long lastLapTime;
  unsigned long bestLapTime;
  unsigned long totalLapTime;
  int lapTimeCount;

  unsigned long lapLogs[MAX_QUALIFY_LAPS];
};

unsigned long getRaceTime();
int getCountdownValue();
String buildRaceJson();
String buildFullDataJson();
String buildLiveDataJson();
void broadcastState();
void broadcastLiveState();
void addQualifyLap(uint32_t idDecimal, unsigned long ts);

uint32_t hexToUint32(String hex) {
  return strtoul(hex.c_str(), NULL, 16);
}

String escapeJson(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

unsigned long qualifyMinLapInterval = 3000;
int qualifyMaxLap = 10;
bool qualifySortByBestLap = true;

Qualifier qualifiers[MAX_QUALIFIERS];
int qualifierCount = 0;
bool qualifyingRunning = false;

void pushTerminalEntry(uint32_t idDecimal, int hits, int quality) {
  terminalLog[terminalLogHead].idDecimal = idDecimal;
  terminalLog[terminalLogHead].hits = hits;
  terminalLog[terminalLogHead].quality = quality;

  terminalLogHead = (terminalLogHead + 1) % MAX_TERMINAL_LOG;

  if (terminalLogCount < MAX_TERMINAL_LOG) {
    terminalLogCount++;
  }
}

// v4.2: broadcastTerminalEntry() lama sudah digantikan queueTerminalBroadcast()
// + flushTerminalBroadcastQueue() (lihat dekat deklarasi psocSerial di atas)
// -- broadcast per-hit yang blocking dipindah keluar dari jalur pembacaan
// UART, di-batch, dan dikirim belakangan dari loop().

void saveDrivers() {
  File file = FFat.open("/drivers.csv", "w");
  if (!file) {
    dbgPrintln("Failed to save drivers");
    return;
  }

  for (int i = 0; i < driverCount; i++) {
    file.print(drivers[i].name);
    file.print(",");
    file.print(drivers[i].idDecimal);
    file.print(",");
    file.println(drivers[i].active ? 1 : 0);
  }

  file.close();
  dbgPrintln("Drivers saved");
}

void saveConfig() {

  File file = FFat.open("/config.cfg", "w");

  if (!file) return;

  file.println(lapTarget);
  file.println(minLapInterval);
  file.println(countdownStartNumber);
  file.println(hornRandomDelayEnabled ? 1 : 0);
  file.println(hornDelayFixedMs);
  file.println(hornDelayMinMs);
  file.println(hornDelayMaxMs);

  file.close();
}

void loadConfig() {

  if (!FFat.exists("/config.cfg")) return;

  File file = FFat.open("/config.cfg", "r");

  if (!file) return;

  lapTarget = file.readStringUntil('\n').toInt();

  minLapInterval =
    file.readStringUntil('\n').toInt();

  // v4.7: field baru -- kalau file config LAMA (sebelum v4.7) yang cuma
  // punya 2 baris, readStringUntil() di sini bakal balikin string kosong
  // -> toInt() = 0 -> divalidasi & dikasih default di bawah, sama seperti
  // lapTarget/minLapInterval sudah lama begitu.
  int savedCountdown = file.readStringUntil('\n').toInt();
  int savedRandomFlag = file.readStringUntil('\n').toInt();
  long savedFixed = file.readStringUntil('\n').toInt();
  long savedMin = file.readStringUntil('\n').toInt();
  long savedMax = file.readStringUntil('\n').toInt();

  file.close();

  if (lapTarget < 1)
    lapTarget = 10;

  if (minLapInterval < 1000)
    minLapInterval = 3000;

  if (savedCountdown >= 1) countdownStartNumber = savedCountdown;
  hornRandomDelayEnabled = (savedRandomFlag == 1);
  if (savedFixed >= 0) hornDelayFixedMs = savedFixed;
  if (savedMin >= 0) hornDelayMinMs = savedMin;
  if (savedMax > 0 && savedMax >= (long)hornDelayMinMs) hornDelayMaxMs = savedMax;
}


// Bikin SSID unik per unit pakai 3 byte terakhir MAC address AP, supaya
// beberapa device tidak bentrok nama WiFi kalau dipakai berdekatan
// (misal beberapa lintasan dalam satu venue).
String buildUniqueSsid() {
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);

  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);

  return "YurLaps Decoder-" + String(suffix);
}

void loadWifiConfig() {
  if (!FFat.exists("/wifi.cfg")) return;

  File file = FFat.open("/wifi.cfg", "r");
  if (!file) return;

  String pw = file.readStringUntil('\n');
  pw.trim();
  file.close();

  // WPA2 minimal 8 karakter -- kalau file korup/kosong, biarkan default.
  if (pw.length() >= 8) {
    apPassword = pw;
  }
}

void saveWifiConfig() {
  File file = FFat.open("/wifi.cfg", "w");
  if (!file) return;

  file.println(apPassword);
  file.close();
}

void loadDrivers() {
  driverCount = 0;

  if (!FFat.exists("/drivers.csv")) {
    dbgPrintln("No drivers file");
    return;
  }

  File file = FFat.open("/drivers.csv", "r");
  if (!file) {
    dbgPrintln("Failed to open drivers file");
    return;
  }

  while (file.available() && driverCount < MAX_DRIVERS) {
    String row = file.readStringUntil('\n');
    row.trim();

    if (row.length() == 0) continue;

    int c1 = row.indexOf(',');
    int c2 = row.indexOf(',', c1 + 1);

    if (c1 < 0 || c2 < 0) continue;

    String name = row.substring(0, c1);
    uint32_t id = row.substring(c1 + 1, c2).toInt();
    bool active = row.substring(c2 + 1).toInt() == 1;

    if (name.length() > 0 && id > 0) {
      drivers[driverCount].name = name;
      drivers[driverCount].idDecimal = id;
      drivers[driverCount].active = active;
      driverCount++;
    }
  }

  file.close();

  dbgPrint("Loaded drivers: ");
  dbgPrintln(driverCount);
}

String raceStateText() {
  if (raceState == IDLE) return "IDLE";
  if (raceState == COUNTDOWN) return "COUNTDOWN";
  if (raceState == RUNNING) return "RUNNING";
  if (raceState == STOPPED) return "STOPPED";
  return "UNKNOWN";
}

String buildRaceJson() {

  String json = "{";

  json += "\"latestIdDecimal\":" +
          String(latestIdDecimal) + ",";

  json += "\"hits\":" +
          String(latestHits) + ",";

  json += "\"quality\":" +
          String(latestQuality) + ",";

  json += "\"raceState\":\"" +
          raceStateText() + "\",";

  json += "\"qualifyingRunning\":" +
          String(
            qualifyingRunning ?
            "true" : "false"
          );

  json += "}";

  return json;
}

unsigned long getRaceTime() {
  if (raceState == RUNNING) {
    return raceElapsedBeforeStop + (millis() - raceStartMillis);
  }
  return raceElapsedBeforeStop;
}

int getCountdownValue() {
  if (raceState != COUNTDOWN) return 0;

  unsigned long elapsed = millis() - countdownStartMillis;
  int remain = countdownStartNumber - (elapsed / 1000);

  if (remain < 0) remain = 0;
  return remain;
}

int findDriver(uint32_t idDecimal) {
  for (int i = 0; i < driverCount; i++) {
    if (drivers[i].idDecimal == idDecimal) return i;
  }
  return -1;
}

int findRacer(uint32_t idDecimal) {
  for (int i = 0; i < racerCount; i++) {
    if (racers[i].idDecimal == idDecimal) return i;
  }
  return -1;
}

int findQualifier(uint32_t idDecimal) {
  for (int i = 0; i < qualifierCount; i++) {
    if (qualifiers[i].idDecimal == idDecimal) return i;
  }
  return -1;
}



void rebuildRacersFromActiveDrivers() {
  racerCount = 0;

  for (int i = 0; i < driverCount; i++) {
    if (drivers[i].active && racerCount < MAX_RACERS) {
      racers[racerCount].name = drivers[i].name;
      racers[racerCount].idDecimal = drivers[i].idDecimal;
      racers[racerCount].laps = 0;
      racers[racerCount].hasStarted = false;
      racers[racerCount].finished = false;

      racers[racerCount].lastReadMillis = 0;
      racers[racerCount].lastLapAchievedMillis = 0;
      racers[racerCount].lastLapTime = 0;
      racers[racerCount].bestLapTime = 0;
      racers[racerCount].totalLapTime = 0;
      racers[racerCount].lapTimeCount = 0;
      for (int j = 0; j < MAX_LAPS_PER_RACER; j++) {
  racers[racerCount].lapLogs[j] = 0;
}

      racerCount++;
    }
  }
}

String getRacerStatus(Racer r) {
  if (r.finished || r.laps >= lapTarget) return "Finish";
  // laps==0 berarti transponder-nya GENUINELY tidak pernah kedeteksi sama
  // sekali sepanjang race (karena sejak mode "trigger pertama = Lap 1",
  // laps cuma bisa nol kalau memang belum pernah ada pembacaan valid).
  // Beda kondisi dari DNF (mulai balapan tapi tidak selesai).
  if (raceState == STOPPED && r.laps == 0) return "DNS";
  if (raceState == STOPPED && r.laps < lapTarget) return "DNF";
  if (raceState == RUNNING && r.hasStarted) return "In Race";
  return "Ready";
}

void sortLeaderboard() {
  for (int i = 0; i < racerCount - 1; i++) {
    for (int j = i + 1; j < racerCount; j++) {
      bool swapNeeded = false;

      if (racers[j].laps > racers[i].laps) {
        swapNeeded = true;
      } 
      else if (racers[j].laps == racers[i].laps) {
        if (racers[j].hasStarted && !racers[i].hasStarted) {
          swapNeeded = true;
        }
        else if (racers[j].lastLapAchievedMillis > 0 &&
                 racers[i].lastLapAchievedMillis > 0 &&
                 racers[j].lastLapAchievedMillis < racers[i].lastLapAchievedMillis) {
          swapNeeded = true;
        }
      }

      if (swapNeeded) {
        Racer temp = racers[i];
        racers[i] = racers[j];
        racers[j] = temp;
      }
    }
  }
}

void addLap(uint32_t idDecimal, unsigned long ts) {
  if (raceState != RUNNING) return;

  int index = findRacer(idDecimal);

  if (index < 0) return;

  if (racers[index].finished || racers[index].laps >= lapTarget) {
    racers[index].finished = true;
    return;
  }

  // v4.2: "now" adalah timestamp yang sudah diambil sedini mungkin (di
  // onPsocReceive(), saat baris ini baru selesai diterima dari UART) --
  // BUKAN millis() baru yang dipanggil di sini. Ini kunci fix akurasi:
  // tidak ada lagi penundaan jaringan/CDC yang numpang masuk ke angka ini.
  unsigned long now = ts;

  // Catatan: dulu di sini ada blok "if (!hasStarted) { arm, return; }"
  // (arming pass pertama tanpa hitung lap) -- itu sisa desain versi lama.
  // Sejak race disederhanakan jadi "trigger pertama = Lap 1" (hasStarted
  // di-set true untuk SEMUA racer tepat saat green flag, lihat
  // updateRaceState()), blok itu jadi dead code yang tidak akan pernah
  // tereksekusi lagi -- sudah dihapus supaya tidak membingungkan.

  if (now - racers[index].lastReadMillis >= minLapInterval) {
    unsigned long lapDuration = now - racers[index].lastLapAchievedMillis;

    racers[index].laps++;
racers[index].lastReadMillis = now;
racers[index].lastLapAchievedMillis = now;

racers[index].lastLapTime = lapDuration;
racers[index].totalLapTime += lapDuration;

if (racers[index].lapTimeCount < MAX_LAPS_PER_RACER) {
  racers[index].lapLogs[racers[index].lapTimeCount] = lapDuration;
  racers[index].lapTimeCount++;
}

    if (racers[index].bestLapTime == 0 || lapDuration < racers[index].bestLapTime) {
      racers[index].bestLapTime = lapDuration;
    }

    if (racers[index].laps >= lapTarget) {
      racers[index].finished = true;
    }

    // PENTING: sort DULU baru broadcast -- sebelumnya urutannya kebalik
    // (broadcast lalu sort di akhir fungsi), jadi leaderboard yang
    // dikirim ke browser tiap ada lap baru itu posisi LAMA (belum
    // memperhitungkan lap yang baru saja tercatat). Ini yang bikin posisi
    // di leaderboard & announce posisi kadang kelihatan salah/telat satu
    // langkah -- baru "benar" di broadcast berikutnya.
    sortLeaderboard();

    // v4.2: dulu broadcastState() dipanggil LANGSUNG di sini (blocking,
    // network). Sekarang cuma pasang flag -- broadcast beneran dikirim
    // belakangan dari loop(), setelah SEMUA baris PSoC yang lagi
    // tersedia selesai diproses. Efeknya: kalau ada beberapa lap
    // tercatat berdekatan (mis. 2 racer crossing hampir bersamaan),
    // cukup satu broadcast yang mencakup semuanya, dan yang lebih
    // penting, timestamp lap racer berikutnya tidak pernah menunggu
    // proses kirim WebSocket punya racer sebelumnya.
    pendingStateBroadcast = true;
  }
}

void processPsocLine(String raw, unsigned long ts) {
  raw.trim();
  if (raw.length() == 0) return;

  int dash = raw.indexOf('-');
  if (dash < 0) return;

  String left = raw.substring(0, dash);
  String right = raw.substring(dash + 1);

  left.trim();
  right.trim();

  if (left.length() < 14) return;
  if (right.length() < 10) return;

  String idHex = left.substring(0, 6);
  String hitsHex = right.substring(2, 4);
  String qualityHex = right.substring(4, 6);

  idHex.toUpperCase();

  uint32_t transponderId = hexToUint32(idHex);
  int hits = strtoul(hitsHex.c_str(), NULL, 16);
  int quality = strtoul(qualityHex.c_str(), NULL, 16);

  latestIdDecimal = transponderId;
  latestHits = hits;
  latestQuality = quality;

  // v4.2: catat lap DULUAN, pakai timestamp yang sudah diambil di
  // onPsocReceive() -- SEBELUM melakukan apapun yang menyentuh jaringan
  // (WebSocket) atau CDC (Serial). Ini yang tadinya jadi sumber utama
  // ketidakakuratan: broadcastTerminalEntry() versi v4.1 (blocking) ada
  // di jalur ini SEBELUM timestamp lap diambil.
  addLap(transponderId, ts);
  addQualifyLap(transponderId, ts);

  // Baru sekarang hal-hal non-timing-critical: catat & antre entri
  // Terminal (dikirim belakangan, di-batch, lihat flushTerminalBroadcastQueue()),
  // dan debug print (aman dari blocking CDC lewat dbgPrint*()).
  pushTerminalEntry(transponderId, hits, quality);
  queueTerminalBroadcast(transponderId, hits, quality);

  dbgPrint("ID: ");
  dbgPrint(transponderId);
  dbgPrint(" | Quality: ");
  dbgPrint(quality);
  dbgPrint(" | Hits: ");
  dbgPrintln(hits);
}

void updateRaceState() {
  if (raceState == COUNTDOWN) {
    unsigned long elapsed = millis() - countdownStartMillis;

    // v4.7: total waktu tunggu = bagian angka (countdownStartNumber
    // detik, yang kelihatan/kedengeran di layar/suara) + delay tambahan
    // SETELAH itu (countdownExtraDelayMs, fix atau hasil random -- lihat
    // handleStart()). Selama delay tambahan ini, getCountdownValue()
    // sudah balik 0 (tampilan "GO") tapi race BELUM benar-benar mulai --
    // horn baru bunyi persis di titik totalNeeded ini tercapai.
    unsigned long totalNeeded = (unsigned long)countdownStartNumber * 1000UL + countdownExtraDelayMs;

    if (elapsed >= totalNeeded) {
      raceState = RUNNING;
      raceStartMillis = millis();
      raceElapsedBeforeStop = 0;

      // Begitu green flag, semua racer otomatis "In Race", dan lintasan
      // PERTAMA yang kebaca sensor langsung dihitung Lap 1 (bukan cuma
      // arming). Cooldown aktif dari detik ini juga -- pembacaan sensor
      // yang masuk sebelum cooldown lewat dianggap noise dan diabaikan.
      // Ini aman dipakai untuk berbagai posisi sensor fisik di lintasan.
      for (int i = 0; i < racerCount; i++) {
        racers[i].hasStarted = true;
        racers[i].lastLapAchievedMillis = raceStartMillis;
        racers[i].lastReadMillis = raceStartMillis;
      }

      broadcastState(); // dorong instan momen "GO!", jangan nunggu heartbeat
    }
  }
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", MAIN_page);
}

String buildFullDataJson() {
  String json;
  // Dinaikkan dari 4KB ke 40KB -- di kapasitas maksimal (50 racer x 30 lap
  // + 50 qualifier x 30 lap + 100 driver) payload ini bisa ~37KB. Reserve
  // kekecilan artinya realokasi berkali-kali tiap generate = boros CPU +
  // risiko fragmentasi heap. 40KB dialokasikan SEKALI di awal fungsi lalu
  // dibebaskan lagi begitu fungsi return (bukan dipegang terus), jadi tidak
  // "mengunci" RAM permanen -- cuma numpang lewat saat generate saja.
  json.reserve(40960);
  json = "{\"type\":\"full\",";

  json += "\"firmwareVersion\":\"" FIRMWARE_VERSION "\",";
  json += "\"latestIdDecimal\":" + String(latestIdDecimal) + ",";
  json += "\"hits\":" + String(latestHits) + ",";
  json += "\"quality\":" + String(latestQuality) + ",";
  json += "\"lapTarget\":" + String(lapTarget) + ",";
  json += "\"minLapInterval\":" + String(minLapInterval / 1000) + ",";
  json += "\"countdownStartNumber\":" + String(countdownStartNumber) + ",";
  json += "\"hornRandomDelayEnabled\":" + String(hornRandomDelayEnabled ? "true" : "false") + ",";
  json += "\"hornDelayFixedMs\":" + String(hornDelayFixedMs) + ",";
  json += "\"hornDelayMinMs\":" + String(hornDelayMinMs) + ",";
  json += "\"hornDelayMaxMs\":" + String(hornDelayMaxMs) + ",";
  json += "\"wifiSsid\":\"" + escapeJson(apSsid) + "\",";
  json += "\"raceState\":\"" + raceStateText() + "\",";
  json += "\"countdown\":" + String(getCountdownValue()) + ",";
  json += "\"raceTime\":" + String(getRaceTime()) + ",";

  json += "\"terminalLog\":[";
  for (int n = 0; n < terminalLogCount; n++) {
    // Ambil dari yang PALING BARU ke paling lama, biar frontend tinggal
    // prepend tanpa perlu balik urutan lagi.
    int idx = (terminalLogHead - 1 - n + MAX_TERMINAL_LOG) % MAX_TERMINAL_LOG;

    if (n > 0) json += ",";

    json += "{\"idDecimal\":" + String(terminalLog[idx].idDecimal) +
            ",\"hits\":" + String(terminalLog[idx].hits) +
            ",\"quality\":" + String(terminalLog[idx].quality) + "}";
  }
  json += "],";

  json += "\"racers\":[";
  for (int i = 0; i < racerCount; i++) {
    unsigned long avgLap = 0;

    if (racers[i].lapTimeCount > 0) {
      avgLap = racers[i].totalLapTime / racers[i].lapTimeCount;
    }

    if (i > 0) json += ",";

    json += "{";
    json += "\"name\":\"" + escapeJson(racers[i].name) + "\",";
    json += "\"idDecimal\":" + String(racers[i].idDecimal) + ",";
    json += "\"laps\":" + String(racers[i].laps) + ",";
    json += "\"bestLapTime\":" + String(racers[i].bestLapTime) + ",";
    json += "\"lastLapTime\":" + String(racers[i].lastLapTime) + ",";
    json += "\"averageLapTime\":" + String(avgLap) + ",";
    json += "\"status\":\"" + getRacerStatus(racers[i]) + "\",";
json += "\"lapLogs\":[";

for (int l = 0; l < racers[i].lapTimeCount; l++) {
  if (l > 0) json += ",";
  json += String(racers[i].lapLogs[l]);
}

json += "]";
    json += "}";
  }

  json += "],";

  json += "\"drivers\":[";
  for (int i = 0; i < driverCount; i++) {
    if (i > 0) json += ",";

    json += "{";
    json += "\"name\":\"" + escapeJson(drivers[i].name) + "\",";
    json += "\"idDecimal\":" + String(drivers[i].idDecimal) + ",";
    json += "\"active\":" + String(drivers[i].active ? "true" : "false");
    json += "}";
  }

  json += "],";

json += "\"qualifyingRunning\":" + String(qualifyingRunning ? "true" : "false") + ",";
json += "\"qualifyMaxLap\":" + String(qualifyMaxLap) + ",";
json += "\"qualifyCooldown\":" + String(qualifyMinLapInterval / 1000) + ",";
json += "\"qualifySort\":\"" + String(qualifySortByBestLap ? "best" : "lap") + "\",";

json += "\"qualifiers\":[";

for (int i = 0; i < qualifierCount; i++) {
  unsigned long avgLap = 0;

  if (qualifiers[i].lapTimeCount > 0) {
    avgLap = qualifiers[i].totalLapTime / qualifiers[i].lapTimeCount;
  }

  if (i > 0) json += ",";

  json += "{";
  json += "\"name\":\"" + escapeJson(qualifiers[i].name) + "\",";
  json += "\"idDecimal\":" + String(qualifiers[i].idDecimal) + ",";
  json += "\"laps\":" + String(qualifiers[i].laps) + ",";
  json += "\"bestLapTime\":" + String(qualifiers[i].bestLapTime) + ",";
  json += "\"lastLapTime\":" + String(qualifiers[i].lastLapTime) + ",";
  json += "\"averageLapTime\":" + String(avgLap) + ",";

  json += "\"lapLogs\":[";

  for (int l = 0; l < qualifiers[i].lapTimeCount; l++) {
    if (l > 0) json += ",";
    json += String(qualifiers[i].lapLogs[l]);
  }

  json += "]";
  json += "}";
}

json += "]}";

  return json;
}

// Versi RINGAN, khusus buat heartbeat 200ms. Cuma berisi hal-hal yang
// benar-benar berubah tiap saat (jam/countdown) + ringkasan status per
// racer/qualifier (TANPA lapLogs array). Di kapasitas maksimal, payload
// ini ~2-3KB dibanding ~37KB versi lengkap -- ini yang bikin heartbeat
// 200ms tetap ringan buat C3 walau driver/racer penuh.
String buildLiveDataJson() {
  String json;
  json.reserve(4096);
  json = "{\"type\":\"live\",";

  json += "\"latestIdDecimal\":" + String(latestIdDecimal) + ",";
  json += "\"hits\":" + String(latestHits) + ",";
  json += "\"quality\":" + String(latestQuality) + ",";
  json += "\"lapTarget\":" + String(lapTarget) + ",";
  json += "\"raceState\":\"" + raceStateText() + "\",";
  json += "\"countdown\":" + String(getCountdownValue()) + ",";
  json += "\"raceTime\":" + String(getRaceTime()) + ",";

  json += "\"racersLive\":[";
  for (int i = 0; i < racerCount; i++) {
    unsigned long avgLap = 0;

    if (racers[i].lapTimeCount > 0) {
      avgLap = racers[i].totalLapTime / racers[i].lapTimeCount;
    }

    if (i > 0) json += ",";

    json += "{";
    json += "\"idDecimal\":" + String(racers[i].idDecimal) + ",";
    json += "\"laps\":" + String(racers[i].laps) + ",";
    json += "\"bestLapTime\":" + String(racers[i].bestLapTime) + ",";
    json += "\"lastLapTime\":" + String(racers[i].lastLapTime) + ",";
    json += "\"averageLapTime\":" + String(avgLap) + ",";
    json += "\"status\":\"" + getRacerStatus(racers[i]) + "\"";
    json += "}";
  }
  json += "],";

  json += "\"qualifiersLive\":[";
  for (int i = 0; i < qualifierCount; i++) {
    unsigned long avgLap = 0;

    if (qualifiers[i].lapTimeCount > 0) {
      avgLap = qualifiers[i].totalLapTime / qualifiers[i].lapTimeCount;
    }

    if (i > 0) json += ",";

    json += "{";
    json += "\"idDecimal\":" + String(qualifiers[i].idDecimal) + ",";
    json += "\"laps\":" + String(qualifiers[i].laps) + ",";
    json += "\"bestLapTime\":" + String(qualifiers[i].bestLapTime) + ",";
    json += "\"lastLapTime\":" + String(qualifiers[i].lastLapTime) + ",";
    json += "\"averageLapTime\":" + String(avgLap);
    json += "}";
  }
  json += "]}";

  return json;
}

void handleData() {
  server.send(200, "application/json", buildFullDataJson());
}

// Broadcast LENGKAP (dengan lapLogs) -- dipanggil pas ada event nyata:
// lap tercatat, race/qualify start-stop-reset, driver/setting berubah.
// Kalau tidak ada client sama sekali, skip total -- tidak buang CPU/heap
// buat generate JSON yang toh tidak akan dikirim kemana-mana.
void broadcastState() {
  if (webSocket.connectedClients() == 0) return;
  String json = buildFullDataJson();
  webSocket.broadcastTXT(json);
}

// Broadcast RINGAN -- dipanggil tiap heartbeat 200ms saat race/qualifying
// aktif. Jauh lebih murah daripada broadcastState() di kapasitas besar.
void broadcastLiveState() {
  if (webSocket.connectedClients() == 0) return;
  String json = buildLiveDataJson();
  webSocket.broadcastTXT(json);
}

void handleSettings() {
  bool changed = false;

  if (server.hasArg("target")) {
    int target = server.arg("target").toInt();

    if (target > 0) {
      lapTarget = target;
      changed = true;
    }
  }

  if (server.hasArg("cooldown")) {
    unsigned long cooldownSecond = server.arg("cooldown").toInt();

    if (cooldownSecond >= 1) {
      minLapInterval = cooldownSecond * 1000;
      changed = true;
    }
  }

  if (changed) {
    saveConfig();
    broadcastState();

    dbgPrint("Config saved | Lap Target: ");
    dbgPrint(lapTarget);
    dbgPrint(" | Cooldown ms: ");
    dbgPrintln(minLapInterval);
  }

  server.send(200, "text/plain", "OK");
}

// v4.7: pengaturan Countdown Race (mulai dari angka berapa) & Horn Delay
// (fix/random) -- dipisah dari handleSettings() supaya endpoint-nya
// jelas namanya, tapi pola validasi & penyimpanannya sama persis.
void handleCountdownSettings() {
  bool changed = false;

  if (server.hasArg("start")) {
    int start = server.arg("start").toInt();
    if (start >= 1 && start <= 60) {
      countdownStartNumber = start;
      changed = true;
    }
  }

  if (server.hasArg("mode")) {
    hornRandomDelayEnabled = (server.arg("mode") == "random");
    changed = true;
  }

  if (server.hasArg("fixedMs")) {
    long v = server.arg("fixedMs").toInt();
    if (v >= 0) {
      hornDelayFixedMs = v;
      changed = true;
    }
  }

  if (server.hasArg("minMs") && server.hasArg("maxMs")) {
    long vMin = server.arg("minMs").toInt();
    long vMax = server.arg("maxMs").toInt();

    // Validasi rentang -- kalau operator kebalik nulis min > max, tolak
    // daripada diam-diam "membetulkan" jadi nilai yang tidak mereka minta.
    if (vMin >= 0 && vMax > vMin) {
      hornDelayMinMs = vMin;
      hornDelayMaxMs = vMax;
      changed = true;
    } else {
      server.send(400, "text/plain", "INVALID_RANGE");
      return;
    }
  }

  if (changed) {
    saveConfig();
    broadcastState();
  }

  server.send(200, "text/plain", "OK");
}

void handleAddDriver() {
  if (!server.hasArg("name") || !server.hasArg("id")) {
    server.send(400, "text/plain", "BAD_REQUEST");
    return;
  }

  if (driverCount >= MAX_DRIVERS) {
    server.send(400, "text/plain", "DRIVER_FULL");
    return;
  }

  String name = server.arg("name");
  uint32_t id = server.arg("id").toInt();

  name.trim();

  if (name.length() == 0 || id == 0) {
    server.send(400, "text/plain", "INVALID_DATA");
    return;
  }

  int existing = findDriver(id);

  if (existing >= 0) {
    drivers[existing].name = name;
  } else {
    drivers[driverCount].name = name;
    drivers[driverCount].idDecimal = id;
    drivers[driverCount].active = false;
    driverCount++;
  }

  saveDrivers();

  if (raceState == IDLE || raceState == STOPPED) {
    rebuildRacersFromActiveDrivers();
  }

  broadcastState();
  server.send(200, "text/plain", "OK");
}

void handleToggleDriver() {
  if (!server.hasArg("index")) {
    server.send(400, "text/plain", "BAD_REQUEST");
    return;
  }

  int index = server.arg("index").toInt();

  if (index < 0 || index >= driverCount) {
    server.send(400, "text/plain", "INVALID_INDEX");
    return;
  }

  drivers[index].active = !drivers[index].active;

  saveDrivers();

  if (raceState == IDLE || raceState == STOPPED) {
    rebuildRacersFromActiveDrivers();
  }

  broadcastState();
  server.send(200, "text/plain", "OK");
}

void handleDeleteDriver() {
  if (!server.hasArg("index")) {
    server.send(400, "text/plain", "BAD_REQUEST");
    return;
  }

  int index = server.arg("index").toInt();

  if (index < 0 || index >= driverCount) {
    server.send(400, "text/plain", "INVALID_INDEX");
    return;
  }

  for (int i = index; i < driverCount - 1; i++) {
    drivers[i] = drivers[i + 1];
  }

  driverCount--;

  saveDrivers();

  if (raceState == IDLE || raceState == STOPPED) {
    rebuildRacersFromActiveDrivers();
  }

  broadcastState();
  server.send(200, "text/plain", "OK");
}

void handleStart() {
  if (raceState == RUNNING || raceState == COUNTDOWN) {
    server.send(200, "text/plain", "ALREADY_RUNNING");
    return;
  }

  // v4.2.2: Race & Qualifying tidak boleh jalan bersamaan (keduanya
  // sama-sama pakai psocSerial/addLap-addQualifyLap dari baris PSoC yang
  // sama -- kalau dibiarkan jalan bareng, satu transponder yang lewat
  // bakal kecatat di DUA sistem sekaligus, membingungkan). Simetris
  // dengan pengecekan RACE_RUNNING di handleQualifyStart() di bawah.
  if (qualifyingRunning) {
    server.send(400, "text/plain", "QUALIFY_RUNNING");
    return;
  }

  rebuildRacersFromActiveDrivers();

  raceState = COUNTDOWN;
  countdownStartMillis = millis();
  raceElapsedBeforeStop = 0;

  // v4.7: tentukan delay tambahan buat race start KALI INI -- kalau
  // random, diundi ulang setiap kali Start ditekan (jadi tidak bisa
  // dihafal drivernya), kalau fix, ya nilai tetap yang di-setting.
  if (hornRandomDelayEnabled && hornDelayMaxMs > hornDelayMinMs) {
    countdownExtraDelayMs = hornDelayMinMs +
      (unsigned long)random(0, (long)(hornDelayMaxMs - hornDelayMinMs + 1));
  } else {
    countdownExtraDelayMs = hornDelayFixedMs;
  }

  broadcastState();
  server.send(200, "text/plain", "COUNTDOWN_STARTED");
}

void handleStop() {
  if (raceState == RUNNING) {
    raceElapsedBeforeStop += millis() - raceStartMillis;
    raceState = STOPPED;
  } 
  else if (raceState == COUNTDOWN) {
    raceState = STOPPED;
  }

  broadcastState();
  server.send(200, "text/plain", "STOPPED");
}

void handleReset() {
  rebuildRacersFromActiveDrivers();

  latestIdDecimal = 0;
  latestHits = 0;
  latestQuality = 0;

  raceState = IDLE;
  raceElapsedBeforeStop = 0;
  raceStartMillis = 0;
  countdownStartMillis = 0;

  broadcastState();
  server.send(200, "text/plain", "RESET");
}

void saveRaceToFile(String raceName, String startDateTime) {
  raceName.trim();

  if (raceName.length() == 0) {
    dbgPrintln("Race name empty");
    return;
  }

  if (!FFat.exists("/races")) {
    FFat.mkdir("/races");
  }

  String fileName = "/races/race_" + String(millis()) + ".json";
  File indexFile = FFat.open("/race_index.csv", "a");
if (indexFile) {
  String safeName = raceName;
  safeName.replace("|", " ");

  String safeDate = startDateTime;
  safeDate.replace("|", " ");

  indexFile.print(fileName);
  indexFile.print("|");
  indexFile.print(safeName);
  indexFile.print("|");
  indexFile.println(safeDate);
  indexFile.close();
}

  File file = FFat.open(fileName, "w");

  if (!file) {
    dbgPrintln("Failed to create race file");
    return;
  }

  file.println("{");
  file.println("\"raceName\":\"" + escapeJson(raceName) + "\",");
  file.println("\"startDateTime\":\"" + escapeJson(startDateTime) + "\",");
  file.println("\"startMillis\":" + String(raceStartMillis) + ",");
  file.println("\"raceTime\":" + String(getRaceTime()) + ",");
  file.println("\"lapTarget\":" + String(lapTarget) + ",");
  file.println("\"racers\":[");

  for (int i = 0; i < racerCount; i++) {
    unsigned long avgLap = 0;

    if (racers[i].lapTimeCount > 0) {
      avgLap = racers[i].totalLapTime / racers[i].lapTimeCount;
    }

    if (i > 0) file.println(",");

    file.print("{");
    file.print("\"rank\":");
    file.print(i + 1);
    file.print(",");

    file.print("\"name\":\"");
    file.print(escapeJson(racers[i].name));
    file.print("\",");

    file.print("\"idDecimal\":");
    file.print(racers[i].idDecimal);
    file.print(",");

    file.print("\"laps\":");
    file.print(racers[i].laps);
    file.print(",");

    file.print("\"bestLapTime\":");
    file.print(racers[i].bestLapTime);
    file.print(",");

    file.print("\"lastLapTime\":");
    file.print(racers[i].lastLapTime);
    file.print(",");

    file.print("\"averageLapTime\":");
    file.print(avgLap);
    file.print(",");

    file.print("\"status\":\"");
    file.print(getRacerStatus(racers[i]));
    file.print("\",");

    file.print("\"lapLogs\":[");

    for (int l = 0; l < racers[i].lapTimeCount; l++) {
      if (l > 0) file.print(",");
      file.print(racers[i].lapLogs[l]);
    }

    file.print("]");
    file.print("}");
  }

  file.println("]");
  file.println("}");

  file.close();

  dbgPrint("Race saved: ");
  dbgPrintln(fileName);
  cleanupOldRaceHistory();
}

void handleHistory() {
  String rows[MAX_SAVED_RACES];
  int count = 0;

  if (FFat.exists("/race_index.csv")) {
    File file = FFat.open("/race_index.csv", "r");

    while (file && file.available() && count < MAX_SAVED_RACES) {
      String row = file.readStringUntil('\n');
      row.trim();

      if (row.length() > 0) {
        rows[count++] = row;
      }
    }

    if (file) file.close();
  }

  String json = "{\"history\":[";

  bool first = true;

  for (int i = count - 1; i >= 0; i--) {
    int p1 = rows[i].indexOf('|');
    int p2 = rows[i].indexOf('|', p1 + 1);

    if (p1 < 0 || p2 < 0) continue;

    String fileName = rows[i].substring(0, p1);
    String raceName = rows[i].substring(p1 + 1, p2);
    String dateTime = rows[i].substring(p2 + 1);

    if (!first) json += ",";
    first = false;

    json += "{";
    json += "\"file\":\"" + escapeJson(fileName) + "\",";
    json += "\"name\":\"" + escapeJson(raceName) + "\",";
    json += "\"datetime\":\"" + escapeJson(dateTime) + "\"";
    json += "}";
  }

  json += "]}";

  server.send(200, "application/json", json);
}


void handleRaceDetail() {
  if (!server.hasArg("file")) {
    server.send(400, "application/json", "{\"error\":\"NO_FILE\"}");
    return;
  }

  String fileName = server.arg("file");

  if (!fileName.startsWith("/races/")) {
    server.send(400, "application/json", "{\"error\":\"INVALID_FILE\"}");
    return;
  }

  if (!FFat.exists(fileName)) {
    server.send(404, "application/json", "{\"error\":\"FILE_NOT_FOUND\"}");
    return;
  }

  File file = FFat.open(fileName, "r");

  if (!file) {
    server.send(500, "application/json", "{\"error\":\"OPEN_FAILED\"}");
    return;
  }

  server.streamFile(file, "application/json");
  file.close();
}


void cleanupOldRaceHistory() {
  if (!FFat.exists("/race_index.csv")) return;

  File file = FFat.open("/race_index.csv", "r");
  if (!file) return;

  String rows[MAX_SAVED_RACES + 50];
  int count = 0;

  while (file.available() && count < MAX_SAVED_RACES + 20) {
    String row = file.readStringUntil('\n');
    row.trim();

    if (row.length() > 0) {
      rows[count++] = row;
    }
  }

  file.close();

  if (count <= MAX_SAVED_RACES) return;

  int removeCount = count - MAX_SAVED_RACES;

  for (int i = 0; i < removeCount; i++) {
    int p1 = rows[i].indexOf('|');

    if (p1 > 0) {
      String oldFile = rows[i].substring(0, p1);

      if (FFat.exists(oldFile)) {
        FFat.remove(oldFile);
        dbgPrint("Deleted old race: ");
        dbgPrintln(oldFile);
      }
    }
  }

  File newIndex = FFat.open("/race_index.csv", "w");
  if (!newIndex) return;

  for (int i = removeCount; i < count; i++) {
    newIndex.println(rows[i]);
  }

  newIndex.close();
}

// Sama persis pola-nya dengan cleanupOldRaceHistory(), tapi untuk history
// qualifying. Sebelumnya tidak ada fungsi ini sama sekali, jadi
// /qualify_index.csv dan folder /qualify/ tumbuh tanpa batas -> flash bisa
// penuh kalau dipakai rutin dalam jangka panjang.
void cleanupOldQualifyHistory() {
  if (!FFat.exists("/qualify_index.csv")) return;

  File file = FFat.open("/qualify_index.csv", "r");
  if (!file) return;

  String rows[MAX_SAVED_RACES + 50];
  int count = 0;

  while (file.available() && count < MAX_SAVED_RACES + 20) {
    String row = file.readStringUntil('\n');
    row.trim();

    if (row.length() > 0) {
      rows[count++] = row;
    }
  }

  file.close();

  if (count <= MAX_SAVED_RACES) return;

  int removeCount = count - MAX_SAVED_RACES;

  for (int i = 0; i < removeCount; i++) {
    int p1 = rows[i].indexOf('|');

    if (p1 > 0) {
      String oldFile = rows[i].substring(0, p1);

      if (FFat.exists(oldFile)) {
        FFat.remove(oldFile);
        dbgPrint("Deleted old qualify: ");
        dbgPrintln(oldFile);
      }
    }
  }

  File newIndex = FFat.open("/qualify_index.csv", "w");
  if (!newIndex) return;

  for (int i = removeCount; i < count; i++) {
    newIndex.println(rows[i]);
  }

  newIndex.close();
}

void handleHornUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (!FFat.exists("/sounds")) {
      FFat.mkdir("/sounds");
    }

    uploadFile = FFat.open("/sounds/start_horn.mp3", "w");

    dbgPrintln("Horn upload start");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }

    dbgPrint("Horn upload finished, size: ");
    dbgPrintln(upload.totalSize);
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    // Sebelumnya state ini tidak ditangani sama sekali -> kalau upload
    // dibatalkan di tengah jalan (koneksi putus/browser ditutup), file
    // handle tidak pernah ditutup dan upload berikutnya bisa gagal sampai
    // device di-reboot. Sekarang file ditutup + file setengah jadi dihapus
    // supaya tidak menyisakan mp3 rusak/korup.
    if (uploadFile) {
      uploadFile.close();
    }

    if (FFat.exists("/sounds/start_horn.mp3")) {
      FFat.remove("/sounds/start_horn.mp3");
    }

    dbgPrintln("Horn upload aborted, cleaned up");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      dbgPrintf("WS client %u connected\n", num);
      // Kirim state lengkap sekali begitu client baru connect, supaya dia
      // langsung dapat data terkini tanpa nunggu event/heartbeat berikutnya.
      {
        String json = buildFullDataJson();
        webSocket.sendTXT(num, json);
      }
      break;

    case WStype_DISCONNECTED:
      dbgPrintf("WS client %u disconnected\n", num);
      break;

    default:
      break;
  }
}

// --- WiFi password ---
void handleChangeWifiPassword() {
  if (!server.hasArg("password")) {
    server.send(400, "text/plain", "BAD_REQUEST");
    return;
  }

  String newPassword = server.arg("password");

  // WPA2 mewajibkan minimal 8 karakter, di bawah itu softAP akan gagal start.
  if (newPassword.length() < 8) {
    server.send(400, "text/plain", "PASSWORD_TOO_SHORT");
    return;
  }

  apPassword = newPassword;
  saveWifiConfig();

  server.send(200, "text/plain", "PASSWORD_CHANGED_REBOOTING");

  // Restart supaya WiFi AP benar-benar restart bersih dengan password baru.
  // Semua client yang lagi connect otomatis kepental & harus reconnect
  // pakai password baru -- ini memang konsekuensi wajar ganti password WiFi.
  delay(500);
  ESP.restart();
}

// --- Factory reset ---
void handleFactoryReset() {
  // Wajib ada confirm=YES supaya tidak ke-trigger tidak sengaja (misal
  // prefetch browser atau klik salah). Konfirmasi utama tetap di sisi UI
  // (dialog konfirmasi sebelum request ini dikirim).
  if (!server.hasArg("confirm") || server.arg("confirm") != "YES") {
    server.send(400, "text/plain", "CONFIRMATION_REQUIRED");
    return;
  }

  server.send(200, "text/plain", "FACTORY_RESET_REBOOTING");

  delay(500);
  FFat.format();
  ESP.restart();
}

// --- Diagnostics (dipakai menu Settings > Device Diagnostics) ---
// Dipakai untuk memantau kesehatan memori device dari jarak jauh lewat web
// UI, tanpa perlu colok USB + buka Serial Monitor. getMinFreeHeap() paling
// penting: itu titik BALIK TERENDAH heap sejak boot terakhir -- kalau
// angka ini terus turun/rendah padahal freeHeap saat ini kelihatan normal,
// itu tanda ada momen heap nyaris habis (indikasi fragmentasi/kebocoran),
// walau device sempat "pulih" lagi setelahnya.
void handleDiagnostics() {
  String json = "{";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"minFreeHeap\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"maxAllocHeap\":" + String(ESP.getMaxAllocHeap()) + ",";
  json += "\"uptimeSec\":" + String(millis() / 1000) + ",";
  json += "\"wsClients\":" + String(webSocket.connectedClients());
  json += "}";
  server.send(200, "application/json", json);
}

// --- OTA firmware update ---
void handleOtaUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    dbgPrintf("OTA update start: %s\n", upload.filename.c_str());

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      dbgPrintf("OTA update success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    dbgPrintln("OTA update aborted");
  }
}


void sortQualifiers() {
  for (int i = 0; i < qualifierCount - 1; i++) {
    for (int j = i + 1; j < qualifierCount; j++) {
      bool swapNeeded = false;

      if (qualifySortByBestLap) {
        // Mode "Best Lap": murni waktu tercepat di atas, terlepas dari
        // jumlah lap.
        if (qualifiers[j].bestLapTime > 0 &&
            (qualifiers[i].bestLapTime == 0 ||
             qualifiers[j].bestLapTime < qualifiers[i].bestLapTime)) {
          swapNeeded = true;
        }
      } else {
        // Mode "Lap Count": jumlah lap terbanyak di atas. Kalau SERI
        // jumlah lap, baru dibandingkan siapa yang waktu tercepatnya
        // lebih kecil (lebih baik). Sebelumnya tie-break ini tidak ada
        // sama sekali -- dua qualifier dengan lap sama urutannya asal
        // (posisi array lama), bukan berdasarkan performa.
        if (qualifiers[j].laps > qualifiers[i].laps) {
          swapNeeded = true;
        } else if (qualifiers[j].laps == qualifiers[i].laps) {
          if (qualifiers[j].bestLapTime > 0 &&
              (qualifiers[i].bestLapTime == 0 ||
               qualifiers[j].bestLapTime < qualifiers[i].bestLapTime)) {
            swapNeeded = true;
          }
        }
      }

      if (swapNeeded) {
        Qualifier temp = qualifiers[i];
        qualifiers[i] = qualifiers[j];
        qualifiers[j] = temp;
      }
    }
  }
}

void addQualifyLap(uint32_t idDecimal, unsigned long ts) {
  if (!qualifyingRunning) return;

  int driverIndex = findDriver(idDecimal);
  if (driverIndex < 0) return; // hanya ID terdaftar yang boleh muncul

  int index = findQualifier(idDecimal);

  if (index < 0) {
    if (qualifierCount >= MAX_QUALIFIERS) return;

    index = qualifierCount;

    qualifiers[index].name = drivers[driverIndex].name;
    qualifiers[index].idDecimal = idDecimal;
    qualifiers[index].laps = 0;
    qualifiers[index].hasStarted = false;
    qualifiers[index].lastReadMillis = 0;
    qualifiers[index].lastLapAchievedMillis = 0;
    qualifiers[index].lastLapTime = 0;
    qualifiers[index].bestLapTime = 0;
    qualifiers[index].totalLapTime = 0;
    qualifiers[index].lapTimeCount = 0;

    for (int i = 0; i < MAX_QUALIFY_LAPS; i++) {
      qualifiers[index].lapLogs[i] = 0;
    }

    qualifierCount++;
  }

  // v4.2: sama seperti addLap() -- pakai timestamp yang sudah diambil
  // sedini mungkin, bukan millis() baru di sini.
  unsigned long now = ts;

  if (!qualifiers[index].hasStarted) {
    qualifiers[index].hasStarted = true;
    qualifiers[index].lastReadMillis = now;
    qualifiers[index].lastLapAchievedMillis = now;
    sortQualifiers();

    // v4.2.1: dulu tidak ada broadcast di sini -- akibatnya driver yang
    // BARU KETANGKEP PERTAMA KALI (armed, belum ngelap) tidak langsung
    // nongol di leaderboard browser, baru muncul pas dia nyelesein lap
    // pertamanya (atau nunggu driver lain trigger broadcast). Sekarang
    // langsung di-broadcast juga supaya begitu qualifying jalan & mobil
    // pertama lewat, dia LANGSUNG muncul di leaderboard (dengan 0 lap).
    pendingStateBroadcast = true;
    return;
  }

  if (qualifiers[index].laps >= qualifyMaxLap) return;

  if (now - qualifiers[index].lastReadMillis >= qualifyMinLapInterval) {
    unsigned long lapDuration = now - qualifiers[index].lastLapAchievedMillis;

    qualifiers[index].laps++;
    qualifiers[index].lastReadMillis = now;
    qualifiers[index].lastLapAchievedMillis = now;

    qualifiers[index].lastLapTime = lapDuration;
    qualifiers[index].totalLapTime += lapDuration;

    if (qualifiers[index].lapTimeCount < MAX_QUALIFY_LAPS) {
      qualifiers[index].lapLogs[qualifiers[index].lapTimeCount] = lapDuration;
      qualifiers[index].lapTimeCount++;
    }

    if (qualifiers[index].bestLapTime == 0 ||
        lapDuration < qualifiers[index].bestLapTime) {
      qualifiers[index].bestLapTime = lapDuration;
    }

    // Sama seperti addLap(): sort DULU baru broadcast -- sebelumnya
    // kebalik, jadi leaderboard qualifying yang dikirim ke browser tiap
    // ada lap baru itu posisi LAMA (belum memperhitungkan lap yang baru
    // saja tercatat).
    sortQualifiers();

    // v4.2: sama seperti addLap() -- broadcast beneran ditunda ke
    // loop(), lihat pendingStateBroadcast.
    pendingStateBroadcast = true;
  }
}

void handleQualifyStart() {

  if (raceState == RUNNING ||
      raceState == COUNTDOWN) {

    server.send(400,
      "text/plain",
      "RACE_RUNNING");

    return;
  }

  qualifyingRunning = true;

  broadcastState();
  server.send(
    200,
    "text/plain",
    "QUALIFY_STARTED"
  );
}

void handleQualifyStop() {
  qualifyingRunning = false;
  broadcastState();
  server.send(200, "text/plain", "QUALIFY_STOPPED");
}

void handleQualifyReset() {
  qualifierCount = 0;
  qualifyingRunning = false;
  broadcastState();
  server.send(200, "text/plain", "QUALIFY_RESET");
}

void handleQualifyDelete() {
  // Dulu berbasis "index" array -- berisiko race condition karena
  // qualifiers[] auto-sort tiap ada lap baru masuk (sortQualifiers()).
  // Kalau user klik Delete pas posisi 3, tapi tepat sebelum request
  // sampai server ada lap baru yang mengubah urutan, index 3 bisa jadi
  // qualifier yang SALAH. Sekarang berbasis idDecimal -- selalu tepat
  // sasaran, tidak peduli urutan array berubah kapan saja.
  if (!server.hasArg("id")) {
    server.send(400, "text/plain", "BAD_REQUEST");
    return;
  }

  uint32_t idDecimal = strtoul(server.arg("id").c_str(), NULL, 10);
  int index = -1;

  for (int i = 0; i < qualifierCount; i++) {
    if (qualifiers[i].idDecimal == idDecimal) {
      index = i;
      break;
    }
  }

  if (index < 0) {
    server.send(400, "text/plain", "NOT_FOUND");
    return;
  }

  for (int i = index; i < qualifierCount - 1; i++) {
    qualifiers[i] = qualifiers[i + 1];
  }

  qualifierCount--;

  broadcastState();
  server.send(200, "text/plain", "QUALIFIER_DELETED");
}

void handleQualifySettings() {
  if (server.hasArg("maxlap")) {
    int maxLap = server.arg("maxlap").toInt();
    if (maxLap > 0) qualifyMaxLap = maxLap;
  }

  if (server.hasArg("cooldown")) {
    int cd = server.arg("cooldown").toInt();
    if (cd >= 1) qualifyMinLapInterval = cd * 1000;
  }

  if (server.hasArg("sort")) {
    String sort = server.arg("sort");
    qualifySortByBestLap = sort == "best";
  }

  sortQualifiers();
  saveQualifyConfig();
  broadcastState();
  server.send(200, "text/plain", "OK");
}

void saveQualifyConfig() {

  File file =
    FFat.open(
      "/qualify.cfg",
      "w"
    );

  if (!file) return;

  file.println(
    qualifyMaxLap
  );

  file.println(
    qualifyMinLapInterval
  );

  file.println(
    qualifySortByBestLap
  );

  file.close();
}


void loadQualifyConfig() {

  if (!FFat.exists(
    "/qualify.cfg"
  )) return;

  File file =
    FFat.open(
      "/qualify.cfg",
      "r"
    );

  if (!file) return;

  qualifyMaxLap =
    file.readStringUntil('\n').toInt();

  qualifyMinLapInterval =
    file.readStringUntil('\n').toInt();

  qualifySortByBestLap =
    file.readStringUntil('\n').toInt();

  file.close();
}

void saveQualifyToFile(String qualifyName, String startDateTime) {
  qualifyName.trim();

  if (qualifyName.length() == 0) return;

  if (!FFat.exists("/qualify")) {
    FFat.mkdir("/qualify");
  }

  String fileName = "/qualify/qualify_" + String(millis()) + ".json";

  File indexFile = FFat.open("/qualify_index.csv", "a");

  if (indexFile) {
    String safeName = qualifyName;
    safeName.replace("|", " ");

    String safeDate = startDateTime;
    safeDate.replace("|", " ");

    indexFile.print(fileName);
    indexFile.print("|");
    indexFile.print(safeName);
    indexFile.print("|");
    indexFile.println(safeDate);
    indexFile.close();
  }

  File file = FFat.open(fileName, "w");
  if (!file) return;

  file.println("{");
  file.println("\"qualifyName\":\"" + escapeJson(qualifyName) + "\",");
  file.println("\"startDateTime\":\"" + escapeJson(startDateTime) + "\",");
  file.println("\"qualifyMaxLap\":" + String(qualifyMaxLap) + ",");
  file.println("\"sort\":\"" + String(qualifySortByBestLap ? "best" : "lap") + "\",");
  file.println("\"qualifiers\":[");

  for (int i = 0; i < qualifierCount; i++) {
    unsigned long avgLap = 0;

    if (qualifiers[i].lapTimeCount > 0) {
      avgLap = qualifiers[i].totalLapTime / qualifiers[i].lapTimeCount;
    }

    if (i > 0) file.println(",");

    file.print("{");
    file.print("\"rank\":");
    file.print(i + 1);
    file.print(",");

    file.print("\"name\":\"");
    file.print(escapeJson(qualifiers[i].name));
    file.print("\",");

    file.print("\"idDecimal\":");
    file.print(qualifiers[i].idDecimal);
    file.print(",");

    file.print("\"laps\":");
    file.print(qualifiers[i].laps);
    file.print(",");

    file.print("\"bestLapTime\":");
    file.print(qualifiers[i].bestLapTime);
    file.print(",");

    file.print("\"lastLapTime\":");
    file.print(qualifiers[i].lastLapTime);
    file.print(",");

    file.print("\"averageLapTime\":");
    file.print(avgLap);
    file.print(",");

    file.print("\"lapLogs\":[");

    for (int l = 0; l < qualifiers[i].lapTimeCount; l++) {
      if (l > 0) file.print(",");
      file.print(qualifiers[i].lapLogs[l]);
    }

    file.print("]");
    file.print("}");
  }

  file.println("]");
  file.println("}");
  file.close();

  dbgPrint("Qualify saved: ");
  dbgPrintln(fileName);
  cleanupOldQualifyHistory();
}

void handleQualifyHistory() {
  String rows[MAX_SAVED_RACES];
  int count = 0;

  if (FFat.exists("/qualify_index.csv")) {
    File file = FFat.open("/qualify_index.csv", "r");

    while (file && file.available() && count < MAX_SAVED_RACES) {
      String row = file.readStringUntil('\n');
      row.trim();

      if (row.length() > 0) {
        rows[count++] = row;
      }
    }

    if (file) file.close();
  }

  String json = "{\"history\":[";

  bool first = true;

  for (int i = count - 1; i >= 0; i--) {
    int p1 = rows[i].indexOf('|');
    int p2 = rows[i].indexOf('|', p1 + 1);

    if (p1 < 0 || p2 < 0) continue;

    String fileName = rows[i].substring(0, p1);
    String qualifyName = rows[i].substring(p1 + 1, p2);
    String dateTime = rows[i].substring(p2 + 1);

    if (!first) json += ",";
    first = false;

    json += "{";
    json += "\"file\":\"" + escapeJson(fileName) + "\",";
    json += "\"name\":\"" + escapeJson(qualifyName) + "\",";
    json += "\"datetime\":\"" + escapeJson(dateTime) + "\"";
    json += "}";
  }

  json += "]}";

  server.send(200, "application/json", json);
}

void handleQualifyDetail() {
  if (!server.hasArg("file")) {
    server.send(400, "application/json", "{\"error\":\"NO_FILE\"}");
    return;
  }

  String fileName = server.arg("file");

  if (!fileName.startsWith("/qualify/")) {
    server.send(400, "application/json", "{\"error\":\"INVALID_FILE\"}");
    return;
  }

  if (!FFat.exists(fileName)) {
    server.send(404, "application/json", "{\"error\":\"FILE_NOT_FOUND\"}");
    return;
  }

  File file = FFat.open(fileName, "r");

  if (!file) {
    server.send(500, "application/json", "{\"error\":\"OPEN_FAILED\"}");
    return;
  }

  server.streamFile(file, "application/json");
  file.close();
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  // v4.7: seed RNG hardware ESP32 (bukan Arduino random() default yang
  // tanpa seed = urutan sama tiap boot) -- dipakai buat undi delay horn
  // random. esp_random() sendiri sudah true random (hardware), tapi
  // randomSeed() di sini bikin Arduino random(min,max) yang dipakai di
  // handleStart() ikut ter-seed dengan baik sejak awal.
  randomSeed(esp_random());

  if (!FFat.begin(true)) {
    dbgPrintln("FFat Mount Failed");
  } else {
    dbgPrintln("FFat Mounted");
    loadDrivers();
    loadConfig();
    loadQualifyConfig();
    loadWifiConfig();
  }

  rebuildRacersFromActiveDrivers();

  // v4.2: queue buat lempar {baris, timestamp} dari onPsocReceive()
  // (task UART terpisah) ke loop(). Dibuat SEBELUM psocSerial.begin()
  // supaya sudah siap begitu event RX pertama bisa saja langsung masuk.
  psocLineQueue = xQueueCreate(32, sizeof(PsocLineEvent));

  // v4.2: buffer RX diperbesar (default 256) sebagai jaring pengaman
  // tambahan -- harus dipanggil SEBELUM begin().
  psocSerial.setRxBufferSize(512);
  psocSerial.begin(57600, SERIAL_8N1, PSOC_RX_PIN, PSOC_TX_PIN);

  // v4.2: inilah inti fix akurasi -- callback ini dieksekusi oleh task
  // UART internal driver, BUKAN loop() Arduino, jadi timestamp yang
  // diambil di dalamnya (lihat onPsocReceive()) tidak pernah tersandera
  // walau loop() lagi sibuk (server.handleClient(), webSocket.loop(),
  // atau broadcast WebSocket yang blocking).
  psocSerial.onReceive(onPsocReceive);

  // Set mode AP dulu supaya MAC address AP bisa dibaca untuk bikin SSID
  // unik, baru start AP-nya pakai SSID+password yang sudah final.
  WiFi.mode(WIFI_AP);
  apSsid = buildUniqueSsid();
  WiFi.softAP(apSsid.c_str(), apPassword.c_str());

  dbgPrintln("YurLaps Pro Started");
  dbgPrint("WiFi AP: ");
  dbgPrintln(apSsid);
  dbgPrint("Password: ");
  dbgPrintln(apPassword);
  dbgPrint("IP Address: ");
  dbgPrintln(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/settings", handleSettings);
  server.on("/countdownSettings", handleCountdownSettings);
  server.on("/addDriver", handleAddDriver);
  server.on("/toggleDriver", handleToggleDriver);
  server.on("/deleteDriver", handleDeleteDriver);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/reset", handleReset);
  server.on("/qualifyStart", handleQualifyStart);
  server.on("/qualifyStop", handleQualifyStop);
  server.on("/qualifyReset", handleQualifyReset);
  server.on("/qualifyDelete", handleQualifyDelete);
  server.on("/qualifySettings", handleQualifySettings);
  server.on("/saveQualify", HTTP_GET, []() {
  String qualifyName = server.arg("name");
  String startDateTime = server.arg("datetime");

  saveQualifyToFile(qualifyName, startDateTime);

  server.send(200, "text/plain", "OK");
});

server.on("/qualifyHistory", handleQualifyHistory);
server.on("/qualifyDetail", handleQualifyDetail);
  server.on("/saveRace", HTTP_GET, []() {
  String raceName = server.arg("name");
  String startDateTime = server.arg("datetime");
   saveRaceToFile(raceName, startDateTime);
    server.send(200, "text/plain", "OK");
});
  server.on("/history", handleHistory);
  server.on("/raceDetail", handleRaceDetail);
  server.on("/start_horn.mp3", HTTP_GET, []() {
  if (!FFat.exists("/sounds/start_horn.mp3")) {
    server.send(404, "text/plain", "NO_HORN");
    return;
  }

  File file = FFat.open("/sounds/start_horn.mp3", "r");
  server.streamFile(file, "audio/mpeg");
  file.close();
});

  server.on(
  "/uploadHorn",
  HTTP_POST,
  []() {
    server.send(200, "text/plain", "Horn uploaded successfully");
  },
  handleHornUpload
);

  server.on("/wifiPassword", HTTP_POST, handleChangeWifiPassword);
  server.on("/factoryReset", HTTP_POST, handleFactoryReset);
  server.on("/diagnostics", handleDiagnostics);

  server.on(
  "/otaUpdate",
  HTTP_POST,
  []() {
    bool ok = !Update.hasError();
    server.send(200, "text/plain", ok ? "OTA_OK_REBOOTING" : "OTA_FAILED");
    delay(500);
    if (ok) ESP.restart();
  },
  handleOtaUpload
);

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Ping tiap client tiap 15 detik; kalau 2x pong berturut-turut tidak
  // dibalas (client sudah mati/zombie), library otomatis disconnect socket
  // itu. Ini fix utama untuk web UI yang freeze setelah device
  // ditinggal/HP terkunci beberapa menit -- sebelumnya socket zombie
  // begini tidak pernah terdeteksi, dan broadcastTXT() ke socket itu bisa
  // blocking lama sehingga MACETIN seluruh loop() (termasuk http server),
  // bukan cuma WebSocket.
  webSocket.enableHeartbeat(15000, 3000, 2);

  // Watchdog aplikasi -- lihat komentar WATCHDOG_TIMEOUT_SEC di atas.
  // Signature esp_task_wdt_init() beda antara arduino-esp32 core versi 2.x
  // (IDF4, terima int detik) dan versi 3.x (IDF5, terima struct config) --
  // #if di bawah supaya sketch ini kompil di kedua versi core tanpa perlu
  // diubah manual sesuai versi Arduino IDE / board package yang terpasang.
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&wdtConfig);
  #else
    esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  #endif
  esp_task_wdt_add(NULL);

  dbgPrintln("Web Server Started");
}

unsigned long lastHeartbeatBroadcast = 0;
const unsigned long heartbeatIntervalMs = 200;

void loop() {
  esp_task_wdt_reset(); // "lapor" ke watchdog bahwa loop() masih hidup/tidak macet

  updateRaceState();
  server.handleClient();
  webSocket.loop();

  // Heartbeat broadcast: cuma jalan saat race/qualifying benar-benar aktif
  // (jam/countdown-nya berubah terus walau tidak ada lap baru). Saat IDLE
  // atau STOPPED, tidak ada broadcast berkala sama sekali -- CPU C3 nganggur
  // buat itu, hemat besar dibanding polling 250ms terus-menerus 24/7.
  bool needsLiveTicking = (raceState == RUNNING || raceState == COUNTDOWN || qualifyingRunning);

  if (needsLiveTicking && millis() - lastHeartbeatBroadcast >= heartbeatIntervalMs) {
    broadcastLiveState();
    lastHeartbeatBroadcast = millis();
  }

  // v4.2: TIDAK LAGI membaca psocSerial langsung di sini. Baris + timestamp
  // sudah ditangkap oleh onPsocReceive() (task UART terpisah, lihat atas)
  // dan menunggu di psocLineQueue. Di sini kita cuma men-drain queue itu
  // (xQueueReceive dengan timeout 0 = non-blocking, tidak pernah bikin
  // loop() nunggu) dan memproses tiap baris pakai timestamp yang SUDAH
  // final -- proses parsing (CPU only, tidak menyentuh jaringan) boleh
  // secepat-cepatnya di sini karena tidak lagi mempengaruhi keakuratan
  // timestamp itu sendiri.
  PsocLineEvent ev;
  while (xQueueReceive(psocLineQueue, &ev, 0) == pdTRUE) {
    processPsocLine(String(ev.line), ev.ts);
  }

  // Broadcast jaringan (yang berpotensi blocking/tidak konstan lamanya
  // di WiFi) baru dilakukan DI SINI -- setelah semua baris yang lagi
  // tersedia selesai dicatat. Kalau psocSerial menerima baris baru
  // SAAT broadcast di bawah ini masih berjalan, baris itu aman menunggu
  // di UART (buffer 512 byte) sampai iterasi loop() berikutnya --
  // timestamp-nya TETAP diambil oleh onPsocReceive() saat baris itu
  // benar-benar selesai diterima, bukan saat loop() sempat membacanya.
  if (pendingStateBroadcast) {
    broadcastState();
    pendingStateBroadcast = false;
  }
  flushTerminalBroadcastQueue();
}