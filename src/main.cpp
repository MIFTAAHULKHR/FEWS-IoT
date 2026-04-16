/**
 * ============================================================
 *  FEWS — Flood Early Warning System (Context-Aware)
 *  Platform   : ESP32 DevKit V1
 *  Framework  : Arduino (PlatformIO / Arduino IDE)
 *  Author     : Tim Mahasiswa Teknik Informatika
 * ============================================================
 *
 *  3 Sumber Konteks:
 *    1. Ketinggian Air  → HC-SR04 Ultrasonik
 *    2. Intensitas Hujan → YL-83 / MH-RD
 *    3. Waktu Hari      → NTP via WiFi
 *
 *  Output:
 *    - LED Traffic Light (Hijau / Kuning / Merah)
 *    - Active Buzzer
 *    - Notifikasi Telegram Bot
 *    - Log SPIFFS
 * ============================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <SPIFFS.h>

// ── Konfigurasi WiFi & Telegram ───────────────────────────
#define WIFI_SSID        "NAMA_WIFI_KAMU"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define TELEGRAM_TOKEN   "TOKEN_BOT_TELEGRAM"
#define TELEGRAM_CHAT_ID "CHAT_ID_KAMU"

// ── Pin Mapping ───────────────────────────────────────────
// HC-SR04 (Ultrasonik)
#define TRIG_PIN    12
#define ECHO_PIN    14

// YL-83 Rain Sensor
#define RAIN_AO_PIN 34   // Analog Output
#define RAIN_DO_PIN 27   // Digital Output (1 = tidak hujan, 0 = hujan)

// LED Traffic Light
#define LED_GREEN   25
#define LED_YELLOW  26
#define LED_RED     33

// Buzzer Aktif
#define BUZZER_PIN  32

// ── Threshold Ketinggian Air (jarak sensor ke permukaan air, cm) ──
#define LEVEL_SAFE       100  // > 100cm  → AMAN
#define LEVEL_WASPADA     50  // 50–100cm → WASPADA
#define LEVEL_SIAGA1      20  // 20–50cm  → SIAGA 1
                              // < 20cm   → DARURAT

// ── Threshold Sensor Hujan (nilai ADC, 0–4095) ───────────
#define RAIN_LIGHT_THRESHOLD  3000  // > 3000  → tidak hujan
#define RAIN_MED_THRESHOLD    2000  // 2000–3000 → hujan sedang
                                    // < 2000  → hujan deras

// ── NTP ──────────────────────────────────────────────────
#define NTP_SERVER    "pool.ntp.org"
#define TZ_OFFSET_WIB 7 * 3600   // UTC+7

// ── Interval (ms) ─────────────────────────────────────────
#define LOOP_INTERVAL         2000   // baca sensor tiap 2 detik
#define HEARTBEAT_INTERVAL  3600000  // heartbeat tiap 1 jam
#define NOTIF_DARURAT_SIANG  120000  // notif darurat siang tiap 2 mnt
#define NOTIF_DARURAT_MALAM   60000  // notif darurat malam tiap 1 mnt
#define NOTIF_SIAGA1_INTERVAL 300000 // notif siaga 1 tiap 5 mnt

// ── Enum Status ───────────────────────────────────────────
typedef enum {
  STATUS_AMAN = 0,
  STATUS_WASPADA,
  STATUS_SIAGA1,
  STATUS_DARURAT
} FloodStatus;

typedef enum {
  RAIN_TIDAK = 0,
  RAIN_RINGAN,
  RAIN_SEDANG,
  RAIN_DERAS
} RainLevel;

// ── Variabel Global ───────────────────────────────────────
FloodStatus currentStatus    = STATUS_AMAN;
FloodStatus previousStatus   = STATUS_AMAN;
RainLevel   currentRain      = RAIN_TIDAK;
float       waterDistanceCm  = 0;
int         rainAnalog        = 0;
bool        isNightTime       = false;

unsigned long lastLoopTime      = 0;
unsigned long lastNotifTime     = 0;
unsigned long lastHeartbeatTime = 0;

// ── Forward Declarations ──────────────────────────────────
float   readWaterLevel();
RainLevel readRainSensor(int &analogVal);
bool    checkNightTime();
FloodStatus determineStatus(float dist, RainLevel rain);
void    setLED(FloodStatus status);
void    setBuzzer(FloodStatus status, bool night);
void    sendTelegram(const String &msg);
void    handleNotification(FloodStatus status, bool statusChanged, bool night);
void    logToSPIFFS(const String &entry);
void    handleTelegramCommand();
String  buildStatusMessage();

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);

  // Pin mode
  pinMode(TRIG_PIN,    OUTPUT);
  pinMode(ECHO_PIN,    INPUT);
  pinMode(RAIN_DO_PIN, INPUT);
  pinMode(LED_GREEN,   OUTPUT);
  pinMode(LED_YELLOW,  OUTPUT);
  pinMode(LED_RED,     OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);

  // Matikan semua output
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] Mount gagal!");
  } else {
    Serial.println("[SPIFFS] OK");
  }

  // WiFi
  Serial.printf("[WiFi] Menghubungkan ke %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Terhubung! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WiFi] GAGAL terhubung — sistem lanjut tanpa internet");
  }

  // NTP
  configTime(TZ_OFFSET_WIB, 0, NTP_SERVER);
  Serial.println("[NTP] Sinkronisasi waktu...");
  delay(2000);

  // Notif startup
  if (WiFi.isConnected()) {
    sendTelegram("🟢 *FEWS Online*\n"
                 "Flood Early Warning System aktif.\n"
                 "Kirim /status untuk cek kondisi terkini.");
  }

  Serial.println("[FEWS] Sistem siap.");
}

// ═══════════════════════════════════════════════════════════
//  LOOP UTAMA
// ═══════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // ── Baca sensor tiap LOOP_INTERVAL ────────────────────
  if (now - lastLoopTime >= LOOP_INTERVAL) {
    lastLoopTime = now;

    waterDistanceCm = readWaterLevel();
    currentRain     = readRainSensor(rainAnalog);
    isNightTime     = checkNightTime();

    previousStatus  = currentStatus;
    currentStatus   = determineStatus(waterDistanceCm, currentRain);

    bool statusChanged = (currentStatus != previousStatus);

    // Update output fisik
    setLED(currentStatus);
    setBuzzer(currentStatus, isNightTime);

    // Log serial
    Serial.printf("[SENSOR] Jarak=%.1fcm | Hujan ADC=%d | Malam=%s | Status=%d\n",
                  waterDistanceCm, rainAnalog, isNightTime ? "Ya" : "Tidak", currentStatus);

    // Notifikasi & log
    handleNotification(currentStatus, statusChanged, isNightTime);
    logToSPIFFS(buildStatusMessage());
  }

  // ── Heartbeat jam-an ──────────────────────────────────
  if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
    lastHeartbeatTime = now;
    if (WiFi.isConnected() && currentStatus == STATUS_AMAN) {
      sendTelegram("💓 *Heartbeat FEWS*\n" + buildStatusMessage());
    }
  }

  // ── Cek perintah Telegram ─────────────────────────────
  if (WiFi.isConnected()) {
    handleTelegramCommand();
  }
}

// ═══════════════════════════════════════════════════════════
//  FUNGSI SENSOR
// ═══════════════════════════════════════════════════════════

/**
 * Membaca jarak ultrasonik HC-SR04 (cm).
 * Makin kecil nilainya → air makin tinggi (makin dekat sensor).
 */
