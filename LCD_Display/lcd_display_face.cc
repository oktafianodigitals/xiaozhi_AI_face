// =============================================================================
// lcd_display.cc  (modified for FaceEngineLcd)
//
// This file replaces the original lcd_display.cc.
// Key differences:
//   • SetupUI() creates a pure black screen and initialises FaceEngineLcd
//     instead of building bars, emojis and chat widgets.
//   • SetEmotion() maps emotion strings to FaceState transitions.
//   • SetChatMessage() / ClearChatMessages() are intentional no-ops;
//     the face engine conveys state visually.
//   • All three driver subclasses (SPI / RGB / MIPI) are unchanged.
// =============================================================================

#include "lcd_display.h"   // the modified header above

#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <esp_psram.h>
#include <vector>
#include <cstring>
#include <src/misc/cache/lv_cache.h>

#include "board.h"
#include "settings.h"
#include "lvgl_theme.h"
#include "assets/lang_config.h"

#define TAG "LcdDisplay"

// ---------------------------------------------------------------------------
// Theme initialisation (kept intact so ThemeManager is populated)
// ---------------------------------------------------------------------------
void LcdDisplay::InitializeLcdThemes() {
    // Minimal theme registration – only background / text colours matter
    // since the face engine draws its own colours.
    auto* dark = new LvglTheme("dark");
    dark->set_background_color(lv_color_hex(0x000000));
    dark->set_text_color(lv_color_hex(0xFFFFFF));
    // (other fields irrelevant for face-only mode, but set sane defaults)
    dark->set_chat_background_color(lv_color_hex(0x000000));
    dark->set_user_bubble_color(lv_color_hex(0x000000));
    dark->set_assistant_bubble_color(lv_color_hex(0x000000));
    dark->set_system_bubble_color(lv_color_hex(0x000000));
    dark->set_system_text_color(lv_color_hex(0xFFFFFF));
    dark->set_border_color(lv_color_hex(0x000000));
    dark->set_low_battery_color(lv_color_hex(0xFF0000));

    auto* light = new LvglTheme("light");
    light->set_background_color(lv_color_hex(0xFFFFFF));
    light->set_text_color(lv_color_hex(0x000000));
    light->set_chat_background_color(lv_color_hex(0xFFFFFF));
    light->set_user_bubble_color(lv_color_hex(0xFFFFFF));
    light->set_assistant_bubble_color(lv_color_hex(0xFFFFFF));
    light->set_system_bubble_color(lv_color_hex(0xFFFFFF));
    light->set_system_text_color(lv_color_hex(0x000000));
    light->set_border_color(lv_color_hex(0xFFFFFF));
    light->set_low_battery_color(lv_color_hex(0xFF0000));

    auto& mgr = LvglThemeManager::GetInstance();
    mgr.RegisterTheme("dark",  dark);
    mgr.RegisterTheme("light", light);
}

// ---------------------------------------------------------------------------
// Base constructor
// ---------------------------------------------------------------------------
LcdDisplay::LcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                       esp_lcd_panel_handle_t panel,
                       int width, int height)
    : panel_io_(panel_io), panel_(panel) {
    width_  = width;
    height_ = height;
    InitializeLcdThemes();

    Settings settings("display", false);
    std::string theme_name = settings.GetString("theme", "dark");
    current_theme_ = LvglThemeManager::GetInstance().GetTheme(theme_name);
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
LcdDisplay::~LcdDisplay() {
    if (display_ != nullptr) lv_display_delete(display_);
    if (panel_   != nullptr) esp_lcd_panel_del(panel_);
    if (panel_io_!= nullptr) esp_lcd_panel_io_del(panel_io_);
}

// ---------------------------------------------------------------------------
// Lock / Unlock (unchanged)
// ---------------------------------------------------------------------------
bool LcdDisplay::Lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }
void LcdDisplay::Unlock()             { lvgl_port_unlock(); }

// ---------------------------------------------------------------------------
// SetupUI  –  THE KEY CHANGE
//   Replaces bars / emoji / chat widgets with a pure FaceEngineLcd canvas.
// ---------------------------------------------------------------------------
void LcdDisplay::SetupUI() {
    if (setup_ui_called_) {
        ESP_LOGW(TAG, "SetupUI() called more than once – ignored");
        return;
    }
    Display::SetupUI();          // sets setup_ui_called_ = true
    DisplayLockGuard lock(this);

    auto* screen = lv_screen_active();

    // Paint the entire screen black so there is no white flash.
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    // Initialise the face engine – it creates its own full-screen bg object
    // and attaches a 50 ms LVGL timer that drives all animations.
    face_engine_.Init(screen, width_, height_);

    // Start in idle state
    face_engine_.SetState(FaceState::Idle);

    ESP_LOGI(TAG, "FaceEngineLcd SetupUI complete (%dx%d)", width_, height_);
}

// ---------------------------------------------------------------------------
// SetTheme – update background colour if ever called at runtime
// ---------------------------------------------------------------------------
void LcdDisplay::SetTheme(Theme* theme) {
    current_theme_ = theme;
    // The face engine uses its own colour constants (CALIBRATION macros),
    // so there is nothing more to do here unless you want dynamic recolouring.
}

// ---------------------------------------------------------------------------
// SetEmotion – map emotion strings to FaceState
//
// The xiaozhi firmware calls SetEmotion() with strings like "idle",
// "listening", "speaking", "thinking" …  Map them here.
// ---------------------------------------------------------------------------
void LcdDisplay::SetEmotion(const char* emotion) {
    if (!emotion) return;

    ESP_LOGI(TAG, "SetEmotion: %s", emotion);

    if (strcmp(emotion, "listening") == 0) {
        face_engine_.SetState(FaceState::Listening);
    } else if (strcmp(emotion, "speaking")  == 0 ||
               strcmp(emotion, "thinking")  == 0 ||
               strcmp(emotion, "loading")   == 0) {
        face_engine_.SetState(FaceState::Speaking);
    } else {
        // idle / neutral / any unknown emotion → idle face
        face_engine_.SetState(FaceState::Idle);
    }
}

