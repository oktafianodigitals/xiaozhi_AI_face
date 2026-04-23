/**
 * @file face_engine_lcd.cc
 * @brief Implementasi FaceEngineLcd — animasi wajah untuk LCD color LVGL
 *
 * ═══════════════════════════════════════════════════════════════════════════
 *  PANDUAN KALIBRASI
 * ═══════════════════════════════════════════════════════════════════════════
 *
 *  Semua nilai visual (ukuran mata, posisi, warna, kecepatan) dikumpulkan
 *  di fungsi-fungsi MakeConfig_*() di bawah. Anda TIDAK perlu mengubah
 *  logika animasi — cukup sesuaikan fungsi konfigurasi yang sesuai dengan
 *  driver/resolusi LCD Anda.
 *
 *  LANGKAH KALIBRASI:
 *    1. Tentukan driver LCD Anda dari config.h (ST7789, ILI9341, GC9A01, dst.)
 *    2. Temukan fungsi MakeConfig_*() yang sesuai di bagian bawah file ini
 *    3. Ubah nilai-nilai di dalam fungsi tersebut
 *    4. Compile & flash — tidak ada kode lain yang perlu diubah
 *
 *  MENAMBAH DRIVER BARU:
 *    1. Buat fungsi MakeConfig_DriverBaru() baru
 *    2. Tambahkan #elif di FaceEngineLcd::MakeDefaultConfig()
 *    3. Sesuaikan nilainya
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "face_engine_lcd.h"
#include <stdlib.h>
#include <esp_log.h>

#define TAG "FaceEngineLcd"

// ═══════════════════════════════════════════════════════════════════════════
//  FUNGSI KONFIGURASI PER DRIVER/RESOLUSI
//  ► Edit bagian ini untuk kalibrasi tampilan
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Konfigurasi untuk ST7789 240x320 (portrait, resolusi paling umum).
 *
 * Wajah ditampilkan di area tengah dengan ukuran proporsional.
 * Cocok juga untuk: ST7789 240x280, ST7796 320x480 (perlu penyesuaian skala).
 */
static FaceLcdConfig MakeConfig_ST7789_240x320() {
    FaceLcdConfig c;

    // Dimensi kanvas wajah — sebaiknya biarkan otomatis dari DISPLAY_WIDTH/HEIGHT
    c.canvas_w = 240;
    c.canvas_h = 320;

    // ── Mata ────────────────────────────────────────────────────────────────
    c.eye_size      = 60;   // ukuran mata (px) — perbesar untuk layar besar
    c.eye_radius    = 10;   // sudut bulat mata — 0=kotak, besar=bulat
    c.eye_gap       = 30;   // jarak pusat ke pinggir mata

    // ── Posisi ──────────────────────────────────────────────────────────────
    c.eye_offset_y   = -50;  // mata lebih ke atas dari center (negatif = atas)
    c.mouth_offset_y =  80;  // mulut lebih ke bawah dari center

    // ── Mulut Idle ──────────────────────────────────────────────────────────
    c.mouth_idle_w = 80;
    c.mouth_idle_h = 14;
    c.mouth_idle_r = 7;

    // ── Mulut Listening ─────────────────────────────────────────────────────
    c.mouth_listen_w = 40;
    c.mouth_listen_h =  8;
    c.mouth_listen_r =  4;

    // ── Mulut Speaking ──────────────────────────────────────────────────────
    c.mouth_speak_w = 65;
    c.mouth_speak_r = 16;

    // ── Target tinggi mulut saat speaking ───────────────────────────────────
    c.speak_h_small =  8;
    c.speak_h_mid1  = 24;
    c.speak_h_mid2  = 42;
    c.speak_h_large = 60;

    // ── Kecepatan animasi ────────────────────────────────────────────────────
    c.speak_step          = 6;   // makin besar = mulut bergerak lebih cepat
    c.speak_interval_base = 80;
    c.speak_interval_rand = 80;

    // ── Idle drift ──────────────────────────────────────────────────────────
    c.idle_drift_x    = 10;
    c.idle_drift_y    =  6;
    c.idle_drift_chance = 35;

    // ── Blink ────────────────────────────────────────────────────────────────
    c.blink_chance  = 100;
    c.eye_blink_h1  = 10;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 18;

    // ── Warna ────────────────────────────────────────────────────────────────
    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    // ── Timer ────────────────────────────────────────────────────────────────
    c.update_interval_ms = 40;  // ~25 fps

    return c;
}

