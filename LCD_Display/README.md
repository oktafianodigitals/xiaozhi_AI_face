# FaceEngineLcd — Panduan Lengkap

Animasi wajah AI untuk **LCD color** (LVGL) pada board `bread-compact-wifi-lcd`.  
Script ini adalah versi LCD dari `face_engine` yang sebelumnya hanya mendukung OLED 128×64.

---

## Daftar Isi

1. [Gambaran Cara Kerja](#cara-kerja)
2. [File yang Dibuat](#file-yang-dibuat)
3. [Struktur Proyek Setelah Integrasi](#struktur-proyek)
4. [Driver LCD yang Didukung](#driver-yang-didukung)
5. [Langkah Integrasi ke lcd_display.cc](#integrasi)
6. [Panduan Kalibrasi](#kalibrasi)
7. [Referensi Nilai Konfigurasi](#referensi-konfigurasi)
8. [Alur Animasi Per State](#alur-animasi)
9. [Troubleshooting](#troubleshooting)

---

## Cara Kerja

```
lcd_display.cc → SetStatus("listening")
       ↓
FaceEngineLcd::SetState(FaceStateLcd::Listening)
       ↓
LVGL Timer (setiap 40ms) → FaceEngineLcd::Update()
       ↓
ListeningBehavior(eye_h) → ubah ukuran & posisi LVGL object
       ↓
LVGL refresh → tampil di LCD
```

Seluruh animasi berjalan lewat **LVGL timer** yang dipasang saat `Init()`.  
Tidak ada loop manual — tidak mengganggu FreeRTOS task lain.

### Tiga State Animasi

| State | Mata | Mulut | Keterangan |
|---|---|---|---|
| **Idle** | Bergerak pelan (drift), berkedip acak | Tipis, diam | Standby / menunggu |
| **Listening** | Asimetris (kanan lebih besar) | Kecil, sedikit ke kiri | Sedang mendengarkan user |
| **Speaking** | Stabil | Bergerak naik-turun acak | AI sedang berbicara |

---

## File yang Dibuat

| File | Fungsi |
|---|---|
| `display/face_engine_lcd.h` | Header: deklarasi class, struct konfigurasi, enum state |
| `display/face_engine_lcd.cc` | Implementasi: animasi + fungsi konfigurasi per driver |

---

## Struktur Proyek

Setelah integrasi, tempatkan file di:

```
xiaozhi-esp32/
└── main/
    └── display/
        ├── face_engine_lcd.h       ← baru
        ├── face_engine_lcd.cc      ← baru
        ├── face_engine.h           (OLED, tidak diubah)
        ├── face_engine.cc          (OLED, tidak diubah)
        ├── lcd_display.h           (perlu sedikit tambahan — lihat bagian Integrasi)
        └── lcd_display.cc          (perlu tambahan SetStatus + Init — lihat Integrasi)
```

---

## Driver yang Didukung

Script ini memiliki konfigurasi bawaan untuk semua driver di `bread-compact-wifi-lcd/config.h`:

| Makro `config.h` | Driver | Resolusi |
|---|---|---|
| `CONFIG_LCD_ST7789_240X320` | ST7789 | 240 × 320 |
| `CONFIG_LCD_ST7789_240X320_NO_IPS` | ST7789 | 240 × 320 |
| `CONFIG_LCD_ST7789_170X320` | ST7789 | 170 × 320 |
| `CONFIG_LCD_ST7789_172X320` | ST7789 | 172 × 320 |
| `CONFIG_LCD_ST7789_240X280` | ST7789 | 240 × 280 |
| `CONFIG_LCD_ST7789_240X240` | ST7789 | 240 × 240 |
| `CONFIG_LCD_ST7789_240X240_7PIN` | ST7789 | 240 × 240 |
| `CONFIG_LCD_ST7789_240X135` | ST7789 | 240 × 135 |
| `CONFIG_LCD_ST7735_128X160` | ST7735 | 128 × 160 |
| `CONFIG_LCD_ST7735_128X128` | ST7735 | 128 × 128 |
| `CONFIG_LCD_ST7796_320X480` | ST7796 | 320 × 480 |
| `CONFIG_LCD_ST7796_320X480_NO_IPS` | ST7796 | 320 × 480 |
| `CONFIG_LCD_ILI9341_240X320` | ILI9341 | 240 × 320 |
| `CONFIG_LCD_ILI9341_240X320_NO_IPS` | ILI9341 | 240 × 320 |
| `CONFIG_LCD_GC9A01_240X240` | GC9A01 | 240 × 240 (bulat) |
| `CONFIG_LCD_CUSTOM` | Custom | Otomatis proporsional |

---

## Integrasi

### 1. Tambahkan include di `lcd_display.cc`

Di bagian paling atas `lcd_display.cc`, tambahkan:

```cpp
#include "face_engine_lcd.h"
```

### 2. Tambahkan variabel instance di dalam class LcdDisplay (di `lcd_display.h`)

Di dalam `class LcdDisplay` (section `protected` atau `private`):

```cpp
// Tambahkan di lcd_display.h, dalam class LcdDisplay
FaceEngineLcd* face_engine_lcd_ = nullptr;
```

### 3. Inisialisasi di `LcdDisplay::SetupUI()`

Temukan fungsi `SetupUI()` di `lcd_display.cc`. Di bagian akhir fungsi tersebut (setelah semua widget dibuat), tambahkan:

```cpp
// ── Inisialisasi FaceEngineLcd ──────────────────────────────────────────
// Pilih area tampilan wajah: bisa menggunakan emoji_box_ atau container_
// Ganti emoji_box_ dengan objek LVGL mana pun yang jadi "kanvas" wajah Anda

FaceLcdConfig face_cfg = FaceEngineLcd::MakeDefaultConfig(width_, height_);

// OPSIONAL: override warna agar sesuai tema aktif
auto* lvgl_theme = static_cast<LvglTheme*>(current_theme_);
face_cfg.color_bg    = lvgl_theme->background_color();
face_cfg.color_eye   = lvgl_theme->text_color();
face_cfg.color_mouth = lvgl_theme->text_color();

face_engine_lcd_ = new FaceEngineLcd();
face_engine_lcd_->Init(emoji_box_, face_cfg);
// ────────────────────────────────────────────────────────────────────────
```

> **Catatan:** `emoji_box_` adalah objek centered di layar pada SetupUI() default.  
> Jika Anda ingin wajah tampil di area lain, ganti dengan objek LVGL yang sesuai.

### 4. Tambahkan `SetStatus()` di `lcd_display.cc`

Tambahkan fungsi ini (override dari class Display) ke dalam `lcd_display.cc`:

```cpp
#include "assets/lang_config.h"   // pastikan sudah ada

void LcdDisplay::SetStatus(const char* status) {
    // Teruskan ke base class untuk logging & logika lain
    Display::SetStatus(status);

    if (!face_engine_lcd_) return;

    if (strcmp(status, Lang::Strings::STANDBY) == 0) {
        face_engine_lcd_->SetState(FaceStateLcd::Idle);
    } else if (strcmp(status, Lang::Strings::LISTENING) == 0) {
        face_engine_lcd_->SetState(FaceStateLcd::Listening);
    } else if (strcmp(status, Lang::Strings::SPEAKING) == 0) {
        face_engine_lcd_->SetState(FaceStateLcd::Speaking);
    } else {
        face_engine_lcd_->SetState(FaceStateLcd::Idle);
    }
}
```

### 5. Deklarasikan `SetStatus()` di `lcd_display.h`

Di dalam `class LcdDisplay`, tambahkan:

```cpp
virtual void SetStatus(const char* status) override;
```

### 6. Daftarkan file ke CMakeLists.txt

Di file `CMakeLists.txt` (di dalam folder `display/` atau folder parent-nya):

```cmake
set(SOURCES
    ...
    "display/face_engine_lcd.cc"   # tambahkan baris ini
    ...
)
```

---

## Kalibrasi

### Kalibrasi Cepat (disarankan)

Buka `face_engine_lcd.cc` dan temukan fungsi konfigurasi yang sesuai driver Anda.  
Contoh: jika menggunakan ST7789 240×320, cari `MakeConfig_ST7789_240x320()`.

Ubah nilai-nilai di dalam fungsi tersebut, lalu compile & flash.  
**Tidak ada kode lain yang perlu diubah.**

### Parameter yang Paling Sering Dikalibrasi

```
eye_size        → ukuran dasar mata (px)
eye_gap         → jarak antara kedua mata
eye_offset_y    → posisi vertikal mata (negatif = ke atas)
mouth_offset_y  → posisi vertikal mulut (positif = ke bawah)
mouth_idle_w    → lebar mulut saat idle
mouth_speak_w   → lebar mulut saat speaking
speak_h_large   → tinggi maksimum mulut saat speaking
color_eye       → warna mata (ganti dengan lv_color_hex(0xRRGGBB))
color_bg        → warna background wajah
```

### Menambahkan Driver LCD Baru

1. Buat fungsi konfigurasi baru di `face_engine_lcd.cc`:

```cpp
static FaceLcdConfig MakeConfig_DriverBaru_WxH() {
    FaceLcdConfig c;
    c.canvas_w = W;
    c.canvas_h = H;
    // ... isi semua nilai
    return c;
}
```

2. Tambahkan entry di `FaceEngineLcd::MakeDefaultConfig()`:

```cpp
#elif defined(CONFIG_LCD_DRIVER_BARU)
    return MakeConfig_DriverBaru_WxH();
```

3. Tambahkan makro yang sesuai di `config.h` board Anda.

---

## Referensi Konfigurasi

### FaceLcdConfig — semua field

| Field | Tipe | Default (240×320) | Keterangan |
|---|---|---|---|
| `canvas_w` | `int` | 240 | Lebar area wajah (px) |
| `canvas_h` | `int` | 320 | Tinggi area wajah (px) |
| `eye_size` | `int` | 60 | Ukuran mata default |
| `eye_radius` | `int` | 10 | Radius sudut mata (0=kotak) |
| `eye_gap` | `int` | 30 | Jarak pusat ke pinggir mata |
| `eye_offset_y` | `int` | -50 | Offset Y mata dari center |
| `mouth_offset_y` | `int` | 80 | Offset Y mulut dari center |
| `mouth_idle_w` | `int` | 80 | Lebar mulut Idle |
| `mouth_idle_h` | `int` | 14 | Tinggi mulut Idle |
| `mouth_idle_r` | `int` | 7 | Radius mulut Idle |
| `mouth_listen_w` | `int` | 40 | Lebar mulut Listening |
| `mouth_listen_h` | `int` | 8 | Tinggi mulut Listening |
| `mouth_listen_r` | `int` | 4 | Radius mulut Listening |
| `mouth_speak_w` | `int` | 65 | Lebar mulut Speaking |
| `mouth_speak_r` | `int` | 16 | Radius mulut Speaking |
| `speak_h_small` | `int` | 8 | Tinggi mulut kecil (speaking) |
| `speak_h_mid1` | `int` | 24 | Tinggi mulut medium 1 |
| `speak_h_mid2` | `int` | 42 | Tinggi mulut medium 2 |
| `speak_h_large` | `int` | 60 | Tinggi mulut maksimum |
| `speak_step` | `int` | 6 | Langkah smooth mouth per frame |
| `speak_interval_base` | `int` | 80 | Interval min pergantian target (ms) |
| `speak_interval_rand` | `int` | 80 | Rentang acak tambahan (ms) |
| `idle_drift_x` | `int` | 10 | Maksimum drift X mata (px) |
| `idle_drift_y` | `int` | 6 | Maksimum drift Y mata (px) |
| `idle_drift_chance` | `int` | 35 | 1-dari-N peluang drift baru |
| `blink_chance` | `int` | 100 | 1-dari-N peluang mulai kedip |
| `eye_blink_h1` | `int` | 10 | Tinggi mata fase kedip 1 |
| `eye_blink_h2` | `int` | 1 | Tinggi mata fase kedip 2 (tertutup) |
| `eye_blink_h3` | `int` | 18 | Tinggi mata fase buka kembali |
| `color_eye` | `lv_color_t` | white | Warna mata |
| `color_mouth` | `lv_color_t` | white | Warna mulut |
| `color_bg` | `lv_color_t` | black | Warna background |
| `update_interval_ms` | `uint32_t` | 40 | Interval timer animasi (ms) |

---

## Alur Animasi

### Idle

```
Update() setiap 40ms
  ↓
blink_phase_: 0 → periksa peluang kedip (1/100)
  ↓ (jika kedip)
frame 1: eye_h = eye_blink_h1  (setengah tutup)
frame 2: eye_h = eye_blink_h2  (tutup)
frame 3: eye_h = eye_blink_h3  (buka)
frame 4+: eye_h = eye_size     (normal)
  ↓
IdleBehavior(eye_h):
  - drift baru dipilih acak (peluang 1/idle_drift_chance)
  - mata bergerak ke posisi drift
  - mulut diam di mouth_idle_*
```

### Listening

```
Update() setiap 40ms
  ↓ (blink tetap berjalan)
ListeningBehavior(eye_h):
  - mata kiri: height - eye_size/8  (lebih kecil)
  - mata kanan: height normal
  - mulut: mouth_listen_* (kecil, sedikit ke kiri)
```

### Speaking

```
Update() setiap 40ms
  ↓ (blink tetap berjalan)
SpeakingBehavior(eye_h):
  - setiap speak_interval ms: pilih speak_target_h_ acak
  - speak_current_h_ ± speak_step mendekati target (smooth)
  - mulut: width=mouth_speak_w, height=speak_current_h_
```

---

## Troubleshooting

**Wajah tidak muncul / layar gelap**  
→ Pastikan `SetupUI()` sudah dipanggil sebelum `Init()`  
→ Cek parent object yang dilewatkan ke `Init()` tidak `nullptr`

**Wajah terlalu besar / terpotong**  
→ Kecilkan `eye_size`, `eye_gap`, dan `mouth_*_w` di fungsi MakeConfig  
→ Sesuaikan `eye_offset_y` dan `mouth_offset_y`

**Animasi terlalu cepat / lambat**  
→ Ubah `update_interval_ms` (lebih besar = lebih lambat)  
→ Untuk speaking: ubah `speak_step` dan `speak_interval_base`

**Kedipan terlalu sering / jarang**  
→ Ubah `blink_chance`: lebih kecil = lebih sering, lebih besar = lebih jarang

**Error compile: `Lang::Strings::STANDBY` tidak ditemukan**  
→ Pastikan `#include "assets/lang_config.h"` ada di lcd_display.cc  
→ Cek file lang_config.h di proyek Anda untuk nama string yang tepat

**Wajah tidak berubah state saat berbicara**  
→ Pastikan `SetStatus()` terhubung ke state machine di `application.cc`  
→ Tambahkan log: `ESP_LOGI(TAG, "SetStatus: %s", status)` untuk debug

**GC9A01 (layar bulat): wajah terpotong di sudut**  
→ Kecilkan `eye_size` dan `mouth_*_w`  
→ Geser `eye_offset_y` dan `mouth_offset_y` lebih mendekati 0  

---

## Catatan Penting

- `FaceEngineLcd` dan `FaceEngine` (OLED) **dapat berjalan bersamaan** di proyek yang berbeda. Keduanya menggunakan enum name berbeda (`FaceStateLcd` vs `FaceState`) untuk menghindari konflik.
- Instance `FaceEngineLcd` di-`new` di heap — pastikan tidak di-`delete` selama program berjalan.
- Semua operasi LVGL di dalam engine sudah di dalam LVGL task context (via timer), sehingga **thread-safe terhadap LVGL**.
- Untuk mengubah warna wajah saat runtime (misalnya ikut tema), gunakan `face_engine_lcd_->GetConfig()` atau buat ulang dengan `Init()` — namun `Init()` sebaiknya hanya dipanggil sekali.