float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duration == 0) return 999.0; // sensor error → anggap aman

  float distance = (duration * 0.0343) / 2.0;
  return distance;
}

/**
 * Membaca sensor hujan YL-83.
 * Makin rendah nilai ADC → permukaan makin basah → hujan makin deras.
 */
RainLevel readRainSensor(int &analogVal) {
  analogVal = analogRead(RAIN_AO_PIN);

  if (analogVal > RAIN_LIGHT_THRESHOLD) return RAIN_TIDAK;
  if (analogVal > RAIN_MED_THRESHOLD)   return RAIN_RINGAN;
  if (analogVal > 1000)                 return RAIN_SEDANG;
  return RAIN_DERAS;
}

/**
 * Menentukan apakah sekarang malam (22.00–06.00 WIB).
 */
bool checkNightTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false; // fallback: anggap siang
  int hour = timeinfo.tm_hour;
  return (hour >= 22 || hour < 6);
}

// ═══════════════════════════════════════════════════════════
//  LOGIKA ADAPTIF
// ═══════════════════════════════════════════════════════════

FloodStatus determineStatus(float dist, RainLevel rain) {
  // DARURAT: air sangat tinggi ATAU ketinggian kritis
  if (dist < LEVEL_SIAGA1)  return STATUS_DARURAT;
  // SIAGA 1: air tinggi atau kombinasi waspada + hujan deras
  if (dist < LEVEL_WASPADA) return STATUS_SIAGA1;
  if (dist < LEVEL_SAFE && rain == RAIN_DERAS) return STATUS_SIAGA1;
  // WASPADA
  if (dist < LEVEL_SAFE || rain >= RAIN_SEDANG) return STATUS_WASPADA;
  return STATUS_AMAN;
}

// ═══════════════════════════════════════════════════════════
//  OUTPUT FISIK
// ═══════════════════════════════════════════════════════════

void setLED(FloodStatus status) {
  digitalWrite(LED_GREEN,  status == STATUS_AMAN     ? HIGH : LOW);
  digitalWrite(LED_YELLOW, status == STATUS_WASPADA  ? HIGH : LOW);
  digitalWrite(LED_RED,    (status == STATUS_SIAGA1 || status == STATUS_DARURAT) ? HIGH : LOW);
}