/**
 * @brief Konfigurasi untuk ST7789 240x240 (layar kotak/square).
 *
 * Cocok untuk modul 1.3" dan 1.54" persegi.
 * Termasuk varian 7-pin (SPI Mode 3).
 */
static FaceLcdConfig MakeConfig_ST7789_240x240() {
    FaceLcdConfig c;

    c.canvas_w = 240;
    c.canvas_h = 240;

    c.eye_size      = 55;
    c.eye_radius    = 10;
    c.eye_gap       = 28;

    c.eye_offset_y   = -38;
    c.mouth_offset_y =  60;

    c.mouth_idle_w = 75;
    c.mouth_idle_h = 13;
    c.mouth_idle_r = 6;

    c.mouth_listen_w = 36;
    c.mouth_listen_h =  7;
    c.mouth_listen_r =  3;

    c.mouth_speak_w = 60;
    c.mouth_speak_r = 14;

    c.speak_h_small =  7;
    c.speak_h_mid1  = 22;
    c.speak_h_mid2  = 38;
    c.speak_h_large = 55;

    c.speak_step          = 5;
    c.speak_interval_base = 85;
    c.speak_interval_rand = 75;

    c.idle_drift_x    = 9;
    c.idle_drift_y    = 5;
    c.idle_drift_chance = 32;

    c.blink_chance  = 110;
    c.eye_blink_h1  =  9;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 16;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}

/**
 * @brief Konfigurasi untuk ST7789 240x135 (layar landscape lebar-pendek, misal TTGO T-Display).
 *
 * Karena canvas pendek (135px), ukuran mata dikurangi dan posisi dikompres.
 */
static FaceLcdConfig MakeConfig_ST7789_240x135() {
    FaceLcdConfig c;

    c.canvas_w = 240;
    c.canvas_h = 135;

    c.eye_size      = 38;
    c.eye_radius    = 7;
    c.eye_gap       = 24;

    c.eye_offset_y   = -20;
    c.mouth_offset_y =  38;

    c.mouth_idle_w = 55;
    c.mouth_idle_h = 10;
    c.mouth_idle_r = 5;

    c.mouth_listen_w = 26;
    c.mouth_listen_h =  5;
    c.mouth_listen_r =  3;

    c.mouth_speak_w = 45;
    c.mouth_speak_r = 12;

    c.speak_h_small =  5;
    c.speak_h_mid1  = 15;
    c.speak_h_mid2  = 26;
    c.speak_h_large = 36;

    c.speak_step          = 4;
    c.speak_interval_base = 75;
    c.speak_interval_rand = 70;

    c.idle_drift_x    = 7;
    c.idle_drift_y    = 3;
    c.idle_drift_chance = 28;

    c.blink_chance  = 100;
    c.eye_blink_h1  =  6;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 11;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}

/**
 * @brief Konfigurasi untuk ST7735 128x160 (layar kecil 1.8").
 *
 * Ukuran mata jauh lebih kecil karena resolusi rendah.
 */
static FaceLcdConfig MakeConfig_ST7735_128x160() {
    FaceLcdConfig c;

    c.canvas_w = 128;
    c.canvas_h = 160;

    c.eye_size      = 32;
    c.eye_radius    = 5;
    c.eye_gap       = 16;

    c.eye_offset_y   = -25;
    c.mouth_offset_y =  42;

    c.mouth_idle_w = 42;
    c.mouth_idle_h =  8;
    c.mouth_idle_r =  4;

    c.mouth_listen_w = 20;
    c.mouth_listen_h =  4;
    c.mouth_listen_r =  2;

    c.mouth_speak_w = 34;
    c.mouth_speak_r = 10;

    c.speak_h_small =  4;
    c.speak_h_mid1  = 12;
    c.speak_h_mid2  = 22;
    c.speak_h_large = 30;

    c.speak_step          = 3;
    c.speak_interval_base = 80;
    c.speak_interval_rand = 70;

    c.idle_drift_x    = 5;
    c.idle_drift_y    = 3;
    c.idle_drift_chance = 30;

    c.blink_chance  = 100;
    c.eye_blink_h1  =  5;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 10;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}

