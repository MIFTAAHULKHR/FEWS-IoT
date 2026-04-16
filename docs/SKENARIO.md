# 🎬 5 Skenario Pervasive FEWS

---

## Skenario 1 — "Waspada Dini Siang Hari"

**Waktu:** Siang (06:00–22:00)  
**Kondisi:** Hujan mulai deras, air mulai naik

```
YL-83 deteksi hujan deras (ADC < 2000)
    ↓
HC-SR04 baca water level WASPADA (50–100cm)
    ↓
LED Kuning nyala
    ↓
Telegram: "⚠️ Waspada! Hujan deras, pantau ketinggian air."
```

---

## Skenario 2 — "Siaga 1 Siang Hari"

**Waktu:** Siang  
**Kondisi:** Air kritis + hujan deras

```
HC-SR04 < 50cm + YL-83 tinggi (ADC < 2000)
    ↓
LED Merah + Sirine pendek (300ms)
    ↓
Telegram URGENT: "🚨 SIAGA 1! Segera amankan barang."
    ↓
Notifikasi ulang tiap 5 menit
```

---

## Skenario 3 — "Siaga 1 Malam Hari" ⭐ Unggulan

**Waktu:** Malam (22:00–06:00)  
**Kondisi:** Air kritis + hujan deras

```
HC-SR04 < 50cm + YL-83 tinggi + NTP = malam
    ↓
LED Merah + Sirine panjang (1000ms) — membangunkan warga
    ↓
Telegram URGENT repeated: "🚨 SIAGA 1 MALAM! Segera evakuasi!"
    ↓
Notifikasi ulang tiap 2 menit hingga kondisi aman
```

> 💡 **Diferensiasi utama:** Sistem sadar bahwa malam hari warga tidur — sirine lebih panjang, notifikasi lebih sering.

---

## Skenario 4 — "Kondisi Membaik / Surut"

**Waktu:** Kapan saja  
**Kondisi:** Air turun, hujan berhenti

```
Sensor kembali ke threshold AMAN
    ↓
Sirine mati otomatis
    ↓
LED Hijau
    ↓
Telegram: "✅ Kondisi aman. Air mulai surut."
```

---

## Skenario 5 — "Cek Status On-Demand"

**Waktu:** Kapan saja  
**Aktor:** Warga aktif

```
Warga kirim /status ke Telegram Bot
    ↓
Sistem balas real-time:

📍 Ketinggian air : 75cm dari sensor (WASPADA)
🌧️ Intensitas hujan: Sedang
🕐 Waktu          : 14:32 WIB (Siang)
📶 Status         : 🟡 WASPADA
```
