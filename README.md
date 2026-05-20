# 🌊 FEWS — Flood Early Warning System (Context-Aware)

> FEWS-IoT adalah prototipe **Flood Early Warning System** (Sistem Peringatan Dini Banjir) yang dirancang untuk memantau ketinggian air dan intensitas hujan secara real-time. Sistem ini memadukan sensor ultrasonik, sensor hujan analog, dan modul RTC untuk menghasilkan peringatan yang cerdas dan kontekstual.

>  Notifikasi dikirimkan secara otomatis ke saluran **Telegram**, sehingga petugas atau warga dapat menerima peringatan tanpa harus memantau perangkat secara langsung.
Proyek ini disimulasikan menggunakan platform **[Wokwi](https://wokwi.com/projects/461632741819653121)**.

---
## ✨ Fitur Utama

| Fitur | Deskripsi |
|---|---|
| 🔴 **Indikator LED 3-Warna** | Hijau / Kuning / Merah sesuai level bahaya |
| 🔔 **Buzzer Alarm** | Pola bunyi berbeda untuk tiap level, lebih keras di malam hari |
| 📱 **Notifikasi Telegram** | Broadcast otomatis dengan interval adaptif |
| 🌙 **Mode Siang/Malam** | Respons lebih agresif pada malam hari via RTC DS1307 |
| ⚡ **Non-Blocking Timer** | Menggunakan `millis()` agar sistem selalu responsif |
| 🌧️ **Klasifikasi Intensitas Hujan** | Cerah / Hujan Sedang / Hujan Deras |
| 💬 **Perintah `/status`** | Cek kondisi real-time kapan saja lewat Telegram |

---


## ⚡ Level Status & Logika Adaptasi

### Threshold Ketinggian Air

| Level | Jarak Sensor | Kondisi Hujan | Indikator |
|-------|-------------|---------------|-----------|
| 🟢 AMAN | > 100 cm | Tidak / ringan | LED Hijau |
| 🟡 WASPADA | 50–100 cm | Sedang | LED Kuning |
| 🔴 SIAGA 1 | 20–50 cm | Deras | LED Merah + Sirine |
| ⚫ DARURAT | < 20 cm | Apapun | Semua aktif |

### Matriks Aksi Adaptif

| Status | Siang (06:00–22:00) | Malam (22:00–06:00) |
|--------|---------------------|----------------------|
| 🟢 AMAN | LED Hijau + Heartbeat tiap jam | LED Hijau (hemat daya) |
| 🟡 WASPADA | Notif Telegram biasa | Notif Telegram *override* |
| 🔴 SIAGA 1 | Sirine pendek + Notif URGENT tiap 5 menit | Sirine panjang + Notif tiap 2 menit |
| ⚫ DARURAT | Semua aktif + Notif tiap 2 menit | Sirine terus + Notif tiap 1 menit |

---

## 🔧 Hardware

### Komponen

| Komponen | Spesifikasi | Estimasi Harga |
|----------|-------------|----------------|
| ESP32 DevKit V1 | Dual-core 240MHz, WiFi bawaan | Rp 45.000–65.000 |
| HC-SR04 + Waterproof Case | Range 2–400cm, ±3mm akurasi | Rp 25.000–40.000 |
| YL-83 Rain Sensor | Output analog + digital | Rp 15.000–25.000 |
| Active Buzzer 5V | 85dB | Rp 5.000–10.000 |
| LED Traffic Light Module | Merah / Kuning / Hijau | Rp 10.000–20.000 |
| Power Supply 5V 2A | Adaptor DC | Rp 25.000–50.000 |
| Waterproof Box | IP65 | Rp 15.000–30.000 |
| Kabel, Breadboard, Resistor | — | Rp 15.000 |
| **Total Estimasi** | | **Rp 155.000–255.000** |

### Wiring (Pin Mapping ESP32)

```
HC-SR04  → TRIG: GPIO12  |  ECHO: GPIO14
YL-83    → AO:   GPIO34  |  DO:   GPIO27
LED Hijau   → GPIO25
LED Kuning  → GPIO26
LED Merah   → GPIO33
Buzzer      → GPIO32
```

---

## 🚀 Instalasi & Setup

### Prasyarat

- [PlatformIO IDE](https://platformio.org/) atau Arduino IDE
- Akun Telegram (untuk bot notifikasi)
- ESP32 DevKit V1

### 1. Clone Repositori

```bash
git clone https://github.com/USERNAME/fews-iot.git
cd fews-iot
```

### 2. Konfigurasi Kredensial

Edit bagian ini di `src/main.cpp`:

```cpp
#define WIFI_SSID        "NAMA_WIFI_KAMU"
#define WIFI_PASSWORD    "PASSWORD_WIFI"
#define TELEGRAM_TOKEN   "TOKEN_BOT_TELEGRAM"
#define TELEGRAM_CHAT_ID "CHAT_ID_KAMU"
```

> 🔐 **Penting:** Jangan pernah commit file dengan token asli! Gunakan `.env` atau secrets manager untuk produksi.

### 3. Buat Telegram Bot

Lihat panduan lengkap di [`docs/SETUP_TELEGRAM.md`](docs/SETUP_TELEGRAM.md).

Singkatnya:
1. Chat `@BotFather` di Telegram → `/newbot`
2. Salin token yang diberikan ke `TELEGRAM_TOKEN`
3. Chat bot kamu → dapatkan `chat_id` via `https://api.telegram.org/bot<TOKEN>/getUpdates`

### 4. Upload Firmware

**PlatformIO:**
```bash
pio run --target upload
pio device monitor
```

**Arduino IDE:**
- Board: `ESP32 Dev Module`
- Upload Speed: `115200`
- Buka Serial Monitor @ 115200 baud

---

## 🧪 Simulasi (Wokwi)

Simulasi tersedia sebelum hardware tersedia:

1. Buka [wokwi.com](https://wokwi.com)
2. Klik **New Project** → **ESP32**
3. Ganti konten `diagram.json` dengan file di `simulation/diagram.json`
4. Paste kode dari `src/main.cpp`
5. Jalankan simulasi — gunakan **potensiometer** untuk mensimulasikan sensor hujan

---

## 📱 Perintah Telegram Bot

| Perintah | Fungsi |
|----------|--------|
| `/status` | Tampilkan status terkini (ketinggian air, hujan, waktu, level) |

### Contoh Respons `/status`

```
📊 Status Terkini FEWS
📍 Ketinggian air : 75 cm dari sensor
🌧️ Intensitas hujan: Sedang
🕐 Waktu          : 14:32 WIB (Siang 06:00–22:00)
📶 Status         : 🟡 WASPADA
```

---

## 📋 5 Skenario Pervasive

Lihat detail di [`docs/SKENARIO.md`](docs/SKENARIO.md).

| # | Skenario | Waktu | Kondisi |
|---|----------|-------|---------|
| 1 | Waspada Dini Siang | Siang | Hujan mulai deras, air naik |
| 2 | Siaga 1 Siang | Siang | Air kritis + hujan deras |
| 3 | **Siaga 1 Malam** ⭐ | Malam | Air kritis + warga tidur |
| 4 | Kondisi Membaik | Kapan saja | Air surut, hujan berhenti |
| 5 | Cek Status On-Demand | Kapan saja | `/status` via Telegram |

---

## 🏗️ Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────┐
│                      SENSOR LAYER                       │
│   [HC-SR04]           [YL-83]          [NTP/WiFi]       │
│  Ketinggian Air    Intensitas Hujan    Waktu Hari        │
└──────────┬──────────────────┬──────────────┬────────────┘
           └──────────────────┴──────────────┘
                              │
                    ┌─────────▼─────────┐
                    │      ESP32        │
                    │   DevKit V1       │
                    │   Logic Engine    │
                    └─────────┬─────────┘
           ┌──────────────────┼──────────────────┐
           │                  │                  │
    ┌──────▼──────┐   ┌───────▼──────┐   ┌──────▼──────┐
    │  LOCAL OUT  │   │  CLOUD OUT   │   │  LOG DATA   │
    │ LED + Siren │   │ Telegram Bot │   │   SPIFFS    │
    └─────────────┘   └──────────────┘   └─────────────┘
```

---

## 📊 Stack Teknologi

| Layer | Teknologi |
|-------|-----------|
| Hardware | ESP32 DevKit V1, HC-SR04, YL-83, Buzzer, LED |
| Firmware | Arduino (C++) via PlatformIO |
| Konektivitas | WiFi built-in ESP32 |
| Sinkronisasi Waktu | NTP (`pool.ntp.org`) — gratis, tanpa modul tambahan |
| Notifikasi | Telegram Bot API (gratis) |
| Log Data | SPIFFS (internal flash ESP32) |
| Dashboard Opsional | ThingSpeak / Blynk |
| Simulasi | Wokwi |

---

## 🔒 Keamanan & Privasi

- Token Telegram **jangan** dicommit ke repositori publik — gunakan environment variable atau file `config.h` yang masuk `.gitignore`
- Akses bot dibatasi hanya ke `TELEGRAM_CHAT_ID` yang terdaftar
- Data sensor disimpan lokal di SPIFFS — tidak ada data pengguna yang dikirim ke pihak ketiga

---

## 📄 Lisensi

Proyek ini menggunakan lisensi [MIT](LICENSE). Bebas digunakan dan dimodifikasi untuk keperluan pendidikan.

---

## 👥 Tim

Proyek mata kuliah **Pervasive Computing** — Teknik Informatika

---