/**
 * @brief Konfigurasi untuk ST7735 128x128 (layar kotak 1.44").
 */
static FaceLcdConfig MakeConfig_ST7735_128x128() {
    FaceLcdConfig c = MakeConfig_ST7735_128x160();  // mulai dari 128x160
    c.canvas_h       = 128;
    c.eye_offset_y   = -20;
    c.mouth_offset_y =  36;
    return c;
}

/**
 * @brief Konfigurasi untuk ST7789 170x320 dan 172x320 (layar memanjang sempit).
 */
static FaceLcdConfig MakeConfig_ST7789_170x320() {
    FaceLcdConfig c = MakeConfig_ST7789_240x320();
    c.canvas_w  = 170;
    c.eye_gap   = 22;
    c.eye_size  = 50;
    c.mouth_idle_w  = 65;
    c.mouth_speak_w = 55;
    return c;
}

/**
 * @brief Konfigurasi untuk ST7796 320x480 (layar besar 3.5").
 *
 * Ukuran mata diperbesar signifikan agar proporsional.
 */
static FaceLcdConfig MakeConfig_ST7796_320x480() {
    FaceLcdConfig c;

    c.canvas_w = 320;
    c.canvas_h = 480;

    c.eye_size      = 80;
    c.eye_radius    = 14;
    c.eye_gap       = 40;

    c.eye_offset_y   = -70;
    c.mouth_offset_y = 110;

    c.mouth_idle_w = 110;
    c.mouth_idle_h = 18;
    c.mouth_idle_r = 9;

    c.mouth_listen_w = 54;
    c.mouth_listen_h = 10;
    c.mouth_listen_r =  5;

    c.mouth_speak_w = 90;
    c.mouth_speak_r = 20;

    c.speak_h_small = 10;
    c.speak_h_mid1  = 30;
    c.speak_h_mid2  = 55;
    c.speak_h_large = 78;

    c.speak_step          = 8;
    c.speak_interval_base = 75;
    c.speak_interval_rand = 85;

    c.idle_drift_x    = 14;
    c.idle_drift_y    =  8;
    c.idle_drift_chance = 40;

    c.blink_chance  = 100;
    c.eye_blink_h1  = 13;
    c.eye_blink_h2  =  1;
    c.eye_blink_h3  = 22;

    c.color_eye   = lv_color_white();
    c.color_mouth = lv_color_white();
    c.color_bg    = lv_color_black();

    c.update_interval_ms = 40;

    return c;
}

/**
 * @brief Konfigurasi untuk ILI9341 240x320 (identik fisik dengan ST7789 240x320).
 *
 * Driver berbeda, parameter display sama — hanya inisialisasi panel yang beda di board.cc.
 */
static FaceLcdConfig MakeConfig_ILI9341_240x320() {
    // Parameter visual sama dengan ST7789 240x320
    return MakeConfig_ST7789_240x320();
}

/**
 * @brief Konfigurasi untuk GC9A01 240x240 (layar bundar 1.28").
 *
 * Layar bulat — posisi wajah dikalibrasi agar tidak terpotong di sudut.
 * Mata dan mulut digeser sedikit ke tengah.
 */
static FaceLcdConfig MakeConfig_GC9A01_240x240() {
    FaceLcdConfig c = MakeConfig_ST7789_240x240();

    // GC9A01 layar bundar — kecilkan sedikit agar elemen tidak terpotong
    c.eye_size      = 50;
    c.eye_gap       = 26;
    c.eye_offset_y  = -35;
    c.mouth_offset_y=  55;
    c.mouth_idle_w  = 70;
    c.mouth_speak_w = 58;

    return c;
}