void setBuzzer(FloodStatus status, bool night) {
  switch (status) {
    case STATUS_AMAN:
    case STATUS_WASPADA:
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case STATUS_SIAGA1:
      // Sirine pendek di siang hari, panjang di malam hari
      digitalWrite(BUZZER_PIN, HIGH);
      delay(night ? 1000 : 300);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case STATUS_DARURAT:
      // Sirine terus menerus — dikendalikan di loop utama
      digitalWrite(BUZZER_PIN, HIGH);
      break;
  }
}

// ═══════════════════════════════════════════════════════════
//  NOTIFIKASI TELEGRAM
// ═══════════════════════════════════════════════════════════

void sendTelegram(const String &msg) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure(); // Skip cert validation (cukup untuk proyek ini)

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + msg + "&parse_mode=Markdown";

  http.begin(client, url);
  int code = http.GET();
  if (code != 200) {
    Serial.printf("[Telegram] HTTP error: %d\n", code);
  }
  http.end();
}

void handleNotification(FloodStatus status, bool statusChanged, bool night) {
  unsigned long now = millis();

  // Kirim notif saat status berubah
  if (statusChanged) {
    lastNotifTime = now;
    switch (status) {
      case STATUS_AMAN:
        sendTelegram("✅ *Kondisi Aman*\n" + buildStatusMessage());
        break;
      case STATUS_WASPADA:
        sendTelegram(night
          ? "⚠️ *WASPADA (MALAM)*\nHujan deras / air naik. Tetap pantau!"
          : "⚠️ *Waspada!* Hujan deras, pantau ketinggian air.");
        break;
      case STATUS_SIAGA1:
        sendTelegram(night
          ? "🚨 *SIAGA 1 MALAM!* Segera evakuasi!\n" + buildStatusMessage()
          : "🚨 *SIAGA 1!* Segera amankan barang.\n" + buildStatusMessage());
        break;
      case STATUS_DARURAT:
        sendTelegram("🆘 *DARURAT!* Evakuasi SEGERA!\n" + buildStatusMessage());
        break;
    }
    return;
  }

  // Notif berulang untuk kondisi kritis
  unsigned long interval = (status == STATUS_DARURAT)
    ? (night ? NOTIF_DARURAT_MALAM : NOTIF_DARURAT_SIANG)
    : (status == STATUS_SIAGA1 ? NOTIF_SIAGA1_INTERVAL : 0);

  if (interval > 0 && (now - lastNotifTime >= interval)) {
    lastNotifTime = now;
    sendTelegram((status == STATUS_DARURAT
      ? "🆘 *[DARURAT - PENGINGAT]*\n"
      : "🚨 *[SIAGA 1 - PENGINGAT]*\n") + buildStatusMessage());
  }
}

// ═══════════════════════════════════════════════════════════
//  LOG SPIFFS
// ═══════════════════════════════════════════════════════════

void logToSPIFFS(const String &entry) {
  File f = SPIFFS.open("/log.txt", FILE_APPEND);
  if (!f) return;
  f.println(entry);
  f.close();

  // Rotasi sederhana: hapus jika > 50KB
  if (SPIFFS.usedBytes() > 50000) {
    SPIFFS.remove("/log.txt");
  }
}

// ═══════════════════════════════════════════════════════════
//  PERINTAH TELEGRAM (/status)
// ═══════════════════════════════════════════════════════════

/**
 * Polling sederhana untuk perintah /status dari Telegram.
 * Untuk produksi, gunakan webhook + server.
 */
void handleTelegramCommand() {
  static long lastUpdateId = 0;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_TOKEN) +
               "/getUpdates?offset=" + String(lastUpdateId + 1) + "&timeout=1";
  http.begin(client, url);
  http.setTimeout(3000);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    if (payload.indexOf("/status") >= 0) {
      sendTelegram("📊 *Status Terkini FEWS*\n" + buildStatusMessage());
      // Update offset (parsing sederhana)
      int idIdx = payload.indexOf("\"update_id\":");
      if (idIdx >= 0) {
        lastUpdateId = payload.substring(idIdx + 12, idIdx + 22).toInt();
      }
    }
  }
  http.end();
}

// ═══════════════════════════════════════════════════════════
//  HELPER: MEMBANGUN PESAN STATUS
// ═══════════════════════════════════════════════════════════

String buildStatusMessage() {
  struct tm timeinfo;
  char timeStr[20] = "N/A";
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M WIB", &timeinfo);
  }

  const char* statusLabel[] = {"🟢 AMAN", "🟡 WASPADA", "🔴 SIAGA 1", "⚫ DARURAT"};
  const char* rainLabel[]   = {"Tidak Hujan", "Ringan", "Sedang", "Deras"};
  const char* periodLabel   = isNightTime ? "Malam (22:00–06:00)" : "Siang (06:00–22:00)";

  String msg = "";
  msg += "📍 Ketinggian air : " + String((int)waterDistanceCm) + " cm dari sensor\n";
  msg += "🌧️ Intensitas hujan: " + String(rainLabel[currentRain]) + "\n";
  msg += "🕐 Waktu          : " + String(timeStr) + " (" + periodLabel + ")\n";
  msg += "📶 Status         : " + String(statusLabel[currentStatus]);
  return msg;
}
