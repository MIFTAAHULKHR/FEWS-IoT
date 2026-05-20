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

## 🚦 Logika Status Bahaya

Sistem membaca jarak dari sensor ultrasonik sebagai representasi **ketinggian air** (semakin kecil jarak = semakin tinggi air).

| Status | Jarak Sensor | LED | Buzzer | Interval Telegram |
|---|---|---|---|---|
| 🟢 **AMAN** | > 100 cm | Hijau | Mati | Tidak dikirim |
| 🟡 **WASPADA** | 50 – 100 cm | Kuning | Mati | Setiap 5 menit |
| 🔴 **SIAGA 1** | 20 – 49 cm | Merah | Bip intermiten | Malam: 2 mnt / Siang: 5 mnt |
| 🆘 **DARURAT** | < 20 cm | Merah | Sirine terus-menerus | Malam: 1 mnt / Siang: 2 mnt |

---

## 🔧 Hardware

### Komponen

| Komponen | Spesifikasi | 
|----------|-------------|
| ESP32 DevKit V1 | Dual-core 240MHz, WiFi bawaan | 
| HC-SR04 + Waterproof Case | Range 2–400cm, ±3mm akurasi | 
| YL-83 Rain Sensor | Output analog + digital | 
| Active Buzzer 5V | 85dB | 
| LED Traffic Light Module | Merah / Kuning / Hijau | 
| Power Supply 5V 2A | Adaptor DC |
| Waterproof Box | IP65 | 
| Kabel, Breadboard, Resistor | — | 

---

## 🚀 Cara Menjalankan

### Simulasi di Wokwi (Direkomendasikan)

1. Buka tautan proyek: [https://wokwi.com/projects/461632741819653121](https://wokwi.com/projects/461632741819653121)
2. Masukkan **Token Bot** dan **Chat ID** Telegram Anda di `sketch.ino` (lihat bagian [Konfigurasi Telegram Bot](#-konfigurasi-telegram-bot)).
3. Klik tombol **▶ Play** untuk memulai simulasi.
4. Putar potensiometer untuk mengatur intensitas hujan.
5. Klik sensor HC-SR04 dan ubah nilai `distance` untuk mensimulasikan ketinggian air.

### Deploy ke Hardware Asli

1. Install **Arduino IDE** dan tambahkan board **ESP32** melalui Board Manager.
2. Install library yang dibutuhkan (lihat [`libraries.txt`](libraries.txt)):
   - `RTClib`
   - `CTBot`
   - `ArduinoJson` (v6.21.5)
3. Buka `sketch.ino`, isi kredensial WiFi dan Telegram.
4. Upload ke board ESP32.

---

## ⚙️ Konfigurasi Telegram Bot

Edit bagian berikut di `sketch.ino`:

```cpp
// Kredensial WiFi
String ssid = "NAMA_WIFI_ANDA";
String pass = "PASSWORD_WIFI_ANDA";

// Telegram Bot
String token = "ISI_TOKEN_BOT_ANDA";
const int64_t bot_id = ISI_CHAT_ID_ANDA;
```

### Cara Mendapatkan Token Bot

1. Buka Telegram, cari **@BotFather**.
2. Ketik `/newbot` dan ikuti instruksi.
3. Salin token yang diberikan.

### Cara Mendapatkan Chat ID

1. Cari bot **@userinfobot** di Telegram.
2. Ketik `/start` — bot akan membalas dengan **Chat ID** Anda.

> ⚠️ **Peringatan Keamanan:** Jangan mempublikasikan `token` dan `chat_id` Anda ke repositori publik. Gunakan file konfigurasi terpisah atau environment variable untuk deployment produksi.

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

---
```
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

## 📄 Lisensi

Proyek ini menggunakan lisensi [MIT](LICENSE). Bebas digunakan dan dimodifikasi untuk keperluan pendidikan.

---

## 👥 Tim

Proyek mata kuliah **Pervasive Computing** — Teknik Informatika

---
