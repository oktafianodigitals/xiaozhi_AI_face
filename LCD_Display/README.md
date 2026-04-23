# FaceEngineLcd — Panduan Integrasi

## Gambaran Umum

`FaceEngineLcd` menggantikan seluruh UI pada LCD (bar status, emoji, chat
bubble) dengan animasi wajah tiga-ekspresi yang berjalan murni melalui
primitif LVGL — tidak ada gambar eksternal, tidak ada font khusus.

Cocok untuk semua driver yang terdaftar di `bread-compact-wifi-lcd`:

| Bus  | Driver                            | Resolusi umum              |
|------|-----------------------------------|----------------------------|
| SPI  | GC9A01, ST7789, ILI9341, ST7796   | 240×240, 320×240, 320×320  |
| SPI  | ILI9488                           | 480×320                    |
| RGB  | NT35510, ST7262, EK9716           | 800×480, 480×272           |
| MIPI | ILI9881C, JD9365DA                | 800×1280, 720×1280         |

---

## File yang Disediakan

```
face_engine_lcd.h          ← header utama + semua CALIBRATION macro
face_engine_lcd.cc         ← implementasi animasi
lcd_display_face.h         ← lcd_display.h yang dimodifikasi (ganti file asli)
lcd_display_face.cc        ← lcd_display.cc yang dimodifikasi (ganti file asli)
```

---

## Langkah Integrasi

### 1. Salin file ke proyek

```
xiaozhi-AI\main\display\face_engine_lcd.h
xiaozhi-AI\main\display\face_engine_lcd.cc

# Ganti file asli:
xiaozhi-AI\main\display\lcd_display.h   ← isi dari lcd_display_face.h
xiaozhi-AI\main\display\lcd_display.cc  ← isi dari lcd_display_face.cc
```

### 2. Daftarkan ke CMakeLists.txt

Pastikan `face_engine_lcd.cc` masuk ke daftar sources:

```cmake
idf_component_register(
    SRCS
        "display/face_engine_lcd.cc"
        "display/lcd_display.cc"
        # ... file lain ...
    INCLUDE_DIRS "display"
    REQUIRES lvgl esp_lvgl_port esp_lcd esp_timer esp_psram
)
```

### 3. Tidak ada perubahan pada board file

`SpiLcdDisplay`, `RgbLcdDisplay`, dan `MipiLcdDisplay` mempertahankan
signature konstruktor yang sama persis sehingga board file (mis.
`bread_compact_wifi_lcd.cc`) **tidak perlu diubah**.

### 4. Hubungkan state dari application logic

Panggil `SetFaceState()` kapan pun state AI berubah:

```cpp
// Di mana pun Anda mendapat pointer ke display:
auto* lcd = static_cast<LcdDisplay*>(Board::GetInstance().display());

// Saat mulai mendengarkan input pengguna:
lcd->SetFaceState(FaceState::Listening);

// Saat AI sedang bicara / merespons:
lcd->SetFaceState(FaceState::Speaking);

// Saat idle / menunggu:
lcd->SetFaceState(FaceState::Idle);
```

`SetEmotion()` (dipanggil oleh firmware xiaozhi secara internal) sudah
memetakan string ke `FaceState`:

| emotion string              | FaceState          |
|-----------------------------|--------------------|
| `"listening"`               | `Listening`        |
| `"speaking"`, `"thinking"`, `"loading"` | `Speaking` |
| semua lainnya / `"idle"`    | `Idle`             |

---

## Kalibrasi untuk Driver / Ukuran Berbeda

Semua nilai proporsional ada di bagian **CALIBRATION** pada `face_engine_lcd.h`.
Ubah makro di sana; tidak ada kode lain yang perlu disentuh.

```cpp
// ---- CONTOH untuk GC9A01 240×240 (bulat) ----
#define FACE_EYE_SIZE_RATIO       0.20f   // 20% dari 240 → mata 48 px
#define FACE_EYE_SPACING_X_RATIO  0.22f   // jarak antar mata
#define FACE_EYE_OFFSET_Y_RATIO  -0.12f   // mata sedikit ke atas centre

// ---- CONTOH untuk ILI9341 320×240 (landscape) ----
#define FACE_EYE_SIZE_RATIO       0.22f   // ref = min(320,240)=240 → 52 px
#define FACE_EYE_SPACING_X_RATIO  0.20f
#define FACE_EYE_OFFSET_Y_RATIO  -0.10f
#define FACE_MOUTH_WIDTH_RATIO    0.15f   // mulut sedikit lebih sempit

// ---- CONTOH untuk NT35510 800×480 (landscape besar) ----
#define FACE_EYE_SIZE_RATIO       0.18f   // ref = 480 → 86 px
#define FACE_EYE_SPACING_X_RATIO  0.21f
#define FACE_EYE_OFFSET_Y_RATIO  -0.11f
#define FACE_IRIS_SIZE_RATIO      0.50f   // iris sedikit lebih kecil relatif
```