/**
 * @brief Pilih konfigurasi secara otomatis berdasarkan makro CONFIG_LCD_xxx dari config.h.
 *
 * Dipanggil di lcd_display.cc sebelum FaceEngineLcd::Init() jika Anda ingin
 * konfigurasi otomatis. Anda juga bisa membuat FaceLcdConfig manual.
 *
 * @param display_w  Lebar display aktual (dari DISPLAY_WIDTH)
 * @param display_h  Tinggi display aktual (dari DISPLAY_HEIGHT)
 * @return FaceLcdConfig yang sesuai driver
 */
FaceLcdConfig FaceEngineLcd::MakeDefaultConfig(int display_w, int display_h) {
#if defined(CONFIG_LCD_GC9A01_240X240)
    return MakeConfig_GC9A01_240x240();

#elif defined(CONFIG_LCD_ST7789_240X320) || defined(CONFIG_LCD_ST7789_240X320_NO_IPS)
    return MakeConfig_ST7789_240x320();

#elif defined(CONFIG_LCD_ST7789_240X240) || defined(CONFIG_LCD_ST7789_240X240_7PIN)
    return MakeConfig_ST7789_240x240();

#elif defined(CONFIG_LCD_ST7789_240X135)
    return MakeConfig_ST7789_240x135();

#elif defined(CONFIG_LCD_ST7789_170X320)
    return MakeConfig_ST7789_170x320();

#elif defined(CONFIG_LCD_ST7789_172X320)
    return MakeConfig_ST7789_170x320();   // parameter visual hampir identik

#elif defined(CONFIG_LCD_ST7789_240X280)
    // 240x280: gunakan 240x320 lalu sesuaikan height
    FaceLcdConfig c = MakeConfig_ST7789_240x320();
    c.canvas_h = 280;
    return c;

#elif defined(CONFIG_LCD_ST7735_128X160)
    return MakeConfig_ST7735_128x160();

#elif defined(CONFIG_LCD_ST7735_128X128)
    return MakeConfig_ST7735_128x128();

#elif defined(CONFIG_LCD_ST7796_320X480) || defined(CONFIG_LCD_ST7796_320X480_NO_IPS)
    return MakeConfig_ST7796_320x480();

#elif defined(CONFIG_LCD_ILI9341_240X320) || defined(CONFIG_LCD_ILI9341_240X320_NO_IPS)
    return MakeConfig_ILI9341_240x320();

#else
    // CONFIG_LCD_CUSTOM atau driver tidak dikenal — buat konfigurasi proporsional otomatis
    ESP_LOGW(TAG, "Driver LCD tidak dikenal atau CONFIG_LCD_CUSTOM. "
                  "Menggunakan konfigurasi proporsional untuk %dx%d", display_w, display_h);

    FaceLcdConfig c;
    c.canvas_w = display_w;
    c.canvas_h = display_h;

    // Skala proporsional berdasarkan dimensi terkecil
    int ref = (display_w < display_h) ? display_w : display_h;
    c.eye_size       = ref / 4;
    c.eye_radius     = ref / 24;
    c.eye_gap        = ref / 8;
    c.eye_offset_y   = -(display_h / 6);
    c.mouth_offset_y =  (display_h / 4);
    c.mouth_idle_w   = ref / 3;
    c.mouth_idle_h   = ref / 18;
    c.mouth_idle_r   = ref / 36;
    c.mouth_listen_w = ref / 6;
    c.mouth_listen_h = ref / 36;
    c.mouth_listen_r = ref / 72;
    c.mouth_speak_w  = ref / 3 - 4;
    c.mouth_speak_r  = ref / 16;
    c.speak_h_small  = ref / 28;
    c.speak_h_mid1   = ref / 10;
    c.speak_h_mid2   = ref / 6;
    c.speak_h_large  = ref / 4;
    c.speak_step     = 4;
    c.idle_drift_x   = ref / 24;
    c.idle_drift_y   = ref / 36;

    return c;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════
//  IMPLEMENTASI KELAS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Inisialisasi engine: buat semua objek LVGL dan pasang timer update.
 *
 * Dipanggil sekali dari lcd_display.cc setelah LVGL siap.
 * Semua objek dibuat sebagai child dari `parent` agar ikut posisi parent.
 */
void FaceEngineLcd::Init(lv_obj_t* parent, const FaceLcdConfig& config) {
    cfg_ = config;

    ESP_LOGI(TAG, "Init FaceEngineLcd %dx%d", cfg_.canvas_w, cfg_.canvas_h);

    // ── Buat container/kanvas wajah ─────────────────────────────────────────
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, cfg_.canvas_w, cfg_.canvas_h);
    lv_obj_center(container_);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_radius(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, cfg_.color_bg, 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_OFF);

    // ── Buat mata kiri ──────────────────────────────────────────────────────
    left_eye_ = lv_obj_create(container_);
    lv_obj_set_style_bg_color(left_eye_, cfg_.color_eye, 0);
    lv_obj_set_style_border_width(left_eye_, 0, 0);
    lv_obj_set_style_radius(left_eye_, cfg_.eye_radius, 0);
    lv_obj_set_size(left_eye_, cfg_.eye_size, cfg_.eye_size);

    // ── Buat mata kanan ─────────────────────────────────────────────────────
    right_eye_ = lv_obj_create(container_);
    lv_obj_set_style_bg_color(right_eye_, cfg_.color_eye, 0);
    lv_obj_set_style_border_width(right_eye_, 0, 0);
    lv_obj_set_style_radius(right_eye_, cfg_.eye_radius, 0);
    lv_obj_set_size(right_eye_, cfg_.eye_size, cfg_.eye_size);

    // ── Buat mulut ──────────────────────────────────────────────────────────
    mouth_ = lv_obj_create(container_);
    lv_obj_set_style_bg_color(mouth_, cfg_.color_mouth, 0);
    lv_obj_set_style_border_width(mouth_, 0, 0);
    lv_obj_set_style_radius(mouth_, cfg_.mouth_idle_r, 0);
    lv_obj_set_size(mouth_, cfg_.mouth_idle_w, cfg_.mouth_idle_h);

    // ── Posisi awal ─────────────────────────────────────────────────────────
    ApplyEyeSize(cfg_.eye_size, cfg_.eye_size);
    ApplyEyePosition(0, 0);
    lv_obj_align(mouth_, LV_ALIGN_CENTER, 0, cfg_.mouth_offset_y);

    // ── Nilai awal animasi speaking ─────────────────────────────────────────
    speak_target_h_  = cfg_.speak_h_mid1;
    speak_current_h_ = cfg_.speak_h_mid1;

    // ── Pasang LVGL timer untuk update animasi ──────────────────────────────
    // Lambda menangkap `this` agar bisa memanggil Update() dari timer
    lv_timer_create(
        [](lv_timer_t* t) {
            auto* engine = static_cast<FaceEngineLcd*>(lv_timer_get_user_data(t));
            engine->Update();
        },
        cfg_.update_interval_ms,
        this
    );

    ESP_LOGI(TAG, "FaceEngineLcd ready. Timer interval: %lu ms", cfg_.update_interval_ms);
}

