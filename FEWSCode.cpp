#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

namespace Config {
  const char* WIFI_SSID        = "NamaWiFi";      
  const char* WIFI_PASSWORD    = "PasswordWiFi";   
  const int   WIFI_TIMEOUT_MS  = 15000;

  const char* BOT_TOKEN = "yourbottoken"; 
  const String CHAT_ID  = "yourchatid";  

  const unsigned long INTERVAL_BACA_SENSOR = 500;
  const unsigned long INTERVAL_SERIAL_LOG  = 1000;
  const unsigned long INTERVAL_TELEGRAM    = 60000;
  const unsigned long INTERVAL_CEK_WIFI    = 30000;


  const float BATAS_BAHAYA  = 20.0;
  const float BATAS_WASPADA = 50.0;

  const float        JARAK_MAX_CM       = 400.0;
  const float        JARAK_MIN_CM       = 2.0;
  const unsigned int TIMEOUT_ULTRASONIK = 30000;
}


namespace Pin {

  constexpr int TRIG       = 2;
  constexpr int ECHO       = 15;
  constexpr int HUJAN      = 34;
  constexpr int LED_HIJAU  = 13;
  constexpr int LED_KUNING = 12;
  constexpr int LED_MERAH  = 14;
  constexpr int BUZZER     = 26;
}

enum class StatusBanjir { AMAN, WASPADA, BAHAYA, SENSOR_ERROR };


struct DataSensor {
  float        jarak       = -1.0;
  int          persenHujan = 0;
  StatusBanjir status      = StatusBanjir::SENSOR_ERROR;
};

struct StatusSistem {
  bool         wifiTerhubung  = false;
  bool         rtcOK          = false;
  bool         ultrasonikOK   = false;
  StatusBanjir statusTerakhirTerkirim = StatusBanjir::AMAN;
};


RTC_DS1307        rtc;
WiFiClientSecure  clientSecure;
UniversalTelegramBot bot(Config::BOT_TOKEN, clientSecure);

DataSensor   sensor;
StatusSistem sistem;

unsigned long tBacaSensor = 0;
unsigned long tSerialLog  = 0;
unsigned long tTelegram   = 0;
unsigned long tCekWifi    = 0;


float bacaJarak() {
  digitalWrite(Pin::TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(Pin::TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(Pin::TRIG, LOW);

  long durasi = pulseIn(Pin::ECHO, HIGH, Config::TIMEOUT_ULTRASONIK);
  if (durasi == 0) return -1.0;


  float jarak = durasi * 0.034f / 2.0f;
  if (jarak < Config::JARAK_MIN_CM || jarak > Config::JARAK_MAX_CM) return -1.0;
  return jarak;
}

int bacaPersenHujan() {
  int raw = analogRead(Pin::HUJAN);
  return map(raw, 0, 4095, 100, 0);

}

StatusBanjir tentukanStatus(float jarak) {
  if (jarak < 0)                       return StatusBanjir::SENSOR_ERROR;
  if (jarak < Config::BATAS_BAHAYA)    return StatusBanjir::BAHAYA;
  if (jarak < Config::BATAS_WASPADA)   return StatusBanjir::WASPADA;
  return StatusBanjir::AMAN;
}

void bacaSemuaSensor() {
  sensor.jarak        = bacaJarak();
  sensor.persenHujan  = bacaPersenHujan();
  sensor.status       = tentukanStatus(sensor.jarak);
  sistem.ultrasonikOK = (sensor.jarak >= 0);
}


void setLED(bool hijau, bool kuning, bool merah) {
  digitalWrite(Pin::LED_HIJAU,  hijau  ? HIGH : LOW);
  digitalWrite(Pin::LED_KUNING, kuning ? HIGH : LOW);
  digitalWrite(Pin::LED_MERAH,  merah  ? HIGH : LOW);
}

void updateAktuator() {
  switch (sensor.status) {
    case StatusBanjir::BAHAYA:
      setLED(false, false, true);
      digitalWrite(Pin::BUZZER, HIGH);
      break;

    case StatusBanjir::WASPADA:
      setLED(false, true, false);
      digitalWrite(Pin::BUZZER, LOW);
      break;

    case StatusBanjir::AMAN:
      setLED(true, false, false);
      digitalWrite(Pin::BUZZER, LOW);
      break;

    case StatusBanjir::SENSOR_ERROR:
      digitalWrite(Pin::BUZZER, LOW);
      break;

  }
}


bool hubungkanWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.printf("[WiFi] Menghubungkan ke: %s\n", Config::WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai > Config::WIFI_TIMEOUT_MS) {
      Serial.println("[WiFi] GAGAL — Timeout.");
      return false;
    }
    delay(300);
    Serial.print('.');
  }

  Serial.printf("\n[WiFi] Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void cekKoneksiWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Koneksi terputus, mencoba ulang...");
    sistem.wifiTerhubung = hubungkanWifi();
  }
}


