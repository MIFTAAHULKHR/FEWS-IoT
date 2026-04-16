# 📋 Log Pengujian FEWS

## Pengujian Sensor HC-SR04

| No | Jarak Sebenarnya (cm) | Pembacaan Sensor (cm) | Selisih | Status |
|----|----------------------|----------------------|---------|--------|
| 1  | 120                  | —                    | —       | AMAN |
| 2  | 80                   | —                    | —       | WASPADA |
| 3  | 40                   | —                    | —       | SIAGA 1 |
| 4  | 15                   | —                    | —       | DARURAT |

## Pengujian Sensor YL-83

| No | Kondisi | Nilai ADC | Klasifikasi |
|----|---------|-----------|-------------|
| 1  | Kering  | —         | Tidak Hujan |
| 2  | Sedikit basah | —   | Ringan |
| 3  | Basah sedang | —    | Sedang |
| 4  | Sangat basah | —    | Deras |

## Pengujian Notifikasi Telegram

| Skenario | Dikirim? | Waktu Respons | Catatan |
|----------|----------|---------------|---------|
| Status berubah ke WASPADA | — | — | — |
| Status berubah ke SIAGA 1 siang | — | — | — |
| Status berubah ke SIAGA 1 malam | — | — | — |
| Perintah /status | — | — | — |
| Notif berulang darurat | — | — | — |

## Pengujian Adaptasi Waktu

| Kondisi | Waktu | Sirine | Notif | Hasil |
|---------|-------|--------|-------|-------|
| SIAGA 1 | Siang | Pendek 300ms | Tiap 5 mnt | — |
| SIAGA 1 | Malam | Panjang 1000ms | Tiap 2 mnt | — |
| DARURAT | Siang | Aktif | Tiap 2 mnt | — |
| DARURAT | Malam | Aktif terus | Tiap 1 mnt | — |
