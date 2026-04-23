# 📘 Panduan Lengkap: FaceEngineLcd untuk LCD Color (GC9A01 & Lainnya)

> Dokumen ini menjelaskan **apa yang diubah** dari script OLED asli, **cara kerja** animasi wajah, **perbedaan** antara versi OLED dan LCD, serta **cara membuat konfigurasi custom** untuk driver LCD baru.

---

## Daftar Isi

1. [Gambaran Umum](#gambaran-umum)
2. [Struktur File](#struktur-file)
3. [Apa yang Diubah dari Script Original](#apa-yang-diubah-dari-script-original)
4. [Cara Kerja Animasi](#cara-kerja-animasi)
5. [Perbedaan OLED vs LCD](#perbedaan-oled-vs-lcd)
6. [Cara Membuat Konfigurasi Custom Baru](#cara-membuat-konfigurasi-custom-baru)
7. [Referensi Semua Parameter](#referensi-semua-parameter)
8. [Cara Integrasi ke Proyek](#cara-integrasi-ke-proyek)
9. [Troubleshooting](#troubleshooting)

---

## Gambaran Umum

Script ini adalah **animasi wajah AI** untuk layar LCD berwarna berbasis LVGL. Wajah terdiri dari dua mata dan satu mulut yang beranimasi tergantung kondisi (state) AI:

| State | Ekspresi |
|---|---|
| **Idle** | Mata bergerak pelan, berkedip acak, mulut diam |
| **Listening** | Mata asimetris (kanan lebih besar), mulut kecil |
| **Speaking** | Mulut bergerak naik-turun, mata stabil |

Script LCD ini dikembangkan dari `face_engine` yang sebelumnya hanya bekerja di **OLED monokrom 128×64**. Versi baru ini mendukung **semua driver LCD berwarna** dengan resolusi berbeda-beda.

---

## Struktur File

```
GC9A01.zip
├── 128x64/                         ← Script ORIGINAL (OLED)
│   ├── face_engine.h               ← Header class OLED (tidak diubah)
│   ├── face_engine.cc              ← Implementasi animasi OLED (tidak diubah)
│   ├── oled_display.cc             ← Contoh integrasi di OLED display
│   └── oled_display.h
│
├── display/                        ← File display utama proyek
│   ├── lcd_display.cc              ← Display LCD (template integrasi)
│   ├── lcd_display.h
│   ├── oled_display.cc
│   ├── oled_display.h
│   └── lvgl_display/               ← Library LVGL pendukung
│       ├── gif/
│       ├── jpg/
│       └── ...
│
└── LCD_Display/                    ← Script BARU (LCD color)
    ├── face_engine_lcd.h           ← Header class LCD (baru dibuat)
    ├── face_engine_lcd.cc          ← Implementasi + konfigurasi per driver
    └── README.md                   ← Dokumentasi teknis
```

**File yang perlu Anda gunakan:** hanya dua file di folder `LCD_Display/`:
- `face_engine_lcd.h`
- `face_engine_lcd.cc`

---

## Apa yang Diubah dari Script Original

Script original (`128x64/face_engine`) dirancang untuk OLED dengan ukuran **tetap 128×64 piksel** dan warna **hitam-putih**. Script LCD baru membawa banyak perubahan fundamental:

### 1. Nilai Hardcoded → Konfigurasi Terpisah

**Script OLED (lama)** — semua nilai dikodekan langsung di dalam fungsi:
```cpp
// face_engine.cc (OLED) — nilai langsung di kode
#define EYE_OFFSET_X 5
#define EYE_OFFSET_Y 4

void FaceEngine::Init(lv_obj_t* parent) {
    lv_obj_set_size(container_, 128, 64);   // ← ukuran tetap!
    int eye_size_ = 30;                      // ← tetap!
    ...
}
```

**Script LCD (baru)** — semua nilai dipisah ke struct `FaceLcdConfig`:
```cpp
// face_engine_lcd.h — semua parameter ada di struct ini
struct FaceLcdConfig {
    int canvas_w = 240;
    int canvas_h = 240;
    int eye_size = 50;
    int eye_gap  = 20;
    // ... 30+ parameter lainnya
};
```

> **Dampak:** Anda bisa mengubah tampilan wajah **tanpa menyentuh logika animasi sama sekali**. Cukup ubah nilai di fungsi `MakeConfig_*()`.

---

### 2. Warna Tetap → Warna Bisa Dikonfigurasi

**OLED:** Hanya putih (tidak relevan karena OLED monokrom).

**LCD:** Warna mata, mulut, dan background bisa diatur bebas:
```cpp
c.color_eye   = lv_color_white();            // putih
c.color_mouth = lv_color_hex(0x00FF00);      // hijau
c.color_bg    = lv_color_black();            // hitam
```

---

### 3. Timer Interval Tetap → Bisa Diatur

**OLED:** Timer di-hardcode `60 ms` (~16 fps):
```cpp
lv_timer_create([](lv_timer_t* t) { ... }, 60, this);
//                                          ^^^ tetap 60ms
```

**LCD:** Timer diambil dari konfigurasi:
```cpp
lv_timer_create([](lv_timer_t* t) { ... }, cfg_.update_interval_ms, this);
//                                          ^^^ bisa diatur, default 40ms (~25fps)
```

---

### 4. Animasi Speaking Diperluas

**OLED:** Target tinggi mulut hanya 4 pilihan kecil (2, 6, 10, 16 px):
```cpp
// face_engine.cc OLED
if (r < 20)       speak_mouth_target_ = 2;
else if (r < 50)  speak_mouth_target_ = 6;
else if (r < 80)  speak_mouth_target_ = 10;
else              speak_mouth_target_ = 16;
```

**LCD:** Target tinggi mulut dikonfigurasi per driver (bisa jauh lebih besar):
```cpp
// face_engine_lcd.cc LCD (untuk 240x320)
if (r < 20)       speak_target_h_ = cfg_.speak_h_small;   // 8px
else if (r < 50)  speak_target_h_ = cfg_.speak_h_mid1;    // 24px
else if (r < 80)  speak_target_h_ = cfg_.speak_h_mid2;    // 42px
else              speak_target_h_ = cfg_.speak_h_large;   // 60px
```

---

### 5. Posisi Mata — Sistem Koordinat Diperbarui

**OLED:** Posisi mata menggunakan offset hardcoded dari constant `EYE_OFFSET_X/Y`:
```cpp
lv_obj_align(left_eye_,  LV_ALIGN_CENTER, -eye_size_ - EYE_OFFSET_X, -EYE_OFFSET_Y);
lv_obj_align(right_eye_, LV_ALIGN_CENTER,  eye_size_ + EYE_OFFSET_X, -EYE_OFFSET_Y);
```

**LCD:** Posisi mata dihitung dari `eye_gap` dan `eye_size` yang dikonfigurasi + helper function:
```cpp
void FaceEngineLcd::ApplyEyePosition(int dx, int dy) {
    lv_obj_align(left_eye_,  LV_ALIGN_CENTER,
                 -(cfg_.eye_gap + cfg_.eye_size / 2) + dx,
                  cfg_.eye_offset_y + dy);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER,
                  (cfg_.eye_gap + cfg_.eye_size / 2) + dx,
                  cfg_.eye_offset_y + dy);
}
```

---

### 6. Rasio Lebar Mata Diperbarui

**OLED:** Lebar mata = tinggi × 0.65
```cpp
int eye_w = eye_h * 0.65;
```

**LCD:** Lebar mata = tinggi × 0.70 (sedikit lebih lebar, terlihat lebih natural di layar berwarna):
```cpp
int eye_w = (int)(eye_h * 0.70f);
```

---

### 7. Enum Name Diubah untuk Menghindari Konflik

**OLED:** `FaceState` dan `FaceEngine`

**LCD:** `FaceStateLcd` dan `FaceEngineLcd`

Ini sengaja dibedakan agar **kedua engine bisa dipakai sekaligus** dalam satu proyek tanpa konflik nama.

---

### 8. Konfigurasi per Driver LCD

Ini fitur paling besar. Script LCD menyertakan **16 konfigurasi preset** untuk berbagai driver:

| Driver | Resolusi | Fungsi Konfigurasi |
|---|---|---|
| ST7789 | 240×320 | `MakeConfig_ST7789_240x320()` |
| ST7789 | 240×240 | `MakeConfig_ST7789_240x240()` |
| ST7789 | 240×135 | `MakeConfig_ST7789_240x135()` |
| ST7789 | 170×320 | `MakeConfig_ST7789_170x320()` |
| ST7735 | 128×160 | `MakeConfig_ST7735_128x160()` |
| ST7735 | 128×128 | `MakeConfig_ST7735_128x128()` |
| ST7796 | 320×480 | `MakeConfig_ST7796_320x480()` |
| ILI9341 | 240×320 | `MakeConfig_ILI9341_240x320()` |
| **GC9A01** | **240×240** | **`MakeConfig_GC9A01_240x240()`** |
| Custom | Proporsional | Dihitung otomatis |

Pemilihan konfigurasi terjadi **otomatis** berdasarkan makro di `config.h`:
```cpp
FaceLcdConfig FaceEngineLcd::MakeDefaultConfig(int w, int h) {
#if defined(CONFIG_LCD_GC9A01_240X240)
    return MakeConfig_GC9A01_240x240();
#elif defined(CONFIG_LCD_ST7789_240X320)
    return MakeConfig_ST7789_240x320();
// ... dst
#endif
}
```

---

## Cara Kerja Animasi

### Alur Utama

```
lcd_display.cc
     │
     ├─ SetStatus("listening")
     │        │
     │        └─ FaceEngineLcd::SetState(FaceStateLcd::Listening)
     │
     └─ LVGL Timer (setiap 40ms)
              │
              └─ FaceEngineLcd::Update()
                       │
                       ├─ [1] Proses blink (kedipan)
                       │       → Hitung eye_h aktual
                       │
                       └─ [2] Dispatch ke behavior
                               ├─ Idle     → IdleBehavior(eye_h)
                               ├─ Listening → ListeningBehavior(eye_h)
                               └─ Speaking  → SpeakingBehavior(eye_h)
```

Semua animasi berjalan **tanpa loop manual** — sepenuhnya digerakkan oleh LVGL timer, aman untuk FreeRTOS.

---

### Detail per State

#### 🔵 Idle

```
Update() dipanggil setiap 40ms
  │
  ├─ Peluang 1/blink_chance → mulai kedip
  │     Frame 1: eye_h = eye_blink_h1  (setengah menutup)
  │     Frame 2: eye_h = eye_blink_h2  (tertutup)
  │     Frame 3: eye_h = eye_blink_h3  (membuka kembali)
  │     Frame 4+: eye_h = eye_size    (normal kembali)
  │
  └─ IdleBehavior(eye_h):
        ├─ Peluang 1/idle_drift_chance → pilih posisi drift baru (x, y acak)
        ├─ Mata bergerak ke posisi drift tersebut
        └─ Mulut tipis di posisi center + sedikit ikut drift
```

#### 🟡 Listening

```
Update() dipanggil setiap 40ms
  │
  ├─ Proses blink tetap berjalan
  │
  └─ ListeningBehavior(eye_h):
        ├─ Mata kanan: ukuran normal
        ├─ Mata kiri: 12% lebih kecil (asimetri "perhatian")
        ├─ Posisi mata: tetap, tidak drift
        └─ Mulut: kecil, sedikit ke kiri
```

#### 🟢 Speaking

```
Update() dipanggil setiap 40ms
  │
  ├─ Proses blink tetap berjalan
  │
  └─ SpeakingBehavior(eye_h):
        ├─ Mata: stabil, tidak asimetris, tidak drift
        └─ Mulut:
              ├─ Setiap speak_interval ms → pilih speak_target_h_ baru (acak)
              │     20% → speak_h_small (hampir tutup)
              │     30% → speak_h_mid1  (setengah terbuka)
              │     30% → speak_h_mid2  (terbuka sedang)
              │     20% → speak_h_large (terbuka lebar)
              │
              └─ speak_current_h_ mendekati target +/- speak_step per frame
                   (gerakan smooth, tidak langsung melompat)
```

---

## Perbedaan OLED vs LCD

| Aspek | OLED 128×64 (Original) | LCD Color (Baru) |
|---|---|---|
| **Class name** | `FaceEngine` | `FaceEngineLcd` |
| **Enum name** | `FaceState` | `FaceStateLcd` |
| **Ukuran canvas** | Tetap 128×64 | Dikonfigurasi per driver |
| **Warna** | Hitam-putih saja | RGB penuh, bisa dikonfigurasi |
| **Konfigurasi** | Hardcoded di kode | Struct `FaceLcdConfig` terpisah |
| **Timer interval** | Tetap 60ms (~16fps) | Default 40ms (~25fps), bisa diubah |
| **Driver support** | OLED SSD1306 saja | ST7789, ST7735, ST7796, ILI9341, GC9A01 |
| **Preset konfigurasi** | Tidak ada | 16 preset + custom otomatis |
| **Radius mata** | Tetap (radius 2) | Bisa dikonfigurasi (`eye_radius`) |
| **Range drift mata** | ±2px X, ±1px Y | Bisa dikonfigurasi (`idle_drift_x/y`) |
| **Target mouth saat speaking** | Max 16px | Bisa ratusan px (sesuai layar) |
| **Lebar mata** | 65% dari tinggi | 70% dari tinggi |
| **Kedua engine bisa jalan bersamaan?** | — | ✅ Ya (nama berbeda) |

---

## Cara Membuat Konfigurasi Custom Baru

Ada dua skenario: **kalibrasi driver yang sudah ada**, atau **menambahkan driver baru sepenuhnya**.

### Skenario A: Kalibrasi Ulang Driver yang Sudah Ada

Misalnya Anda pakai GC9A01 tapi wajahnya terlalu besar. Buka `face_engine_lcd.cc` dan cari fungsi `MakeConfig_GC9A01_240x240()`:

```cpp
static FaceLcdConfig MakeConfig_GC9A01_240x240() {
    FaceLcdConfig c;

    c.canvas_w = 240;
    c.canvas_h = 240;

    // Ubah nilai di sini sesuai kebutuhan:
    c.eye_size       = 45;   // ← perkecil dari default 50
    c.eye_radius     = 22;   // ← lebih bulat (nilai tinggi = lebih bulat)
    c.eye_gap        = 18;   // ← dekatkan kedua mata

    c.eye_offset_y   = -28;  // ← naikkan mata (lebih negatif = lebih atas)
    c.mouth_offset_y =  45;  // ← turunkan mulut (lebih positif = lebih bawah)

    // ... nilai lainnya biarkan default
    return c;
}
```

Setelah mengubah, **compile dan flash** — tidak ada perubahan lain yang diperlukan.

---

### Skenario B: Menambahkan Driver LCD Baru

Misalnya Anda punya driver `SH8601` dengan resolusi 454×454.

**Langkah 1:** Buat fungsi konfigurasi baru di `face_engine_lcd.cc`:

```cpp
static FaceLcdConfig MakeConfig_SH8601_454x454() {
    FaceLcdConfig c;

    c.canvas_w = 454;
    c.canvas_h = 454;

    // Skala dari 240x240 (referensi): faktor ~1.9x
    c.eye_size       = 95;   // 50 * 1.9
    c.eye_radius     = 15;
    c.eye_gap        = 38;   // 20 * 1.9

    c.eye_offset_y   = -57;  // -30 * 1.9
    c.mouth_offset_y =  95;  //  50 * 1.9

    c.mouth_idle_w = 114;    // 60 * 1.9
    c.mouth_idle_h =  23;
    c.mouth_idle_r =  11;

    c.mouth_listen_w = 57;
    c.mouth_listen_h = 11;
    c.mouth_listen_r =  6;

    c.mouth_speak_w = 95;
    c.mouth_speak_r = 27;

    c.speak_h_small  = 15;
    c.speak_h_mid1   = 38;
    c.speak_h_mid2   = 68;
    c.speak_h_large  = 105;

    c.speak_step          = 8;
    c.speak_interval_base = 80;
    c.speak_interval_rand = 80;

    c.idle_drift_x    = 12;
    c.idle_drift_y    = 8;
    c.idle_drift_chance = 30;

    c.blink_chance  = 100;
    c.eye_blink_h1  = 15;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 26;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}
```

**Langkah 2:** Daftarkan di fungsi `MakeDefaultConfig()`:

```cpp
FaceLcdConfig FaceEngineLcd::MakeDefaultConfig(int w, int h) {
// ...
#elif defined(CONFIG_LCD_SH8601_454X454)        // ← tambahkan ini
    return MakeConfig_SH8601_454x454();
// ...
}
```

**Langkah 3:** Tambahkan makro di `config.h` board Anda:

```cmake
# Contoh di sdkconfig atau config.h
CONFIG_LCD_SH8601_454X454=y
```

**Langkah 4:** Integrasikan seperti biasa (lihat bagian [Cara Integrasi](#cara-integrasi-ke-proyek)).

---

### Tips Menghitung Nilai Skala

Jika Anda membuat konfigurasi untuk resolusi baru, gunakan rumus skala dari resolusi referensi 240×240:

```
faktor_skala = min(resolusi_baru_w, resolusi_baru_h) / 240.0

eye_size_baru   = round(50 * faktor_skala)
eye_gap_baru    = round(20 * faktor_skala)
eye_offset_baru = round(-30 * faktor_skala)
mouth_offset_y  = round(50 * faktor_skala)
```

Untuk **layar bulat** (GC9A01), kecilkan `eye_size` ~10% dan buat `eye_offset_y` dan `mouth_offset_y` lebih mendekati 0 agar elemen tidak terpotong di sudut lingkaran.

---

## Referensi Semua Parameter

### Ukuran & Posisi

| Parameter | Tipe | Keterangan | Panduan |
|---|---|---|---|
| `canvas_w` | `int` | Lebar area wajah (px) | Biasanya = lebar layar |
| `canvas_h` | `int` | Tinggi area wajah (px) | Biasanya = tinggi layar |
| `eye_size` | `int` | Ukuran default mata | ~20% dari dimensi terkecil layar |
| `eye_radius` | `int` | Kelingkaran sudut mata | 0=kotak, sama dengan `eye_size`=lingkaran |
| `eye_gap` | `int` | Jarak antar mata | ~8–10% lebar layar |
| `eye_offset_y` | `int` | Posisi Y mata dari center | Negatif = ke atas |
| `mouth_offset_y` | `int` | Posisi Y mulut dari center | Positif = ke bawah |

### Mulut

| Parameter | Keterangan |
|---|---|
| `mouth_idle_w/h/r` | Lebar, tinggi, radius mulut saat Idle |
| `mouth_listen_w/h/r` | Lebar, tinggi, radius mulut saat Listening |
| `mouth_speak_w/r` | Lebar dan radius mulut saat Speaking |
| `speak_h_small/mid1/mid2/large` | Target tinggi mulut saat Speaking (dipilih acak) |

### Kecepatan Animasi

| Parameter | Keterangan |
|---|---|
| `speak_step` | Perubahan tinggi mulut per frame (px). Besar = lebih cepat |
| `speak_interval_base` | Interval minimum ganti target mulut (ms) |
| `speak_interval_rand` | Rentang acak tambahan interval (ms) |
| `update_interval_ms` | Interval timer LVGL (ms). 40ms = ~25fps |

### Gerakan Idle

| Parameter | Keterangan |
|---|---|
| `idle_drift_x` | Maksimum pergeseran horizontal mata (px) |
| `idle_drift_y` | Maksimum pergeseran vertikal mata (px) |
| `idle_drift_chance` | 1-dari-N peluang pilih posisi drift baru per frame |

### Kedipan

| Parameter | Keterangan |
|---|---|
| `blink_chance` | 1-dari-N peluang mulai kedip per frame. Kecil = sering |
| `eye_blink_h1` | Tinggi mata frame 1 (setengah menutup) |
| `eye_blink_h2` | Tinggi mata frame 2 (tertutup, biasanya 1) |
| `eye_blink_h3` | Tinggi mata frame 3 (sedikit lebih besar saat buka) |

### Warna

| Parameter | Contoh |
|---|---|
| `color_eye` | `lv_color_white()` atau `lv_color_hex(0xFFFFFF)` |
| `color_mouth` | `lv_color_hex(0x00FF00)` (hijau) |
| `color_bg` | `lv_color_black()` atau `lv_color_hex(0x000000)` |

---

## Cara Integrasi ke Proyek

### 1. Copy file ke proyek

```
main/
└── display/
    ├── face_engine_lcd.h    ← copy dari LCD_Display/
    ├── face_engine_lcd.cc   ← copy dari LCD_Display/
    ├── lcd_display.h        ← sudah ada (perlu sedikit tambahan)
    └── lcd_display.cc       ← sudah ada (perlu sedikit tambahan)
```

### 2. Tambahkan di `lcd_display.h`

Di dalam `class LcdDisplay`, tambahkan:

```cpp
#include "face_engine_lcd.h"

class LcdDisplay : public LvglDisplay {
private:
    // ... member yang sudah ada ...
    FaceEngineLcd* face_engine_lcd_ = nullptr;  // ← tambahkan ini

public:
    virtual void SetStatus(const char* status) override;  // ← tambahkan ini
};
```

### 3. Tambahkan di `lcd_display.cc`

Di akhir fungsi `SetupUI()`:

```cpp
void LcdDisplay::SetupUI() {
    // ... kode yang sudah ada ...

    // ── Inisialisasi FaceEngineLcd ──
    FaceLcdConfig face_cfg = FaceEngineLcd::MakeDefaultConfig(width_, height_);
    face_engine_lcd_ = new FaceEngineLcd();
    face_engine_lcd_->Init(emoji_box_, face_cfg);
}
```

Tambahkan fungsi `SetStatus()`:

```cpp
void LcdDisplay::SetStatus(const char* status) {
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

### 4. Daftarkan ke `CMakeLists.txt`

```cmake
set(SOURCES
    # ... sumber lain ...
    "display/face_engine_lcd.cc"
)
```

---

## Troubleshooting

| Masalah | Penyebab | Solusi |
|---|---|---|
| Wajah tidak muncul / layar gelap | `Init()` dipanggil sebelum LVGL siap | Pastikan `SetupUI()` dipanggil lebih dulu |
| Wajah terpotong di tepi | Nilai terlalu besar untuk resolusi layar | Kecilkan `eye_size`, `eye_gap`, `mouth_*_w` |
| Khusus GC9A01: terpotong di sudut | Layar bulat memotong sudut | Kecilkan semua ukuran ~10–15%, geser offset mendekati 0 |
| Animasi terlalu cepat | `speak_step` terlalu besar atau `update_interval_ms` terlalu kecil | Perbesar `update_interval_ms` atau kecilkan `speak_step` |
| Animasi terlalu lambat | Sebaliknya | Kecilkan `update_interval_ms` atau perbesar `speak_step` |
| Kedipan terlalu sering | `blink_chance` terlalu kecil | Perbesar nilai `blink_chance` |
| Wajah tidak berubah state | `SetStatus()` tidak terhubung | Tambahkan log `ESP_LOGI(TAG, "status: %s", status)` untuk debug |
| Error compile: `FaceStateLcd` conflict | Nama enum bertabrakan | Pastikan tidak ada `using namespace` yang ambiguous |
| Error: `Lang::Strings::STANDBY` not found | Include missing | Tambahkan `#include "assets/lang_config.h"` |

---

## Catatan Penting

- `FaceEngineLcd` dan `FaceEngine` (OLED) **bisa berjalan bersamaan** di proyek berbeda tanpa konflik, karena nama class dan enum berbeda.
- Instance `FaceEngineLcd` di-`new` di heap — **jangan di-`delete`** selama program berjalan.
- Semua operasi LVGL berjalan dalam LVGL timer context → **thread-safe terhadap LVGL**.
- Fungsi `Update()` bersifat `public` bukan untuk dipanggil manual, tapi agar LVGL timer lambda bisa mengaksesnya.
- Untuk mengubah warna saat runtime (ikut tema), override `color_eye`/`color_mouth`/`color_bg` di config sebelum memanggil `Init()`.
