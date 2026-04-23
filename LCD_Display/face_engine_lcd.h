#pragma once

/**
 * @file face_engine_lcd.h
 * @brief FaceEngineLcd - Animasi wajah untuk LCD color (LVGL) berbasis face_engine OLED
 *
 * Kompatibel dengan semua driver LCD pada bread-compact-wifi-lcd:
 *   ST7789 (berbagai resolusi), ST7735, ST7796, ILI9341, GC9A01
 *
 * Mendukung tiga kondisi animasi:
 *   - Idle    : Mata berkedip acak, gerakan halus, mulut tipis
 *   - Listening: Mata asimetris (pendengar), mulut kecil
 *   - Speaking : Animasi mulut bergerak sesuai irama bicara
 *
 * ─── KALIBRASI CEPAT ───────────────────────────────────────────────────────
 *  Ubah hanya blok FACE_LCD_CONFIG di file face_engine_lcd.cc untuk menyesuaikan
 *  ukuran, posisi, dan warna sesuai resolusi LCD yang digunakan.
 * ────────────────────────────────────────────────────────────────────────────
 */

#include <lvgl.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
//  ENUM STATE — sama persis dengan face_engine OLED agar drop-in compatible
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Tiga kondisi utama ekspresi wajah AI.
 *
 * Nilai enum ini sengaja identik dengan FaceState di face_engine.h
 * sehingga kode pemanggil (lcd_display.cc) tidak perlu diubah.
 */
enum class FaceStateLcd {
    Idle,       ///< Standby — mata bergerak pelan, mulut diam
    Listening,  ///< Mendengarkan — mata asimetris menunjukkan perhatian
    Speaking    ///< Berbicara — mulut beranimasi sesuai irama bicara
};

// ═══════════════════════════════════════════════════════════════════════════
//  KONFIGURASI KALIBRASI — edit di face_engine_lcd.cc, bukan di sini
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Struktur konfigurasi semua parameter visual FaceEngineLcd.
 *
 * Diisi satu kali di face_engine_lcd.cc melalui FaceEngineLcd::Init().
 * Pisahkan konfigurasi per resolusi/driver di lcd_display.cc sebelum memanggil Init().
 *
 * CONTOH penggunaan di lcd_display.cc:
 * @code
 *   FaceLcdConfig cfg;
 *   cfg.canvas_w = DISPLAY_WIDTH;
 *   cfg.canvas_h = DISPLAY_HEIGHT;
 *   // ... sesuaikan nilai lain
 *   face_engine_lcd_->Init(content_, cfg);
 * @endcode
 */
struct FaceLcdConfig {

    // ── Dimensi kanvas (diisi otomatis dari DISPLAY_WIDTH/HEIGHT) ──────────
    int canvas_w = 240;   ///< Lebar area wajah dalam piksel
    int canvas_h = 240;   ///< Tinggi area wajah dalam piksel

    // ── Ukuran mata ────────────────────────────────────────────────────────
    int eye_size       = 50;   ///< Ukuran mata default (tinggi & lebar)
    int eye_radius     = 8;    ///< Sudut membulat mata (px); 0 = kotak, besar = oval
    int eye_gap        = 20;   ///< Jarak antara kedua mata (piksel tambahan dari tengah)

    // ── Posisi vertikal mata & mulut (relatif terhadap tengah kanvas) ──────
    int eye_offset_y   = -30;  ///< Offset Y mata dari center (negatif = ke atas)
    int mouth_offset_y =  50;  ///< Offset Y mulut dari center (positif = ke bawah)

    // ── Ukuran mulut ───────────────────────────────────────────────────────
    int mouth_idle_w   = 60;   ///< Lebar mulut saat Idle
    int mouth_idle_h   = 12;   ///< Tinggi mulut saat Idle
    int mouth_idle_r   = 6;    ///< Radius mulut Idle

    int mouth_listen_w = 30;   ///< Lebar mulut saat Listening
    int mouth_listen_h =  6;   ///< Tinggi mulut saat Listening
    int mouth_listen_r = 3;    ///< Radius mulut Listening

    int mouth_speak_w  = 50;   ///< Lebar mulut saat Speaking
    int mouth_speak_r  = 14;   ///< Radius mulut Speaking (membuat tampak oval)

    // ── Target tinggi mulut saat speaking (dipilih acak) ──────────────────
    int speak_h_small  =  8;   ///< Mulut hampir tertutup
    int speak_h_mid1   = 20;   ///< Mulut setengah terbuka
    int speak_h_mid2   = 36;   ///< Mulut terbuka sedang
    int speak_h_large  = 55;   ///< Mulut terbuka lebar

    // ── Kecepatan animasi speaking ─────────────────────────────────────────
    int speak_step      = 4;   ///< Langkah perubahan mouth_current per frame (px)
    int speak_interval_base = 80;  ///< Interval minimum antar perubahan target (ms)
    int speak_interval_rand = 80;  ///< Rentang acak tambahan interval (ms)

    // ── Gerakan idle (eye drift) ────────────────────────────────────────────
    int idle_drift_x   = 6;   ///< Maksimum drift horizontal mata saat Idle (px)
    int idle_drift_y   = 4;   ///< Maksimum drift vertikal mata saat Idle (px)
    int idle_drift_chance = 30; ///< 1-dari-N peluang memilih posisi drift baru per frame

