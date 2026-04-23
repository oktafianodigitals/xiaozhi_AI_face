# Xiaozhi-AI Display Custom — Panduan Lengkap

> Panduan ini menjelaskan arsitektur sistem display pada proyek **xiaozhi-esp32**, apa saja yang diubah dari script original, cara kerja masing-masing custom display, serta panduan langkah demi langkah untuk membuat custom display baru.

---

## Daftar Isi

1. [Gambaran Umum Struktur Proyek](#1-gambaran-umum-struktur-proyek)
2. [Arsitektur Sistem Display (Original)](#2-arsitektur-sistem-display-original)
3. [Perbedaan Original vs Custom](#3-perbedaan-original-vs-custom)
   - [3.1 Custom LCD Color — `LCD_Display/`](#31-custom-lcd-color--lcd_display)
   - [3.2 Custom OLED I2C 128×64 — `128x64/`](#32-custom-oled-i2c-128×64--128x64)
4. [Cara Kerja Animasi Wajah](#4-cara-kerja-animasi-wajah)
   - [4.1 Tiga State Animasi](#41-tiga-state-animasi)
   - [4.2 Alur Data: dari `SetStatus()` ke Layar](#42-alur-data-dari-setstatus-ke-layar)
   - [4.3 Sistem LVGL Timer](#43-sistem-lvgl-timer)
5. [Penjelasan File per Direktori](#5-penjelasan-file-per-direktori)
6. [Panduan Integrasi ke Proyek](#6-panduan-integrasi-ke-proyek)
   - [6.1 Integrasi LCD Color (`LCD_Display`)](#61-integrasi-lcd-color-lcd_display)
   - [6.2 Integrasi OLED 128×64 (`128x64`)](#62-integrasi-oled-128×64-128x64)
7. [Panduan Kalibrasi Visual](#7-panduan-kalibrasi-visual)
8. [Cara Membuat Custom Display Baru](#8-cara-membuat-custom-display-baru)
   - [8.1 Custom untuk Driver LCD Baru](#81-custom-untuk-driver-lcd-baru)
   - [8.2 Custom untuk Tipe Display Baru](#82-custom-untuk-tipe-display-baru)
9. [Referensi Parameter `FaceLcdConfig`](#9-referensi-parameter-facelcdconfig)
10. [Referensi Driver LCD yang Didukung](#10-referensi-driver-lcd-yang-didukung)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Gambaran Umum Struktur Proyek

```
xiaozhi-esp32/
└── main/
    ├── display/              ← Direktori ORIGINAL (tidak diubah)
    │   ├── display.h         ← Base class Display
    │   ├── display.cc
    │   ├── lcd_display.h     ← LCD Color display (SPI/RGB/MIPI)
    │   ├── lcd_display.cc
    │   ├── oled_display.h    ← OLED display (I2C)
    │   ├── oled_display.cc
    │   ├── emote_display.*
    │   └── lvgl_display/     ← LVGL rendering backend
    │       ├── lvgl_display.*
    │       ├── emoji_collection.*
    │       ├── lvgl_theme.*
    │       ├── gif/
    │       └── jpg/
    │
    ├── LCD_Display/          ← Custom: animasi wajah untuk LCD COLOR
    │   ├── face_engine_lcd.h    ← Header: class, config struct, enum
    │   ├── face_engine_lcd.cc   ← Implementasi + konfigurasi per driver
    │   └── README.md
    │
    └── 128x64/               ← Custom: animasi wajah untuk OLED I2C 128×64
        ├── face_engine.h        ← Header: class simpel tanpa config struct
        ├── face_engine.cc       ← Implementasi hardcoded untuk 128×64
        ├── oled_display.h       ← Override: tambah SetStatus() + FaceEngine
        └── oled_display.cc      ← Override: ganti SetupUI_128x64() + SetStatus()
```

> **Catatan penting:** Direktori `display/` adalah kode original upstream. **Jangan edit langsung.** Semua modifikasi dilakukan di direktori custom (`LCD_Display/` dan `128x64/`) yang file-filenya akan menggantikan atau melengkapi file original saat integrasi.

---

## 2. Arsitektur Sistem Display (Original)

Sebelum memahami apa yang diubah, penting untuk mengetahui bagaimana sistem original bekerja.

### Hierarki Class

```
Display                         ← Base class (display.h)
│   - SetStatus()               ← hanya logging, tidak ada animasi
│   - SetEmotion()
│   - SetChatMessage()
│   - ShowNotification()
│
├── LvglDisplay                 ← Abstraksi LVGL (lvgl_display.h)
│   │
│   ├── LcdDisplay              ← LCD Color SPI/RGB/MIPI (lcd_display.h)
│   │   ├── SpiLcdDisplay
│   │   ├── RgbLcdDisplay
│   │   └── MipiLcdDisplay
│   │
│   └── OledDisplay             ← OLED I2C Monochrome (oled_display.h)
│
└── EmoteDisplay                ← Display mode teks/emote (emote_display.h)
```

### Apa yang Original TIDAK Punya

Di versi original:
- `SetStatus()` pada `Display` base class hanya mencetak log ke serial — **tidak ada animasi wajah**.
- `OledDisplay` dan `LcdDisplay` mewarisi `SetStatus()` yang kosong tersebut.
- Tidak ada file `face_engine.h`, `face_engine.cc`, `face_engine_lcd.h`, maupun `face_engine_lcd.cc`.

---

## 3. Perbedaan Original vs Custom

### 3.1 Custom LCD Color — `LCD_Display/`

Direktori ini menambahkan **dua file baru** yang tidak ada di original sama sekali:

| File | Status | Fungsi |
|---|---|---|
| `face_engine_lcd.h` | **Baru** | Deklarasi `FaceEngineLcd`, `FaceLcdConfig`, `FaceStateLcd` |
| `face_engine_lcd.cc` | **Baru** | Implementasi animasi + konfigurasi untuk setiap driver LCD |

**Perubahan yang perlu dilakukan pada file original:**

| File Original | Jenis Perubahan | Detail |
|---|---|---|
| `lcd_display.h` | Tambah member | `FaceEngineLcd* face_engine_lcd_ = nullptr;` di class `LcdDisplay` |
| `lcd_display.h` | Tambah deklarasi | `virtual void SetStatus(const char* status) override;` |
| `lcd_display.cc` | Tambah include | `#include "face_engine_lcd.h"` |
| `lcd_display.cc` | Tambah fungsi | Implementasi `LcdDisplay::SetStatus()` yang mengubah state animasi |
| `lcd_display.cc` | Edit `SetupUI()` | Tambahkan inisialisasi `FaceEngineLcd` di akhir fungsi |
| `CMakeLists.txt` | Tambah source | `"display/face_engine_lcd.cc"` |

**Ringkasan:** Versi original tidak memiliki animasi wajah pada LCD color. Custom ini menambahkan engine animasi wajah yang bekerja melalui LVGL dan dapat dikalibrasi per driver/resolusi.

---

### 3.2 Custom OLED I2C 128×64 — `128x64/`

Direktori ini memuat **dua file baru** dan **dua file pengganti** (replacement) untuk file original:

| File | Status | Fungsi |
|---|---|---|
| `face_engine.h` | **Baru** | Deklarasi `FaceEngine` versi OLED — simpel, hardcoded 128×64 |
| `face_engine.cc` | **Baru** | Implementasi animasi untuk OLED monokrom |
| `oled_display.h` | **Pengganti** | Override `OledDisplay` — tambah deklarasi `SetStatus()` |
| `oled_display.cc` | **Pengganti** | Override `OledDisplay` — tambah `FaceEngine`, ganti `SetupUI_128x64()`, tambah `SetStatus()` |

**Perubahan spesifik pada `oled_display.h`:**

```diff
  // Versi original tidak punya:
+ void SetStatus(const char* status) override;
```

**Perubahan spesifik pada `oled_display.cc`:**

```diff
  // Tambahan di awal file:
+ #include "face_engine.h"
+ FaceEngine* face_engine_ = nullptr;

  // Fungsi baru (tidak ada di original):
+ void OledDisplay::SetStatus(const char* status) { ... }

  // SetupUI_128x64() diubah total:
  // Original: membuat UI dengan emotion_label_ dan chat_message_label_
  //           (layout teks biasa dengan ikon emosi)
+ // Custom:   menggantinya dengan inisialisasi FaceEngine (animasi wajah)
+             face_engine_ = new FaceEngine();
+             face_engine_->Init(content_);
```

**Ringkasan perbandingan tampilan:**

| Aspek | Original 128×64 | Custom 128×64 |
|---|---|---|
| Tampilan utama | Label teks emoji | Animasi wajah (mata + mulut) |
| Respon `SetStatus()` | Tidak ada | Ubah state animasi (Idle/Listening/Speaking) |
| Kedipan mata | Tidak ada | Ada (acak, probabilistik) |
| Gerakan mulut | Tidak ada | Ada (saat Speaking) |
| Drift mata | Tidak ada | Ada (saat Idle) |

---

## 4. Cara Kerja Animasi Wajah

### 4.1 Tiga State Animasi

Kedua custom (LCD dan OLED) menggunakan konsep tiga state yang sama, dipicu oleh string status dari `Application`:

| State | Dipicu oleh | Perilaku Mata | Perilaku Mulut |
|---|---|---|---|
| **Idle** | `Lang::Strings::STANDBY` | Bergerak pelan acak (drift), berkedip | Tipis, diam, ikut drift |
| **Listening** | `Lang::Strings::LISTENING` | Asimetris — kanan lebih besar dari kiri | Kecil, sedikit ke kiri |
| **Speaking** | `Lang::Strings::SPEAKING` | Stabil, tidak drift | Naik-turun acak (smooth) |

### 4.2 Alur Data: dari `SetStatus()` ke Layar

```
Application (application.cc)
    │
    │  SetStatus("standby" / "listening" / "speaking")
    ▼
Display::SetStatus()              ← base class (hanya logging)
    │
    ▼
LcdDisplay::SetStatus()           ← custom override
    │   atau
OledDisplay::SetStatus()          ← custom override
    │
    │  SetState(FaceStateLcd::Idle / Listening / Speaking)
    ▼
FaceEngineLcd / FaceEngine
    │
    │  (state tersimpan, bukan langsung render)
    ▼
LVGL Timer (setiap 40ms)
    │
    ▼
FaceEngineLcd::Update()
    │
    ├── Hitung blink_phase_ (kedipan)
    │
    └── Dispatch ke behavior:
        ├── IdleBehavior()      → drift + kedip + mulut diam
        ├── ListeningBehavior() → mata asimetris + mulut kecil
        └── SpeakingBehavior()  → mata stabil + mulut bergerak
            │
            ▼
        lv_obj_set_size() / lv_obj_align()   ← update LVGL object
            │
            ▼
        LVGL refresh → driver LCD/OLED → layar fisik
```

### 4.3 Sistem LVGL Timer

Animasi **tidak menggunakan loop manual** atau FreeRTOS task tersendiri. Ini penting untuk efisiensi.

```cpp
// Dipasang satu kali di Init():
lv_timer_create(
    [](lv_timer_t* t) {
        auto* engine = static_cast<FaceEngineLcd*>(lv_timer_get_user_data(t));
        engine->Update();          // dipanggil setiap 40ms
    },
    cfg_.update_interval_ms,       // default: 40ms ≈ 25 fps
    this
);
```

**Keuntungan sistem timer LVGL:**
- Berjalan di dalam LVGL task — otomatis thread-safe terhadap LVGL
- Tidak membebani FreeRTOS scheduler dengan task tambahan
- Mudah dikontrol melalui `update_interval_ms`

---

## 5. Penjelasan File per Direktori

### `display/` (Original — Jangan Diubah)

| File | Fungsi |
|---|---|
| `display.h / .cc` | Base class abstrak. Mendefinisikan interface virtual: `SetStatus`, `SetEmotion`, `SetChatMessage`, dll. |
| `lcd_display.h / .cc` | Implementasi LCD color. Mendukung SPI, RGB, MIPI. Menangani tema (light/dark), preview image, GIF/emoji. |
| `oled_display.h / .cc` | Implementasi OLED I2C monokrom. Mendukung 128×64 dan 128×32. UI teks default. |
| `emote_display.h / .cc` | Mode display berbasis teks/emote (tanpa LVGL). |
| `lvgl_display/lvgl_display.*` | Backend LVGL: init display, mutex, semaphore. |
| `lvgl_display/emoji_collection.*` | Koleksi emoji LVGL. |
| `lvgl_display/lvgl_theme.*` | Sistem tema (warna, font). |
| `lvgl_display/gif/` | GIF player untuk animasi emoji. |
| `lvgl_display/jpg/` | Decoder JPEG untuk preview image. |

---

### `LCD_Display/` (Custom — LCD Color)

| File | Fungsi |
|---|---|
| `face_engine_lcd.h` | Mendefinisikan `FaceStateLcd` (enum), `FaceLcdConfig` (struct konfigurasi), dan `FaceEngineLcd` (class utama). |
| `face_engine_lcd.cc` | Semua implementasi: fungsi `MakeConfig_*()` per driver, `Init()`, `SetState()`, `Update()`, dan tiga behavior function. |

---

### `128x64/` (Custom — OLED I2C Monokrom)

| File | Fungsi |
|---|---|
| `face_engine.h` | Deklarasi `FaceState` (enum simpel) dan `FaceEngine` (class — parameter hardcoded untuk 128×64). |
| `face_engine.cc` | Implementasi animasi untuk layar 128×64. Nilai hardcoded, tidak ada config struct. |
| `oled_display.h` | Pengganti `display/oled_display.h`. Penambahan: deklarasi `SetStatus()` di bagian private. |
| `oled_display.cc` | Pengganti `display/oled_display.cc`. Penambahan: include `face_engine.h`, global pointer, `SetStatus()`, dan implementasi `SetupUI_128x64()` yang hanya menampilkan wajah. |

---

## 6. Panduan Integrasi ke Proyek

### 6.1 Integrasi LCD Color (`LCD_Display`)

#### Langkah 1 — Copy file ke direktori display

```bash
cp LCD_Display/face_engine_lcd.h   main/display/
cp LCD_Display/face_engine_lcd.cc  main/display/
```

#### Langkah 2 — Edit `lcd_display.h`

Tambahkan forward declaration dan member di dalam class `LcdDisplay`:

```cpp
// Di bagian atas file, sebelum class LcdDisplay:
#include "face_engine_lcd.h"

// Di dalam class LcdDisplay (section protected atau private):
FaceEngineLcd* face_engine_lcd_ = nullptr;

// Di dalam class LcdDisplay (section public):
virtual void SetStatus(const char* status) override;
```

#### Langkah 3 — Edit `lcd_display.cc`

Tambahkan include di bagian atas:

```cpp
#include "face_engine_lcd.h"
```

Tambahkan implementasi `SetStatus()` — bisa ditempatkan setelah konstruktor:

```cpp
void LcdDisplay::SetStatus(const char* status) {
    Display::SetStatus(status);   // tetap panggil base untuk logging

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

Di akhir fungsi `LcdDisplay::SetupUI()`, tambahkan inisialisasi engine:

```cpp
// Ambil konfigurasi otomatis berdasarkan driver dari config.h
FaceLcdConfig face_cfg = FaceEngineLcd::MakeDefaultConfig(width_, height_);

// Opsional: sesuaikan warna dengan tema aktif
auto* lvgl_theme = static_cast<LvglTheme*>(current_theme_);
face_cfg.color_bg    = lvgl_theme->background_color();
face_cfg.color_eye   = lvgl_theme->text_color();
face_cfg.color_mouth = lvgl_theme->text_color();

// Inisialisasi engine (gunakan emoji_box_ atau container_ sebagai parent)
face_engine_lcd_ = new FaceEngineLcd();
face_engine_lcd_->Init(emoji_box_, face_cfg);
```

#### Langkah 4 — Edit `CMakeLists.txt`

```cmake
set(SOURCES
    # ... file lain ...
    "display/face_engine_lcd.cc"
)
```

---

### 6.2 Integrasi OLED 128×64 (`128x64`)

#### Langkah 1 — Copy semua file

```bash
cp 128x64/face_engine.h    main/display/
cp 128x64/face_engine.cc   main/display/
cp 128x64/oled_display.h   main/display/   # MENGGANTIKAN file original
cp 128x64/oled_display.cc  main/display/   # MENGGANTIKAN file original
```

> ⚠️ **Peringatan:** File `oled_display.h` dan `oled_display.cc` dari `128x64/` **menggantikan** file original. Pastikan backup file original sebelum meng-copy.

#### Langkah 2 — Edit `CMakeLists.txt`

```cmake
set(SOURCES
    # ... file lain ...
    "display/face_engine.cc"
)
```

Tidak ada perubahan lain yang diperlukan — file `oled_display.cc` sudah mencakup semua modifikasi.

---

## 7. Panduan Kalibrasi Visual

### Kalibrasi LCD Color

Semua nilai visual dikumpulkan dalam fungsi `MakeConfig_*()` di `face_engine_lcd.cc`. Tidak perlu menyentuh kode lain.

**Langkah:**

1. Buka `face_engine_lcd.cc`
2. Cari fungsi yang sesuai driver Anda (contoh: `MakeConfig_GC9A01_240x240()`)
3. Ubah nilai yang diinginkan
4. Compile dan flash

**Contoh kalibrasi untuk GC9A01 (layar bulat):**

```cpp
static FaceLcdConfig MakeConfig_GC9A01_240x240() {
    FaceLcdConfig c = MakeConfig_ST7789_240x240();  // mulai dari 240x240

    // Kecilkan sedikit agar tidak terpotong di sudut layar bulat
    c.eye_size       = 50;    // ← kurangi dari 55
    c.eye_gap        = 26;
    c.eye_offset_y   = -35;   // ← naikkan sedikit
    c.mouth_offset_y =  55;
    c.mouth_idle_w   = 70;    // ← kurangi dari 75
    c.mouth_speak_w  = 58;

    return c;
}
```

### Parameter yang Paling Sering Dikalibrasi

| Parameter | Efek | Tips |
|---|---|---|
| `eye_size` | Ukuran mata (px) | Mulai dari 20% lebar layar |
| `eye_gap` | Jarak antar mata | Biasanya 10–15% lebar layar |
| `eye_offset_y` | Posisi vertikal mata | Negatif = ke atas; coba `-h/6` sebagai titik awal |
| `mouth_offset_y` | Posisi vertikal mulut | Positif = ke bawah; coba `h/4` |
| `mouth_idle_w` | Lebar mulut Idle | Sekitar 30–35% lebar layar |
| `mouth_speak_w` | Lebar mulut Speaking | Sedikit lebih kecil dari `mouth_idle_w` |
| `speak_h_large` | Mulut terbuka maksimum | Sekitar 20–25% tinggi layar |
| `blink_chance` | Frekuensi kedipan | Lebih kecil = lebih sering (1/N per frame) |
| `update_interval_ms` | Kecepatan animasi | 40ms ≈ 25fps; 16ms ≈ 60fps |
| `color_eye` | Warna mata | `lv_color_white()` atau `lv_color_hex(0xRRGGBB)` |
| `color_bg` | Warna background | `lv_color_black()` untuk kontras terbaik |

### Kalibrasi OLED 128×64

Untuk OLED 128×64, nilai dikalibrasi langsung di `face_engine.cc` karena menggunakan konstanta hardcoded:

```cpp
// Di face_engine.cc:
#define EYE_OFFSET_X 5     // ← jarak mata dari center horizontal
#define EYE_OFFSET_Y 4     // ← jarak mata dari center vertikal

// Di dalam Init():
int eye_size_ = 30;        // ← ukuran dasar mata (30px untuk layar 128×64)
```

---

## 8. Cara Membuat Custom Display Baru

### 8.1 Custom untuk Driver LCD Baru

Misalkan Anda ingin menambahkan dukungan untuk driver **ILI9488** resolusi **320×480**.

**Langkah 1 — Buat fungsi konfigurasi baru di `face_engine_lcd.cc`:**

```cpp
/**
 * @brief Konfigurasi untuk ILI9488 320x480 (layar 3.5" besar)
 */
static FaceLcdConfig MakeConfig_ILI9488_320x480() {
    FaceLcdConfig c;

    c.canvas_w = 320;
    c.canvas_h = 480;

    // Mata proporsional untuk layar besar
    c.eye_size      = 80;
    c.eye_radius    = 14;
    c.eye_gap       = 42;

    c.eye_offset_y   = -75;
    c.mouth_offset_y =  115;

    c.mouth_idle_w = 110;
    c.mouth_idle_h =  18;
    c.mouth_idle_r =   9;

    c.mouth_listen_w = 55;
    c.mouth_listen_h = 10;
    c.mouth_listen_r =  5;

    c.mouth_speak_w = 92;
    c.mouth_speak_r = 22;

    c.speak_h_small = 10;
    c.speak_h_mid1  = 30;
    c.speak_h_mid2  = 55;
    c.speak_h_large = 80;

    c.speak_step          = 8;
    c.speak_interval_base = 75;
    c.speak_interval_rand = 85;

    c.idle_drift_x    = 14;
    c.idle_drift_y    =  8;
    c.idle_drift_chance = 40;

    c.blink_chance  = 100;
    c.eye_blink_h1  =  14;
    c.eye_blink_h2  =   1;
    c.eye_blink_h3  =  22;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}
```

**Langkah 2 — Daftarkan di `FaceEngineLcd::MakeDefaultConfig()`:**

```cpp
FaceLcdConfig FaceEngineLcd::MakeDefaultConfig(int display_w, int display_h) {
#if defined(CONFIG_LCD_GC9A01_240X240)
    return MakeConfig_GC9A01_240x240();
    
// ... entry lain ...

// Tambahkan ini:
#elif defined(CONFIG_LCD_ILI9488_320X480)
    return MakeConfig_ILI9488_320x480();

#else
    // fallback otomatis proporsional
    // ...
```

**Langkah 3 — Tambahkan makro di `config.h` board Anda:**

```kconfig
config LCD_ILI9488_320X480
    bool "ILI9488 320x480"
    default n
```

**Langkah 4 — Compile dan kalibrasi:**

Flash ke board, amati hasilnya, lalu sesuaikan parameter di `MakeConfig_ILI9488_320x480()` sampai proporsinya pas.

---

### 8.2 Custom untuk Tipe Display Baru

Misalkan Anda ingin membuat custom untuk display **e-Ink 200×200** yang bukan kategori LCD maupun OLED.

**Langkah 1 — Buat header baru `eink_display.h`:**

```cpp
#pragma once
#include "lvgl_display.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

// State animasi (sesuaikan dengan string dari lang_config.h)
enum class FaceStateEink { Idle, Listening, Speaking };

class EinkDisplay : public LvglDisplay {
public:
    EinkDisplay(esp_lcd_panel_io_handle_t panel_io,
                esp_lcd_panel_handle_t panel,
                int width, int height);
    ~EinkDisplay();

    virtual void SetupUI() override;
    virtual void SetStatus(const char* status) override;

private:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;

    lv_obj_t* container_  = nullptr;
    lv_obj_t* left_eye_   = nullptr;
    lv_obj_t* right_eye_  = nullptr;
    lv_obj_t* mouth_      = nullptr;

    FaceStateEink state_ = FaceStateEink::Idle;

    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

    void InitFace();
    void UpdateFace();
};
```

**Langkah 2 — Buat implementasi `eink_display.cc`:**

```cpp
#include "eink_display.h"
#include "assets/lang_config.h"
#include <esp_log.h>
#include <esp_lvgl_port.h>

#define TAG "EinkDisplay"

EinkDisplay::EinkDisplay(esp_lcd_panel_io_handle_t panel_io,
                         esp_lcd_panel_handle_t panel,
                         int width, int height)
    : panel_io_(panel_io), panel_(panel)
{
    width_  = width;
    height_ = height;
    // Inisialisasi LVGL port di sini (lihat OledDisplay::OledDisplay() sebagai referensi)
}

void EinkDisplay::SetStatus(const char* status) {
    Display::SetStatus(status);   // logging

    if (strcmp(status, Lang::Strings::STANDBY)   == 0) state_ = FaceStateEink::Idle;
    else if (strcmp(status, Lang::Strings::LISTENING) == 0) state_ = FaceStateEink::Listening;
    else if (strcmp(status, Lang::Strings::SPEAKING)  == 0) state_ = FaceStateEink::Speaking;
    else state_ = FaceStateEink::Idle;
}

void EinkDisplay::SetupUI() {
    if (setup_ui_called_) return;
    Display::SetupUI();
    InitFace();
}

void EinkDisplay::InitFace() {
    DisplayLockGuard lock(this);

    auto screen = lv_screen_active();

    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, width_, height_);
    lv_obj_center(container_);
    lv_obj_set_style_bg_color(container_, lv_color_white(), 0);  // e-ink: putih
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);

    // Mata — warna HITAM untuk e-ink
    left_eye_  = lv_obj_create(container_);
    right_eye_ = lv_obj_create(container_);
    lv_obj_set_style_bg_color(left_eye_,  lv_color_black(), 0);
    lv_obj_set_style_bg_color(right_eye_, lv_color_black(), 0);
    lv_obj_set_style_border_width(left_eye_,  0, 0);
    lv_obj_set_style_border_width(right_eye_, 0, 0);
    lv_obj_set_size(left_eye_,  40, 40);
    lv_obj_set_size(right_eye_, 40, 40);
    lv_obj_align(left_eye_,  LV_ALIGN_CENTER, -30, -20);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER,  30, -20);

    // Mulut
    mouth_ = lv_obj_create(container_);
    lv_obj_set_style_bg_color(mouth_, lv_color_black(), 0);
    lv_obj_set_style_border_width(mouth_, 0, 0);
    lv_obj_set_size(mouth_, 60, 10);
    lv_obj_align(mouth_, LV_ALIGN_CENTER, 0, 40);

    // Timer animasi — e-ink lambat, gunakan interval lebih panjang
    lv_timer_create(
        [](lv_timer_t* t) {
            static_cast<EinkDisplay*>(lv_timer_get_user_data(t))->UpdateFace();
        },
        200,   // 200ms untuk e-ink (lebih lambat dari LCD)
        this
    );
}

void EinkDisplay::UpdateFace() {
    // Implementasi perilaku per state sesuai kebutuhan display e-ink Anda
    switch (state_) {
        case FaceStateEink::Idle:
            lv_obj_set_size(mouth_, 60, 10);
            break;
        case FaceStateEink::Listening:
            lv_obj_set_size(mouth_, 30, 6);
            break;
        case FaceStateEink::Speaking:
            // e-ink terlalu lambat untuk animasi halus; gunakan tampilan statis
            lv_obj_set_size(mouth_, 55, 20 + rand() % 15);
            break;
    }
}
```

**Langkah 3 — Daftarkan di board file (`board.cc`):**

```cpp
#include "eink_display.h"

// Di dalam fungsi GetDisplay() atau CreateDisplay():
display_ = new EinkDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT);
```

**Langkah 4 — Tambahkan ke `CMakeLists.txt`:**

```cmake
set(SOURCES
    # ...
    "display/eink_display.cc"
)
```

---

## 9. Referensi Parameter `FaceLcdConfig`

Berlaku untuk `LCD_Display/face_engine_lcd.h`.

### Dimensi Kanvas

| Parameter | Tipe | Keterangan |
|---|---|---|
| `canvas_w` | `int` | Lebar area wajah dalam piksel (biasanya = `DISPLAY_WIDTH`) |
| `canvas_h` | `int` | Tinggi area wajah dalam piksel (biasanya = `DISPLAY_HEIGHT`) |

### Mata

| Parameter | Tipe | Default (240×320) | Keterangan |
|---|---|---|---|
| `eye_size` | `int` | 60 | Ukuran mata (tinggi & lebar awal) dalam px |
| `eye_radius` | `int` | 10 | Radius sudut mata; 0=kotak, besar=bulat/oval |
| `eye_gap` | `int` | 30 | Jarak pusat layar ke tepi mata |
| `eye_offset_y` | `int` | -50 | Offset Y mata dari tengah (negatif = ke atas) |

### Mulut

| Parameter | Tipe | Default (240×320) | Keterangan |
|---|---|---|---|
| `mouth_offset_y` | `int` | 80 | Offset Y mulut dari tengah (positif = ke bawah) |
| `mouth_idle_w / h / r` | `int` | 80/14/7 | Lebar, tinggi, radius mulut saat Idle |
| `mouth_listen_w / h / r` | `int` | 40/8/4 | Lebar, tinggi, radius mulut saat Listening |
| `mouth_speak_w / r` | `int` | 65/16 | Lebar dan radius mulut saat Speaking |

### Animasi Speaking

| Parameter | Tipe | Default | Keterangan |
|---|---|---|---|
| `speak_h_small / mid1 / mid2 / large` | `int` | 8/24/42/60 | Target tinggi mulut saat speaking (dipilih acak) |
| `speak_step` | `int` | 6 | Langkah smooth per frame (px/frame) |
| `speak_interval_base` | `int` | 80 | Interval minimum antar pergantian target (ms) |
| `speak_interval_rand` | `int` | 80 | Rentang acak tambahan interval (ms) |

### Idle Drift

| Parameter | Tipe | Default | Keterangan |
|---|---|---|---|
| `idle_drift_x / y` | `int` | 10/6 | Maksimum pergeseran mata saat Idle (px) |
| `idle_drift_chance` | `int` | 35 | 1-dari-N peluang pilih posisi drift baru per frame |

### Kedipan (Blink)

| Parameter | Tipe | Default | Keterangan |
|---|---|---|---|
| `blink_chance` | `int` | 100 | 1-dari-N peluang mulai kedip per frame (lebih kecil = lebih sering) |
| `eye_blink_h1` | `int` | 10 | Tinggi mata fase 1 (setengah tutup) |
| `eye_blink_h2` | `int` | 1 | Tinggi mata fase 2 (tertutup) |
| `eye_blink_h3` | `int` | 18 | Tinggi mata fase 3 (membuka kembali) |

### Warna & Timer

| Parameter | Tipe | Default | Keterangan |
|---|---|---|---|
| `color_eye` | `lv_color_t` | `lv_color_white()` | Warna mata |
| `color_mouth` | `lv_color_t` | `lv_color_white()` | Warna mulut |
| `color_bg` | `lv_color_t` | `lv_color_black()` | Warna background kanvas |
| `update_interval_ms` | `uint32_t` | 40 | Interval timer animasi; 40ms ≈ 25fps |

---

## 10. Referensi Driver LCD yang Didukung

Tabel berikut mencantumkan semua makro `config.h` yang dikenali oleh `FaceEngineLcd::MakeDefaultConfig()`.

| Makro `config.h` | Driver | Resolusi | Keterangan |
|---|---|---|---|
| `CONFIG_LCD_GC9A01_240X240` | GC9A01 | 240×240 | Layar **bulat** 1.28" — parameter dikalibrasi khusus agar tidak terpotong |
| `CONFIG_LCD_ST7789_240X320` | ST7789 | 240×320 | Portrait standard, paling umum |
| `CONFIG_LCD_ST7789_240X320_NO_IPS` | ST7789 | 240×320 | Non-IPS variant |
| `CONFIG_LCD_ST7789_240X240` | ST7789 | 240×240 | Layar persegi 1.3"/1.54" |
| `CONFIG_LCD_ST7789_240X240_7PIN` | ST7789 | 240×240 | 7-pin SPI Mode 3 |
| `CONFIG_LCD_ST7789_240X135` | ST7789 | 240×135 | Landscape pendek (TTGO T-Display) |
| `CONFIG_LCD_ST7789_170X320` | ST7789 | 170×320 | Portrait sempit |
| `CONFIG_LCD_ST7789_172X320` | ST7789 | 172×320 | Portrait sempit (varian 172) |
| `CONFIG_LCD_ST7789_240X280` | ST7789 | 240×280 | Portrait non-standard |
| `CONFIG_LCD_ST7735_128X160` | ST7735 | 128×160 | Layar kecil 1.8" |
| `CONFIG_LCD_ST7735_128X128` | ST7735 | 128×128 | Layar kotak 1.44" |
| `CONFIG_LCD_ST7796_320X480` | ST7796 | 320×480 | Layar besar 3.5" |
| `CONFIG_LCD_ST7796_320X480_NO_IPS` | ST7796 | 320×480 | Non-IPS variant |
| `CONFIG_LCD_ILI9341_240X320` | ILI9341 | 240×320 | Parameter identik dengan ST7789 240×320 |
| `CONFIG_LCD_ILI9341_240X320_NO_IPS` | ILI9341 | 240×320 | Non-IPS variant |
| `CONFIG_LCD_CUSTOM` / tidak ada | — | Otomatis | Konfigurasi proporsional berdasarkan `display_w` × `display_h` |

---

## 11. Troubleshooting

### Wajah tidak muncul sama sekali / layar gelap

- Pastikan `SetupUI()` sudah dipanggil sebelum `Init()` pada FaceEngine.
- Cek bahwa `parent` yang dilewatkan ke `Init()` bukan `nullptr`. Tambahkan log: `ESP_LOGI(TAG, "parent: %p", parent);`
- Pastikan `face_engine_lcd_.cc` sudah terdaftar di `CMakeLists.txt`.

### Wajah terlalu besar atau terpotong (terutama GC9A01 layar bulat)

- Kecilkan `eye_size`, `eye_gap`, dan `mouth_*_w` di fungsi `MakeConfig_*()` yang sesuai.
- Sesuaikan `eye_offset_y` dan `mouth_offset_y` mendekati 0 agar elemen lebih ke tengah.
- Untuk layar bulat, tambahkan padding sekitar 10–15% dari radius.

### Animasi terlalu cepat atau terlalu lambat

- Ubah `update_interval_ms`: lebih besar = lebih lambat, lebih kecil = lebih cepat.
- Untuk kecepatan mulut speaking saja: ubah `speak_step` (langkah per frame) dan `speak_interval_base` (jeda antar perubahan target).

### Mata berkedip terlalu sering atau terlalu jarang

- Ubah `blink_chance`: nilai **lebih kecil** = kedip **lebih sering** (probabilitas 1/N per frame).
- Contoh: `blink_chance = 50` → kedip dua kali lebih sering dari `blink_chance = 100`.

### State animasi tidak berubah saat berbicara atau mendengarkan

- Pastikan `SetStatus()` sudah di-override di subclass yang benar dan terhubung ke `application.cc`.
- Tambahkan log debug: `ESP_LOGI(TAG, "SetStatus called: %s", status);`
- Periksa string yang diterima `SetStatus()` — bandingkan dengan nilai `Lang::Strings::STANDBY`, `LISTENING`, dan `SPEAKING` di `lang_config.h`.

### Error compile: `Lang::Strings::STANDBY` tidak ditemukan

- Tambahkan `#include "assets/lang_config.h"` di `lcd_display.cc` atau `oled_display.cc`.
- Pastikan path relatifnya benar sesuai struktur direktori proyek Anda.

### Error compile: `FaceEngineLcd` tidak dikenali di `lcd_display.cc`

- Pastikan `#include "face_engine_lcd.h"` ada di `lcd_display.cc`.
- Pastikan file `face_engine_lcd.h` berada di direktori yang sama dengan `lcd_display.cc` (yaitu `main/display/`).

### Wajah tidak mengikuti tema (warna salah)

- Setelah `FaceEngineLcd::MakeDefaultConfig()`, override warna secara manual:
  ```cpp
  face_cfg.color_bg    = lvgl_theme->background_color();
  face_cfg.color_eye   = lvgl_theme->text_color();
  face_cfg.color_mouth = lvgl_theme->text_color();
  ```
- Pastikan `current_theme_` sudah diinisialisasi sebelum `SetupUI()` dipanggil.

---

## Catatan Akhir

- `FaceEngineLcd` (LCD) dan `FaceEngine` (OLED) menggunakan **nama enum berbeda** (`FaceStateLcd` vs `FaceState`) — ini disengaja untuk menghindari konflik jika kedua engine ada dalam satu build.
- Instance engine di-`new` di heap — **jangan `delete`** selama program berjalan.
- Semua operasi LVGL dalam engine berjalan di dalam LVGL task context (via timer), sehingga **thread-safe** terhadap LVGL secara otomatis.
- Fungsi `FaceEngineLcd::MakeDefaultConfig()` bersifat `static` — dapat dipanggil tanpa membuat instance terlebih dahulu.
- Untuk mengubah warna saat runtime mengikuti pergantian tema, Anda perlu membuat ulang engine (`Init()` kembali) atau menambahkan method setter di `FaceEngineLcd` yang memanggil `lv_obj_set_style_bg_color()` langsung.

---

*Dibuat untuk proyek xiaozhi-esp32 — Display Customization Guide*