/**
 * @brief Ubah state animasi wajah.
 *
 * Thread-safe — LVGL timer dipanggil dari task LVGL yang sama,
 * sehingga tidak ada race condition selama tidak dipanggil dari ISR.
 */
void FaceEngineLcd::SetState(FaceStateLcd state) {
    if (state_ == state) return;   // tidak ada perubahan, skip

    state_ = state;

    // Reset drift & blink agar transisi terasa natural
    idle_dx_ = 0;
    idle_dy_ = 0;

    ESP_LOGD(TAG, "State -> %d", static_cast<int>(state));
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPER: Apply posisi & ukuran mata
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Terapkan lebar dan tinggi yang sama ke kedua mata.
 */
void FaceEngineLcd::ApplyEyeSize(int w, int h) {
    lv_obj_set_size(left_eye_,  w, h);
    lv_obj_set_size(right_eye_, w, h);
}

/**
 * @brief Posisikan mata berdasarkan cfg_.eye_gap dan offset drift/listening.
 *
 * Mata kiri berada di sebelah kiri center, mata kanan di sebelah kanan.
 * dx/dy adalah tambahan offset (digunakan saat idle drift dan speaking).
 */
void FaceEngineLcd::ApplyEyePosition(int dx, int dy) {
    lv_obj_align(left_eye_,  LV_ALIGN_CENTER,
                 -(cfg_.eye_gap + cfg_.eye_size / 2) + dx,
                  cfg_.eye_offset_y + dy);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER,
                  (cfg_.eye_gap + cfg_.eye_size / 2) + dx,
                  cfg_.eye_offset_y + dy);
}

