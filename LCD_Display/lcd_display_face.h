#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

// =============================================================================
// lcd_display.h  (modified for FaceEngineLcd)
//
// Changes vs original:
//  - Includes face_engine_lcd.h
//  - Adds face_engine_ member to LcdDisplay
//  - Adds SetFaceState() helper
//  - Removes emoji / chat-bubble members that are no longer used
//    (they are kept as nullptr-guarded dead code so the rest of the
//     application compiles without changes)
// =============================================================================

#include "lvgl_display.h"
#include "face_engine_lcd.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

#include <atomic>
#include <memory>

class LcdDisplay : public LvglDisplay {
protected:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t    panel_    = nullptr;

    // ---- Face engine (replaces all emoji / bar UI) ----
    FaceEngineLcd face_engine_;

    // ---- Legacy pointers kept as nullptr so callers compile cleanly ----
    lv_obj_t* top_bar_            = nullptr;
    lv_obj_t* status_bar_         = nullptr;
    lv_obj_t* content_            = nullptr;
    lv_obj_t* container_          = nullptr;
    lv_obj_t* side_bar_           = nullptr;
    lv_obj_t* bottom_bar_         = nullptr;
    lv_obj_t* preview_image_      = nullptr;
    lv_obj_t* emoji_label_        = nullptr;
    lv_obj_t* emoji_image_        = nullptr;
    lv_obj_t* emoji_box_          = nullptr;
    lv_obj_t* chat_message_label_ = nullptr;

    void InitializeLcdThemes();
    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

protected:
    LcdDisplay(esp_lcd_panel_io_handle_t panel_io,
               esp_lcd_panel_handle_t panel,
               int width, int height);

public:
    ~LcdDisplay();

    // ---- Face state control (call from application logic) ----
    void SetFaceState(FaceState state) { face_engine_.SetState(state); }

    // ---- Override display API (no-ops or face-aware implementations) ----
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void ClearChatMessages() override;
    virtual void SetupUI() override;
    virtual void SetTheme(Theme* theme) override;
};

// ---------------------------------------------------------------------------
// Driver-specific subclasses (unchanged interface)
// ---------------------------------------------------------------------------

class SpiLcdDisplay : public LcdDisplay {
public:
    SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                  esp_lcd_panel_handle_t panel,
                  int width, int height,
                  int offset_x, int offset_y,
                  bool mirror_x, bool mirror_y, bool swap_xy);
};

class RgbLcdDisplay : public LcdDisplay {
public:
    RgbLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                  esp_lcd_panel_handle_t panel,
                  int width, int height,
                  int offset_x, int offset_y,
                  bool mirror_x, bool mirror_y, bool swap_xy);
};

class MipiLcdDisplay : public LcdDisplay {
public:
    MipiLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                   esp_lcd_panel_handle_t panel,
                   int width, int height,
                   int offset_x, int offset_y,
                   bool mirror_x, bool mirror_y, bool swap_xy);
};

#endif // LCD_DISPLAY_H