### Referensi seluruh makro

| Makro                       | Default  | Keterangan                                        |
|-----------------------------|----------|---------------------------------------------------|
| `FACE_EYE_SIZE_RATIO`       | 0.20     | Tinggi mata ÷ min(w,h)                            |
| `FACE_EYE_SPACING_X_RATIO`  | 0.22     | Jarak pusat mata ke tengah layar ÷ min(w,h)       |
| `FACE_EYE_OFFSET_Y_RATIO`   | −0.12    | Offset vertikal mata (negatif = ke atas)           |
| `FACE_EYE_ASPECT_RATIO`     | 0.75     | Lebar mata = tinggi × ratio (< 1 = lonjong)       |
| `FACE_EYE_RADIUS_RATIO`     | 0.20     | Radius sudut mata ÷ tinggi mata                   |
| `FACE_MOUTH_WIDTH_RATIO`    | 0.18     | Lebar mulut ÷ lebar layar                         |
| `FACE_MOUTH_OFFSET_Y_RATIO` | 0.28     | Jarak mulut ke bawah dari tengah layar            |
| `FACE_MOUTH_RADIUS_RATIO`   | 0.35     | Radius sudut mulut ÷ tinggi mulut                 |
| `FACE_ENABLE_IRIS`          | 1        | 1 = gambar iris+pupil berwarna, 0 = mata polos    |
| `FACE_IRIS_SIZE_RATIO`      | 0.55     | Diameter iris ÷ tinggi mata                       |
| `FACE_PUPIL_SIZE_RATIO`     | 0.45     | Diameter pupil ÷ diameter iris                    |
| `FACE_UPDATE_INTERVAL_MS`   | 50       | Interval timer LVGL (ms) ≈ framerate              |
| `FACE_BLINK_PERIOD_FRAMES`  | 120      | Rata-rata frame antara kedip                      |
| `FACE_SPEAK_UPDATE_MIN_MS`  | 80       | Interval minimum perubahan target mulut (ms)      |
| `FACE_SPEAK_UPDATE_RAND_MS` | 60       | Rentang random tambahan (ms)                      |
| `FACE_IDLE_DRIFT_PERIOD_FRAMES` | 40   | Frame antara perpindahan gaze idle                |

### Warna

```cpp
#define FACE_COLOR_BACKGROUND  lv_color_hex(0x000000)  // hitam
#define FACE_COLOR_FEATURE     lv_color_hex(0xFFFFFF)  // mata & mulut putih
#define FACE_COLOR_IRIS        lv_color_hex(0x00BFFF)  // biru terang
#define FACE_COLOR_PUPIL       lv_color_hex(0x000000)  // hitam
```

Untuk layar bulat (GC9A01) atau panel AMOLED, latar hitam penuh
menghasilkan kontras paling tajam dan hemat daya.

---

## Cara Kerja Animasi

### Idle
- Kedua mata ukuran penuh, sedikit di atas tengah layar.
- Setiap ~2 detik gaze drift acak (offset ±4% layar).
- Iris mengikuti arah pandang.
- Mulut tipis horizontal.
- Kedip acak ~6 detik sekali (3 frame: tutup sebagian → hampir tutup → buka cepat).

### Listening
- Mata kanan sedikit lebih besar dari kiri (asimetri "penasaran/memperhatikan").
- Kedua mata terpusat, tidak ada gaze drift.
- Mulut sangat tipis dan sempit (ekspresi fokus).
- Kedip tetap aktif.

### Speaking
- Kedua mata ukuran penuh, simetris.
- Tinggi mulut beranimasi acak antara hampir tutup → terbuka penuh.
- Transisi mulut smooth (interpolasi per-frame).
- Kedip tetap aktif.

---

## Kompatibilitas

- **LVGL v8 dan v9** — menggunakan API publik standar (`lv_obj_create`,
  `lv_obj_set_pos`, `lv_timer_create`, dll.).
- **ESP-IDF ≥ 5.0**.
- **Semua `esp_lcd` driver** yang terdaftar via `SpiLcdDisplay`,
  `RgbLcdDisplay`, atau `MipiLcdDisplay`.
- Tidak ada dependensi gambar, font, atau GIF.