    // ── Kedipan (blink) ────────────────────────────────────────────────────
    int blink_chance   = 100;  ///< 1-dari-N peluang mulai kedip per frame
    int eye_blink_h1   =  8;   ///< Tinggi mata fase kedip 1 (setengah menutup)
    int eye_blink_h2   =  1;   ///< Tinggi mata fase kedip 2 (tertutup)
    int eye_blink_h3   = 14;   ///< Tinggi mata fase kedip 3 (membuka kembali)

    // ── Warna elemen (RGB565-compatible, dikonversi lewat lv_color_hex) ────
    lv_color_t color_eye   = lv_color_white();   ///< Warna mata
    lv_color_t color_mouth = lv_color_white();   ///< Warna mulut
    lv_color_t color_bg    = lv_color_black();   ///< Warna background kanvas

    // ── Timer update interval ──────────────────────────────────────────────
    uint32_t update_interval_ms = 40; ///< Interval refresh animasi (ms) — ~25 fps
};

// ═══════════════════════════════════════════════════════════════════════════
//  CLASS FaceEngineLcd
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Engine animasi wajah untuk LCD color LVGL.
 *
 * Cara pakai:
 *  1. Buat instance: FaceEngineLcd* face = new FaceEngineLcd();
 *  2. Siapkan konfigurasi: FaceLcdConfig cfg = MakeConfigForDisplay(w, h);
 *  3. Inisialisasi: face->Init(parent_obj, cfg);
 *  4. Ubah state: face->SetState(FaceStateLcd::Speaking);
 *  5. Update dipanggil otomatis oleh LVGL timer — tidak perlu loop manual.
 */
class FaceEngineLcd {
public:
    /**
     * @brief Inisialisasi engine: buat objek LVGL dan mulai timer animasi.
     * @param parent   Parent LVGL object (biasanya content_ dari LcdDisplay)
     * @param config   Konfigurasi kalibrasi visual (lihat FaceLcdConfig)
     */
    void Init(lv_obj_t* parent, const FaceLcdConfig& config);

    /**
     * @brief Ubah state animasi wajah.
     * @param state  FaceStateLcd::Idle / Listening / Speaking
     */
    void SetState(FaceStateLcd state);

    /**
     * @brief Kembalikan state saat ini.
     */
    FaceStateLcd GetState() const { return state_; }

    /**
     * @brief Akses konfigurasi aktif (untuk debugging / hot-reconfigure).
     */
    const FaceLcdConfig& GetConfig() const { return cfg_; }

    /**
     * @brief Update satu frame — dipanggil oleh LVGL timer, JANGAN panggil manual.
     *
     * Fungsi ini bersifat public agar bisa diakses oleh lambda timer LVGL.
     */
    void Update();

private:
    // ── Behavior per state ──────────────────────────────────────────────────

    /**
     * @brief Animasi saat Idle: gerakan mata pelan + kedipan acak + mulut diam.
     * @param eye_h  Tinggi mata aktual (bisa dikurangi oleh fase blink)
     */
    void IdleBehavior(int eye_h);

    /**
     * @brief Animasi saat Listening: mata asimetris menandakan perhatian + mulut kecil.
     * @param eye_h  Tinggi mata aktual
     */
    void ListeningBehavior(int eye_h);

    /**
     * @brief Animasi saat Speaking: mulut bergerak naik-turun acak + mata stabil.
     * @param eye_h  Tinggi mata aktual
     */
    void SpeakingBehavior(int eye_h);

    /**
     * @brief Terapkan dimensi mata ke kedua objek LVGL sekaligus.
     * @param w  Lebar mata
     * @param h  Tinggi mata
     */
    void ApplyEyeSize(int w, int h);

    /**
     * @brief Terapkan posisi mata dengan offset drift (idle) atau posisi tetap.
     * @param dx  Horizontal offset dari posisi default
     * @param dy  Vertical offset dari posisi default
     */
    void ApplyEyePosition(int dx, int dy);

    // ── LVGL objects ────────────────────────────────────────────────────────
    lv_obj_t* container_  = nullptr;  ///< Kanvas utama (background)
    lv_obj_t* left_eye_   = nullptr;  ///< Mata kiri
    lv_obj_t* right_eye_  = nullptr;  ///< Mata kanan
    lv_obj_t* mouth_      = nullptr;  ///< Mulut

    // ── State & konfigurasi ─────────────────────────────────────────────────
    FaceStateLcd  state_ = FaceStateLcd::Idle;  ///< State animasi aktif
    FaceLcdConfig cfg_;                          ///< Konfigurasi kalibrasi aktif

    // ── Variabel animasi Idle ───────────────────────────────────────────────
    int idle_dx_ = 0;  ///< Drift X mata saat idle
    int idle_dy_ = 0;  ///< Drift Y mata saat idle

    // ── Variabel kedipan ────────────────────────────────────────────────────
    int blink_phase_ = 0;  ///< Fase blink: 0=normal, 1-3=menutup/membuka

    // ── Variabel animasi Speaking ───────────────────────────────────────────
    uint32_t speak_last_ms_   = 0;   ///< Timestamp terakhir pergantian target mulut
    int      speak_target_h_  = 10;  ///< Tinggi target mulut saat speaking
    int      speak_current_h_ = 10;  ///< Tinggi mulut aktual saat ini (smooth)
};