String statusKeString(StatusBanjir s) {
  switch (s) {
    case StatusBanjir::BAHAYA:       return "BAHAYA";
    case StatusBanjir::WASPADA:      return "WASPADA";
    case StatusBanjir::AMAN:         return "AMAN";
    case StatusBanjir::SENSOR_ERROR: return "ERROR SENSOR";
    default:                         return "TIDAK DIKETAHUI";
  }
}

String statusKeEmoji(StatusBanjir s) {
  switch (s) {
    case StatusBanjir::BAHAYA:       return "🔴";
    case StatusBanjir::WASPADA:      return "🟡";
    case StatusBanjir::AMAN:         return "🟢";
    case StatusBanjir::SENSOR_ERROR: return "⚠️";
    default:                         return "❓";
  }
}

String buatPesanTelegram() {
  String jam = "--:--:--";
  String tanggal = "----/--/--";

  if (sistem.rtcOK) {
    DateTime now = rtc.now();
    char bufJam[9], bufTgl[11];
    snprintf(bufJam, sizeof(bufJam), "%02d:%02d:%02d",
             now.hour(), now.minute(), now.second());
    snprintf(bufTgl, sizeof(bufTgl), "%04d/%02d/%02d",
             now.year(), now.month(), now.day());
    jam     = String(bufJam);
    tanggal = String(bufTgl);
  }

  String emoji = statusKeEmoji(sensor.status);
  String status = statusKeString(sensor.status);

  String pesan = "🌊 *FEWS — Peringatan Banjir*\n";
  pesan += "━━━━━━━━━━━━━━━━━━\n";
  pesan += emoji + " *Status   :* " + status + "\n";

  if (sensor.jarak >= 0) {
    pesan += "📏 *Jarak    :* " + String(sensor.jarak, 1) + " cm\n";
  } else {
    pesan += "📏 *Jarak    :* Sensor Error\n";
  }

  pesan += "🌧 *Hujan    :* " + String(sensor.persenHujan) + "%\n";
  pesan += "📅 *Tanggal  :* " + tanggal + "\n";
  pesan += "🕐 *Waktu    :* " + jam + "\n";
  pesan += "📡 *WiFi     :* " + String(sistem.wifiTerhubung ? "Terhubung ✅" : "Terputus ❌") + "\n";
  pesan += "━━━━━━━━━━━━━━━━━━";
  return pesan;
}

void kirimTelegramJikaPerlu() {
  if (!sistem.wifiTerhubung) return;

  bool statusBerubah = (sensor.status != sistem.statusTerakhirTerkirim);
  bool kondisiBahaya = (sensor.status == StatusBanjir::BAHAYA ||
                        sensor.status == StatusBanjir::SENSOR_ERROR);
  bool intervalTercapai = (millis() - tTelegram >= Config::INTERVAL_TELEGRAM);

  if (kondisiBahaya && (statusBerubah || intervalTercapai)) {

    Serial.println("[Telegram] Mengirim notifikasi...");
    String pesan = buatPesanTelegram();

    if (bot.sendMessage(Config::CHAT_ID, pesan, "Markdown")) {
      Serial.println("[Telegram] Pesan terkirim ✓");
      sistem.statusTerakhirTerkirim = sensor.status;
      tTelegram = millis();
    } else {
      Serial.println("[Telegram] GAGAL mengirim! Cek token/chat_id/koneksi.");
    }
  }

  if (sensor.status == StatusBanjir::AMAN &&

      sistem.statusTerakhirTerkirim != StatusBanjir::AMAN &&
      sistem.statusTerakhirTerkirim != StatusBanjir::SENSOR_ERROR) {
    Serial.println("[Telegram] Mengirim notifikasi AMAN...");
    String pesan = buatPesanTelegram();
    pesan += "\n\n✅ Kondisi telah kembali AMAN.";
    if (bot.sendMessage(Config::CHAT_ID, pesan, "Markdown")) {
      Serial.println("[Telegram] Notifikasi AMAN terkirim ✓");
      sistem.statusTerakhirTerkirim = sensor.status;
      tTelegram = millis();
    }
  }
}


