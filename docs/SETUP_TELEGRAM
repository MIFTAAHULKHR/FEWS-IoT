# 📱 Setup Telegram Bot untuk FEWS

## Langkah 1 — Buat Bot Baru

1. Buka Telegram, cari `@BotFather`
2. Kirim perintah `/newbot`
3. Ikuti instruksi: masukkan nama bot dan username (harus diakhiri `bot`)
4. Salin **Token API** yang diberikan, contoh:
   ```
   1234567890:ABCDefGhIJKlmNoPQRsTUVwxyZ
   ```

## Langkah 2 — Dapatkan Chat ID

1. Kirim pesan apa saja ke bot kamu
2. Buka URL berikut di browser (ganti TOKEN):
   ```
   https://api.telegram.org/bot<TOKEN>/getUpdates
   ```
3. Cari nilai `"chat":{"id":XXXXXXXX}` — itulah Chat ID kamu

## Langkah 3 — Isi Konfigurasi di Firmware

```cpp
#define TELEGRAM_TOKEN   "1234567890:ABCDefGhIJKlmNoPQRsTUVwxyZ"
#define TELEGRAM_CHAT_ID "123456789"
```

## Langkah 4 — Test Koneksi

Upload firmware, buka Serial Monitor. Jika berhasil kamu akan melihat:
```
[WiFi] Terhubung! IP: 192.168.x.x
[NTP] Sinkronisasi waktu...
[FEWS] Sistem siap.
```
Dan bot Telegram akan mengirim pesan startup.

---

## 🔐 Tips Keamanan

- **Jangan commit token ke GitHub!** Tambahkan `config.h` ke `.gitignore`
- Buat file `src/config.h` terpisah:
  ```cpp
  #pragma once
  #define WIFI_SSID        "namawifi"
  #define WIFI_PASSWORD    "passwordwifi"
  #define TELEGRAM_TOKEN   "token"
  #define TELEGRAM_CHAT_ID "chatid"
  ```
- Di `main.cpp`, ganti define dengan `#include "config.h"`