// ---------------------------------------------------------------------------
// SetChatMessage / ClearChatMessages – intentional no-ops
//   The face engine communicates state visually; no text overlay.
// ---------------------------------------------------------------------------
void LcdDisplay::SetChatMessage(const char* /*role*/, const char* /*content*/) {
    // No-op: face-only UI has no text chat area.
}

void LcdDisplay::ClearChatMessages() {
    // No-op.
}

// ===========================================================================
// SpiLcdDisplay constructor
//   Identical to original except it no longer creates any UI widgets;
//   that is deferred to SetupUI() above.
// ===========================================================================
SpiLcdDisplay::SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                             esp_lcd_panel_handle_t panel,
                             int width, int height,
                             int offset_x, int offset_y,
                             bool mirror_x, bool mirror_y, bool swap_xy)
    : LcdDisplay(panel_io, panel, width, height) {

    // Blank the panel buffer to black before LVGL takes over
    std::vector<uint16_t> row(width_, 0x0000);   // black in RGB565
    for (int y = 0; y < height_; y++) {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, row.data());
    }

    {
        esp_err_t err = esp_lcd_panel_disp_on_off(panel_, true);
        if (err == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGW(TAG, "disp_on_off not supported – assuming ON");
        } else {
            ESP_ERROR_CHECK(err);
        }
    }

    lv_init();

#if CONFIG_SPIRAM
    size_t psram_mb = esp_psram_get_size() / 1024 / 1024;
    if (psram_mb >= 8) {
        lv_image_cache_resize(2 * 1024 * 1024, true);
    } else if (psram_mb >= 2) {
        lv_image_cache_resize(512 * 1024, true);
    }
#endif

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    lvgl_port_init(&port_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = panel_io_,
        .panel_handle   = panel_,
        .control_handle = nullptr,
        .buffer_size    = static_cast<uint32_t>(width_ * 20),
        .double_buffer  = false,
        .trans_size     = 0,
        .hres           = static_cast<uint32_t>(width_),
        .vres           = static_cast<uint32_t>(height_),
        .monochrome     = false,
        .rotation = {
            .swap_xy  = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma    = 1,
            .buff_spiram = 0,
            .sw_rotate   = 0,
            .swap_bytes  = 1,
            .full_refresh= 0,
            .direct_mode = 0,
        },
    };

    display_ = lvgl_port_add_disp(&disp_cfg);
    if (!display_) {
        ESP_LOGE(TAG, "Failed to add SPI display");
        return;
    }
    if (offset_x || offset_y) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }
    ESP_LOGI(TAG, "SpiLcdDisplay ready (%dx%d)", width_, height_);
}

// ===========================================================================
// RgbLcdDisplay constructor
// ===========================================================================
RgbLcdDisplay::RgbLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                             esp_lcd_panel_handle_t panel,
                             int width, int height,
                             int offset_x, int offset_y,
                             bool mirror_x, bool mirror_y, bool swap_xy)
    : LcdDisplay(panel_io, panel, width, height) {

    std::vector<uint16_t> row(width_, 0x0000);
    for (int y = 0; y < height_; y++) {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, row.data());
    }

    lv_init();

    lvgl_port_cfg_t port_cfg  = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority    = 1;
    port_cfg.timer_period_ms  = 50;
    lvgl_port_init(&port_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = panel_io_,
        .panel_handle  = panel_,
        .buffer_size   = static_cast<uint32_t>(width_ * 20),
        .double_buffer = true,
        .hres          = static_cast<uint32_t>(width_),
        .vres          = static_cast<uint32_t>(height_),
        .rotation = {
            .swap_xy  = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .flags = {
            .buff_dma    = 1,
            .swap_bytes  = 0,
            .full_refresh= 1,
            .direct_mode = 1,
        },
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode      = true,
            .avoid_tearing= true,
        },
    };

    display_ = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
    if (!display_) {
        ESP_LOGE(TAG, "Failed to add RGB display");
        return;
    }
    if (offset_x || offset_y) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }
    ESP_LOGI(TAG, "RgbLcdDisplay ready (%dx%d)", width_, height_);
}

// ===========================================================================
// MipiLcdDisplay constructor
// ===========================================================================
MipiLcdDisplay::MipiLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                               esp_lcd_panel_handle_t panel,
                               int width, int height,
                               int offset_x, int offset_y,
                               bool mirror_x, bool mirror_y, bool swap_xy)
    : LcdDisplay(panel_io, panel, width, height) {

    lv_init();

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = panel_io,
        .panel_handle   = panel,
        .control_handle = nullptr,
        .buffer_size    = static_cast<uint32_t>(width_ * 50),
        .double_buffer  = false,
        .hres           = static_cast<uint32_t>(width_),
        .vres           = static_cast<uint32_t>(height_),
        .monochrome     = false,
        .rotation = {
            .swap_xy  = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .flags = {
            .buff_dma    = true,
            .buff_spiram = false,
            .sw_rotate   = true,
        },
    };

    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = { .avoid_tearing = false },
    };

    display_ = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (!display_) {
        ESP_LOGE(TAG, "Failed to add MIPI display");
        return;
    }
    if (offset_x || offset_y) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }
    ESP_LOGI(TAG, "MipiLcdDisplay ready (%dx%d)", width_, height_);
}