void logSerial() {
  char buf[120];

  Serial.print("[SENSOR] Jarak: ");
  if (sensor.jarak >= 0) {
    snprintf(buf, sizeof(buf), "%5.1f cm", sensor.jarak);
  } else {
    snprintf(buf, sizeof(buf), "ERROR     ");
  }
  Serial.print(buf);

  snprintf(buf, sizeof(buf), " | Hujan: %3d%%", sensor.persenHujan);
  Serial.print(buf);

  Serial.print(" | Status: ");
  Serial.print(statusKeEmoji(sensor.status));
  Serial.print(" ");
  Serial.print(statusKeString(sensor.status));

  if (sistem.rtcOK) {
    DateTime now = rtc.now();
    snprintf(buf, sizeof(buf), " | Jam: %02d:%02d:%02d",
             now.hour(), now.minute(), now.second());
    Serial.print(buf);
  } else {
    Serial.print(" | Jam: RTC Error");
  }

  Serial.print(" | WiFi: ");
  Serial.print(sistem.wifiTerhubung ? "OK" : "PUTUS");

  Serial.println();
}


void tesDiagnostik() {
  Serial.println("\n========== TES DIAGNOSTIK ==========");

  Serial.print("  LED    : ");

  setLED(true, false, false); delay(300);
  setLED(false, true, false); delay(300);
  setLED(false, false, true); delay(300);
  setLED(false, false, false);
  Serial.println("OK");

  Serial.print("  BUZZER : ");

  digitalWrite(Pin::BUZZER, HIGH); delay(200);
  digitalWrite(Pin::BUZZER, LOW);
  Serial.println("OK");

  Serial.print("  RTC    : ");

  Wire.begin(21, 22);

  if (!rtc.begin()) {
    Serial.println("TIDAK TERDETEKSI! ← Periksa wiring SDA/SCL dan baterai CR2032.");
    sistem.rtcOK = false;
  } else {
    sistem.rtcOK = true;
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("OK (waktu disetel otomatis dari kompilasi)");

    } else {
      DateTime now = rtc.now();
      char buf[30];
      snprintf(buf, sizeof(buf), "OK  [%04d/%02d/%02d %02d:%02d:%02d]",
               now.year(), now.month(), now.day(),
               now.hour(), now.minute(), now.second());
      Serial.println(buf);
    }
  }

  Serial.print("  WIFI   : ");

  sistem.wifiTerhubung = hubungkanWifi();
  if (sistem.wifiTerhubung) {
    Serial.println("OK");
  } else {
    Serial.println("GAGAL ← Mode offline, Telegram tidak aktif.");
  }

  if (sistem.wifiTerhubung) {

    Serial.print("  BOT    : ");
    clientSecure.setInsecure();

    User me = bot.getMe();

    if (me.id != 0) {
      Serial.print("OK  [@");
      Serial.print(me.username);
      Serial.println("]");
    } else {
      Serial.println("GAGAL ← Periksa BOT_TOKEN atau koneksi internet.");
    }
  }

  Serial.print("  ULTRAS : ");

  float jarakTest = bacaJarak();
  if (jarakTest >= 0) {
    Serial.printf("OK  [%.1f cm]\n", jarakTest);
  } else {
    Serial.println("ERROR ← Periksa wiring TRIG/ECHO.");
  }

  Serial.println("=====================================\n");
}


void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(Pin::TRIG,       OUTPUT);

  pinMode(Pin::ECHO,       INPUT);
  pinMode(Pin::LED_HIJAU,  OUTPUT);
  pinMode(Pin::LED_KUNING, OUTPUT);
  pinMode(Pin::LED_MERAH,  OUTPUT);
  pinMode(Pin::BUZZER,     OUTPUT);

  digitalWrite(Pin::LED_HIJAU,  LOW);

  digitalWrite(Pin::LED_KUNING, LOW);
  digitalWrite(Pin::LED_MERAH,  LOW);
  digitalWrite(Pin::BUZZER,     LOW);

  tesDiagnostik();

  Serial.println("[SISTEM] FEWS Aktif — Monitoring dimulai\n");
}


void loop() {
  unsigned long sekarang = millis();

  if (sekarang - tBacaSensor >= Config::INTERVAL_BACA_SENSOR) {

    tBacaSensor = sekarang;
    bacaSemuaSensor();
    updateAktuator();

    if (sensor.status == StatusBanjir::SENSOR_ERROR) {

      bool nyala = (sekarang / 250) % 2;
      setLED(false, false, nyala);
    }
  }

  if (sekarang - tSerialLog >= Config::INTERVAL_SERIAL_LOG) {

    tSerialLog = sekarang;
    logSerial();
  }

  if (sekarang - tCekWifi >= Config::INTERVAL_CEK_WIFI) {

    tCekWifi = sekarang;
    cekKoneksiWifi();
  }

  kirimTelegramJikaPerlu();

}