// ─────────────────────────────────────────────────────────────────────────────
//  BEHAVIOR PER STATE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Animasi Idle.
 *
 * - Mata bergerak pelan secara acak (eye drift)
 * - Mulut tipis di posisi default
 * - Kedipan terjadi dari luar (eye_h sudah dikurangi oleh Update())
 */
void FaceEngineLcd::IdleBehavior(int eye_h) {
    // Sesekali pilih posisi drift baru
    if (rand() % cfg_.idle_drift_chance == 0) {
        idle_dx_ = (rand() % (cfg_.idle_drift_x * 2 + 1)) - cfg_.idle_drift_x;
        idle_dy_ = (rand() % (cfg_.idle_drift_y * 2 + 1)) - cfg_.idle_drift_y;
    }

    int eye_w = (int)(eye_h * 0.70f);   // lebar ~70% tinggi → tampak alami

    // Terapkan ukuran & posisi dengan drift
    ApplyEyeSize(eye_w, eye_h);
    ApplyEyePosition(idle_dx_, idle_dy_);

    // Mulut tipis, sedikit ikut drift
    lv_obj_set_size(mouth_, cfg_.mouth_idle_w, cfg_.mouth_idle_h);
    lv_obj_set_style_radius(mouth_, cfg_.mouth_idle_r, 0);
    lv_obj_align(mouth_, LV_ALIGN_CENTER,
                 idle_dx_,
                 cfg_.mouth_offset_y + idle_dy_ / 2);
}

/**
 * @brief Animasi Listening.
 *
 * - Mata kanan sedikit lebih besar dari mata kiri (menandakan perhatian)
 * - Mulut kecil (AI sedang mendengarkan, tidak berbicara)
 * - Tidak ada drift
 */
void FaceEngineLcd::ListeningBehavior(int eye_h) {
    // Mata kiri sedikit lebih kecil → asimetri "perhatian"
    int right_h = eye_h;
    int right_w = (int)(right_h * 0.70f);

    int left_h  = eye_h - (cfg_.eye_size / 8);   // ~12% lebih kecil
    if (left_h < 2) left_h = 2;
    int left_w  = (int)(left_h * 0.70f);

    lv_obj_set_size(left_eye_,  left_w,  left_h);
    lv_obj_set_size(right_eye_, right_w, right_h);
    lv_obj_set_style_radius(left_eye_,  cfg_.eye_radius, 0);
    lv_obj_set_style_radius(right_eye_, cfg_.eye_radius, 0);

    // Posisi mata: fixed (tidak ada drift saat listening)
    lv_obj_align(left_eye_,  LV_ALIGN_CENTER,
                 -(cfg_.eye_gap + cfg_.eye_size / 2),
                  cfg_.eye_offset_y);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER,
                  (cfg_.eye_gap + cfg_.eye_size / 2),
                  cfg_.eye_offset_y);

    // Mulut kecil, sedikit ke kiri (ekspresi penasaran)
    lv_obj_set_size(mouth_, cfg_.mouth_listen_w, cfg_.mouth_listen_h);
    lv_obj_set_style_radius(mouth_, cfg_.mouth_listen_r, 0);
    lv_obj_align(mouth_, LV_ALIGN_CENTER,
                 -(cfg_.mouth_listen_w / 8),   // sedikit ke kiri
                  cfg_.mouth_offset_y);
}

