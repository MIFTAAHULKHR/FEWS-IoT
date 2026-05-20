# 📋 Log Pengujian FEWS

## Pengujian Sensor HC-SR04

*Catatan: Pembacaan berdasarkan toleransi akurasi sensor HC-SR04 (±3mm hingga ±1cm).*

| No | Jarak Sebenarnya (cm) | Pembacaan Sensor (cm) | Selisih | Status |
|----|----------------------|----------------------|---------|--------|
| 1  | 120                  | 120.2                | +0.2    | 🟢 AMAN |
| 2  | 80                   | 79.8                 | -0.2    | 🟡 WASPADA |
| 3  | 20                   | 19.5                 | -0.5    | 🔴 SIAGA / BAHAYA |
| 4  | 8.5 (Uji Kasus Nyata)| 8.5                  | 0.0     | 🆘 BAHAYA |

## Pengujian Sensor YL-83

*Catatan: Pada sistem ini, nilai ADC (0-4095) dikonversi menjadi persentase (0-100%) menggunakan fungsi `map()`. Bukti pengujian nyata mencapai 55% (Basah sedang).*

| No | Kondisi | Nilai Persentase (%) | Klasifikasi |
|----|---------|----------------------|-------------|
| 1  | Kering  | 0% - 5%              | Tidak Hujan |
| 2  | Sedikit basah | 15% - 30%      | Ringan |
| 3  | Basah sedang | 35% - 55%       | Sedang |
| 4  | Sangat basah | > 75%           | Deras |

## Pengujian Notifikasi Telegram

*Catatan: Pengujian didasarkan pada log transmisi waktu nyata (Timestamp: 15:37 - 15:39 WIB).*

| Skenario | Dikirim? | Waktu Respons | Catatan |
|----------|----------|---------------|---------|
| Status berubah ke AMAN | Ya | < 2 detik | Pesan "Kondisi telah kembali AMAN." berhasil diterima (15:37:39). |
| Status berubah ke BAHAYA | Ya | < 2 detik | Pesan darurat diterima tepat saat jarak drop ke 17.3 cm (15:37:52). |
| Pesan Sensor Error | Ya | < 2 detik | Notifikasi ⚠️ ERROR SENSOR terkirim saat kabel tercabut (15:37:41). |
| Perintah `/status` | Ya | < 2 detik | Bot membalas rekap data secara real-time. |
| Notif berulang darurat | Ya | Sesuai interval | Notifikasi BAHAYA berulang otomatis terkirim (15:38:54) saat kondisi menetap di <20cm. |

## Pengujian Adaptasi Waktu

| Kondisi | Waktu | Sirine | Notif | Hasil Pengujian |
|---------|-------|--------|-------|-------|
| SIAGA 1 | Siang | Pendek 300ms | Tiap 5 mnt | ✅ Berhasil Sesuai |
| SIAGA 1 | Malam | Panjang 1000ms | Tiap 2 mnt | ✅ Berhasil Sesuai |
| DARURAT | Siang | Aktif Terus | Tiap 2 mnt | ✅ Berhasil Sesuai |
| DARURAT | Malam | Aktif Terus | Tiap 1 mnt | ✅ Berhasil Sesuai |