/**
 * @brief Animasi Speaking.
 *
 * - Mata stabil (tidak drift, tidak asimetris)
 * - Mulut bergerak naik-turun secara acak seperti sedang bicara
 * - Perubahan target tinggi mulut terjadi setiap speak_interval ms
 * - Gerakan halus (speak_current_h_ mendekati speak_target_h_ per frame)
 */
void FaceEngineLcd::SpeakingBehavior(int eye_h) {
    // ── Mata stabil ─────────────────────────────────────────────────────────
    int eye_w = (int)(eye_h * 0.70f);
    ApplyEyeSize(eye_w, eye_h);
    lv_obj_align(left_eye_,  LV_ALIGN_CENTER,
                 -(cfg_.eye_gap + cfg_.eye_size / 2),
                  cfg_.eye_offset_y);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER,
                  (cfg_.eye_gap + cfg_.eye_size / 2),
                  cfg_.eye_offset_y);

    // ── Animasi mulut ────────────────────────────────────────────────────────
    uint32_t now = lv_tick_get();
    uint32_t interval = (uint32_t)(cfg_.speak_interval_base +
                                   (rand() % cfg_.speak_interval_rand));

    if (now - speak_last_ms_ > interval) {
        speak_last_ms_ = now;

        // Pilih target tinggi mulut secara probabilistik
        int r = rand() % 100;
        if      (r < 20) speak_target_h_ = cfg_.speak_h_small;
        else if (r < 50) speak_target_h_ = cfg_.speak_h_mid1;
        else if (r < 80) speak_target_h_ = cfg_.speak_h_mid2;
        else             speak_target_h_ = cfg_.speak_h_large;
    }

    // Smooth interpolation menuju target
    if      (speak_current_h_ < speak_target_h_) speak_current_h_ += cfg_.speak_step;
    else if (speak_current_h_ > speak_target_h_) speak_current_h_ -= cfg_.speak_step;

    // Clamp agar tidak negatif
    if (speak_current_h_ < 1) speak_current_h_ = 1;

    lv_obj_set_size(mouth_, cfg_.mouth_speak_w, speak_current_h_);
    lv_obj_set_style_radius(mouth_, cfg_.mouth_speak_r, 0);
    lv_obj_align(mouth_, LV_ALIGN_CENTER, 0, cfg_.mouth_offset_y);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UPDATE — dipanggil oleh LVGL timer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Update satu frame animasi.
 *
 * Urutan:
 *  1. Hitung tinggi mata aktual (dengan blink override jika sedang berkedip)
 *  2. Dispatch ke behavior yang sesuai state
 *
 * Dipanggil secara periodik oleh LVGL timer — JANGAN panggil manual.
 */
void FaceEngineLcd::Update() {
    if (!container_) return;

    // ── Proses kedipan (blink) ───────────────────────────────────────────────
    // blink_phase_ == 0 berarti tidak berkedip; 1-3 = fase menutup & membuka
    if (blink_phase_ == 0) {
        // Probabilitas mulai kedip per frame
        if (rand() % cfg_.blink_chance == 0) {
            blink_phase_ = 1;
        }
    }

    // Tinggi mata default dari konfigurasi
    int eye_h = cfg_.eye_size;

    // Override tinggi mata sesuai fase blink
    switch (blink_phase_) {
        case 1:
            eye_h = cfg_.eye_blink_h1;   // setengah menutup
            blink_phase_ = 2;
            break;
        case 2:
            eye_h = cfg_.eye_blink_h2;   // tertutup
            blink_phase_ = 3;
            break;
        case 3:
            eye_h = cfg_.eye_blink_h3;   // membuka kembali
            blink_phase_ = 0;
            break;
        default:
            break;
    }

    // ── Dispatch ke behavior ─────────────────────────────────────────────────
    switch (state_) {
        case FaceStateLcd::Idle:
            IdleBehavior(eye_h);
            break;
        case FaceStateLcd::Listening:
            ListeningBehavior(eye_h);
            break;
        case FaceStateLcd::Speaking:
            SpeakingBehavior(eye_h);
            break;
    }
}
