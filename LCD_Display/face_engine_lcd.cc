// =============================================================================
// face_engine_lcd.cc
//
// Animated face engine for color LCD displays in the xiaozhi-ai project.
// Drop-in companion to face_engine.cc (OLED 128x64).
//
// Supports all driver variants exposed by bread-compact-wifi-lcd:
//   SPI  : GC9A01, ST7789, ILI9341, ST7796, ILI9488
//   RGB  : NT35510, ST7262, EK9716
//   MIPI : ILI9881C, JD9365DA, or any panel registered via esp_lcd_mipi_dsi
//
// The face is drawn purely with LVGL primitives (lv_obj rectangles /
// arcs), so it works at any resolution – 240×240, 320×240, 480×320 …
//
// All proportions are derived from the CALIBRATION macros in the header.
// To adapt to a new driver / panel size, only touch those macros.
// =============================================================================

#include "face_engine_lcd.h"

#include <stdlib.h>
#include <esp_log.h>

#define TAG "FaceEngineLcd"

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void FaceEngineLcd::Init(lv_obj_t* parent, int width, int height) {
    disp_w_ = width;
    disp_h_ = height;

    // Use the shorter dimension as the scaling reference so the face looks
    // correct on both portrait and landscape panels.
    int ref = (width < height) ? width : height;

    // Compute pixel dimensions from ratios
    eye_size_base_  = (int)(ref * FACE_EYE_SIZE_RATIO);
    eye_spacing_x_  = (int)(ref * FACE_EYE_SPACING_X_RATIO);
    eye_offset_y_   = (int)(ref * FACE_EYE_OFFSET_Y_RATIO);
    mouth_w_base_   = (int)(width * FACE_MOUTH_WIDTH_RATIO);
    mouth_offset_y_ = (int)(ref * FACE_MOUTH_OFFSET_Y_RATIO);

    ESP_LOGI(TAG, "Init: disp=%dx%d  eye_size=%d  spacing=%d  ey_off=%d  mouth_w=%d",
             disp_w_, disp_h_, eye_size_base_, eye_spacing_x_, eye_offset_y_, mouth_w_base_);

    // -------------------------------------------------------------------------
    // Full-screen background – covers any existing UI completely
    // -------------------------------------------------------------------------
    screen_bg_ = lv_obj_create(parent);
    lv_obj_set_size(screen_bg_, width, height);
    lv_obj_align(screen_bg_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(screen_bg_, 0, 0);
    lv_obj_set_style_border_width(screen_bg_, 0, 0);
    lv_obj_set_style_pad_all(screen_bg_, 0, 0);
    lv_obj_set_style_bg_color(screen_bg_, FACE_COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(screen_bg_, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(screen_bg_, LV_SCROLLBAR_MODE_OFF);

    // -------------------------------------------------------------------------
    // Helper lambda: create one eye rectangle
    // -------------------------------------------------------------------------
    auto make_eye = [&]() -> lv_obj_t* {
        lv_obj_t* e = lv_obj_create(screen_bg_);
        lv_obj_set_style_bg_color(e, FACE_COLOR_FEATURE, 0);
        lv_obj_set_style_bg_opa(e, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(e, 0, 0);
        lv_obj_set_style_pad_all(e, 0, 0);
        lv_obj_set_scrollbar_mode(e, LV_SCROLLBAR_MODE_OFF);
        return e;
    };

    left_eye_  = make_eye();
    right_eye_ = make_eye();

    // -------------------------------------------------------------------------
    // Iris circles (colored accent inside each eye)
    // -------------------------------------------------------------------------
#if FACE_ENABLE_IRIS
    auto make_iris = [&]() -> lv_obj_t* {
        lv_obj_t* ir = lv_obj_create(screen_bg_);
        lv_obj_set_style_bg_color(ir, FACE_COLOR_IRIS, 0);
        lv_obj_set_style_bg_opa(ir, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(ir, 0, 0);
        lv_obj_set_style_pad_all(ir, 0, 0);
        lv_obj_set_scrollbar_mode(ir, LV_SCROLLBAR_MODE_OFF);
        return ir;
    };

    auto make_pupil = [&]() -> lv_obj_t* {
        lv_obj_t* p = lv_obj_create(screen_bg_);
        lv_obj_set_style_bg_color(p, FACE_COLOR_PUPIL, 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(p, 0, 0);
        lv_obj_set_style_pad_all(p, 0, 0);
        lv_obj_set_scrollbar_mode(p, LV_SCROLLBAR_MODE_OFF);
        return p;
    };

    left_iris_   = make_iris();
    right_iris_  = make_iris();
    left_pupil_  = make_pupil();
    right_pupil_ = make_pupil();
#endif

    // -------------------------------------------------------------------------
    // Mouth rectangle
    // -------------------------------------------------------------------------
    mouth_ = lv_obj_create(screen_bg_);
    lv_obj_set_style_bg_color(mouth_, FACE_COLOR_FEATURE, 0);
    lv_obj_set_style_bg_opa(mouth_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(mouth_, 0, 0);
    lv_obj_set_style_pad_all(mouth_, 0, 0);
    lv_obj_set_scrollbar_mode(mouth_, LV_SCROLLBAR_MODE_OFF);

    // -------------------------------------------------------------------------
    // Initial geometry pass (idle state)
    // -------------------------------------------------------------------------
    int init_eye_h = eye_size_base_;
    int init_eye_w = (int)(init_eye_h * FACE_EYE_ASPECT_RATIO);
    int init_radius = (int)(init_eye_h * FACE_EYE_RADIUS_RATIO);
    lv_obj_set_style_radius(left_eye_,  init_radius, 0);
    lv_obj_set_style_radius(right_eye_, init_radius, 0);
    ApplyEyeSize(init_eye_w, init_eye_h);
    AlignEyes(0, 0);

    int init_mouth_h = (int)(eye_size_base_ * 0.25f);
    int init_mouth_radius = (int)(init_mouth_h * FACE_MOUTH_RADIUS_RATIO);
    lv_obj_set_style_radius(mouth_, init_mouth_radius, 0);
    AlignMouth(mouth_w_base_, init_mouth_h, 0);

#if FACE_ENABLE_IRIS
    SetIrisVisible(true);
#endif

    // -------------------------------------------------------------------------
    // LVGL timer → calls Update() every FACE_UPDATE_INTERVAL_MS ms
    // -------------------------------------------------------------------------
    lv_timer_create(
        [](lv_timer_t* t) {
            auto* self = static_cast<FaceEngineLcd*>(lv_timer_get_user_data(t));
            self->Update();
        },
        FACE_UPDATE_INTERVAL_MS, this);

    ESP_LOGI(TAG, "FaceEngineLcd ready");
}

// ---------------------------------------------------------------------------
// SetState
// ---------------------------------------------------------------------------
void FaceEngineLcd::SetState(FaceState state) {
    if (state_ != state) {
        state_ = state;
        // Reset speaking mouth interpolation on state change
        if (state == FaceState::Speaking) {
            speak_mouth_current_ = (int)(eye_size_base_ * 0.15f);
            speak_mouth_target_  = speak_mouth_current_;
        }
    }
}

// ---------------------------------------------------------------------------
// Update  (called by LVGL timer)
// ---------------------------------------------------------------------------
void FaceEngineLcd::Update() {
    if (!screen_bg_) return;

    // ------ Blink logic --------------------------------------------------
    if (blink_phase_ == 0) {
        if (rand() % FACE_BLINK_PERIOD_FRAMES == 0) {
            blink_phase_ = 1;
        }
    }

    int eye_h = eye_size_base_;

    switch (blink_phase_) {
        case 1:
            eye_h = (int)(eye_size_base_ * 0.20f);
            blink_phase_ = 2;
            break;
        case 2:
            eye_h = (int)(eye_size_base_ * 0.05f);
            blink_phase_ = 3;
            break;
        case 3:
            eye_h = (int)(eye_size_base_ * 0.40f);
            blink_phase_ = 0;
            break;
        default:
            break;
    }

    // ------ State dispatch ------------------------------------------------
    switch (state_) {
        case FaceState::Idle:      IdleBehavior(eye_h);      break;
        case FaceState::Listening: ListeningBehavior(eye_h); break;
        case FaceState::Speaking:  SpeakingBehavior(eye_h);  break;
    }
}

// ---------------------------------------------------------------------------
// IdleBehavior
//   • Both eyes equally open
//   • Gentle random gaze drift
//   • Thin resting mouth
// ---------------------------------------------------------------------------
void FaceEngineLcd::IdleBehavior(int eye_h) {
    // Drift every N frames
    idle_drift_cnt_++;
    if (idle_drift_cnt_ >= FACE_IDLE_DRIFT_PERIOD_FRAMES) {
        idle_drift_cnt_ = 0;
        if (rand() % 3 != 0) {          // 2/3 chance to actually move
            int drift_range_x = (int)(disp_w_ * 0.04f);
            int drift_range_y = (int)(disp_h_ * 0.03f);
            idle_offset_x_ = (rand() % (2 * drift_range_x + 1)) - drift_range_x;
            idle_offset_y_ = (rand() % (2 * drift_range_y + 1)) - drift_range_y;
        }
    }

    int eye_w   = (int)(eye_h * FACE_EYE_ASPECT_RATIO);
    int radius  = (int)(eye_h * FACE_EYE_RADIUS_RATIO);
    lv_obj_set_style_radius(left_eye_,  radius, 0);
    lv_obj_set_style_radius(right_eye_, radius, 0);

    ApplyEyeSize(eye_w, eye_h);
    AlignEyes(idle_offset_x_, idle_offset_y_);

    // Iris follows gaze drift
#if FACE_ENABLE_IRIS
    SetIrisVisible(eye_h > eye_size_base_ * 0.3f);
    if (eye_h > eye_size_base_ * 0.3f) {
        int iris_sz  = (int)(eye_h * FACE_IRIS_SIZE_RATIO);
        int pupil_sz = (int)(iris_sz * FACE_PUPIL_SIZE_RATIO);
        iris_sz  = (iris_sz  % 2 == 0) ? iris_sz  : iris_sz  + 1;
        pupil_sz = (pupil_sz % 2 == 0) ? pupil_sz : pupil_sz + 1;

        lv_obj_set_style_radius(left_iris_,   iris_sz / 2, 0);
        lv_obj_set_style_radius(right_iris_,  iris_sz / 2, 0);
        lv_obj_set_style_radius(left_pupil_,  pupil_sz / 2, 0);
        lv_obj_set_style_radius(right_pupil_, pupil_sz / 2, 0);

        lv_obj_set_size(left_iris_,  iris_sz, iris_sz);
        lv_obj_set_size(right_iris_, iris_sz, iris_sz);
        lv_obj_set_size(left_pupil_,  pupil_sz, pupil_sz);
        lv_obj_set_size(right_pupil_, pupil_sz, pupil_sz);

        int cx = disp_w_ / 2;
        int cy = disp_h_ / 2;

        // slight look-direction offset for iris (half of gaze drift)
        int gaze_x = idle_offset_x_ / 2;
        int gaze_y = idle_offset_y_ / 2;

        lv_obj_set_pos(left_iris_,
            cx - eye_spacing_x_ - iris_sz / 2 + gaze_x + idle_offset_x_,
            cy + eye_offset_y_  - iris_sz / 2 + gaze_y + idle_offset_y_);
        lv_obj_set_pos(right_iris_,
            cx + eye_spacing_x_ - iris_sz / 2 + gaze_x + idle_offset_x_,
            cy + eye_offset_y_  - iris_sz / 2 + gaze_y + idle_offset_y_);

        lv_obj_set_pos(left_pupil_,
            cx - eye_spacing_x_ - pupil_sz / 2 + gaze_x + idle_offset_x_,
            cy + eye_offset_y_  - pupil_sz / 2 + gaze_y + idle_offset_y_);
        lv_obj_set_pos(right_pupil_,
            cx + eye_spacing_x_ - pupil_sz / 2 + gaze_x + idle_offset_x_,
            cy + eye_offset_y_  - pupil_sz / 2 + gaze_y + idle_offset_y_);
    }
#endif

    // Resting mouth: wide and thin
    int mouth_h = (int)(eye_size_base_ * 0.22f);
    mouth_h = (mouth_h < 4) ? 4 : mouth_h;
    int mouth_radius = (int)(mouth_h * FACE_MOUTH_RADIUS_RATIO);
    lv_obj_set_style_radius(mouth_, mouth_radius, 0);
    AlignMouth(mouth_w_base_, mouth_h, idle_offset_y_);
}

// ---------------------------------------------------------------------------
// ListeningBehavior
//   • Right eye slightly larger than left (attentive asymmetry)
//   • Eyes centered, no drift
//   • Narrow mouth (focused expression)
// ---------------------------------------------------------------------------
void FaceEngineLcd::ListeningBehavior(int eye_h) {
    int left_h  = eye_h - (int)(eye_size_base_ * 0.07f);
    if (left_h < 2) left_h = 2;
    int right_h = eye_h;

    int left_w  = (int)(left_h  * FACE_EYE_ASPECT_RATIO);
    int right_w = (int)(right_h * FACE_EYE_ASPECT_RATIO);

    int r_l = (int)(left_h  * FACE_EYE_RADIUS_RATIO);
    int r_r = (int)(right_h * FACE_EYE_RADIUS_RATIO);
    lv_obj_set_style_radius(left_eye_,  r_l, 0);
    lv_obj_set_style_radius(right_eye_, r_r, 0);

    // Set sizes individually
    lv_obj_set_size(left_eye_,  left_w,  left_h);
    lv_obj_set_size(right_eye_, right_w, right_h);

    int cx = disp_w_ / 2;
    int cy = disp_h_ / 2;

    lv_obj_set_pos(left_eye_,
        cx - eye_spacing_x_ - left_w  / 2,
        cy + eye_offset_y_  - left_h  / 2);
    lv_obj_set_pos(right_eye_,
        cx + eye_spacing_x_ - right_w / 2,
        cy + eye_offset_y_  - right_h / 2);

#if FACE_ENABLE_IRIS
    SetIrisVisible(eye_h > eye_size_base_ * 0.3f);
    if (eye_h > eye_size_base_ * 0.3f) {
        int iris_sz  = (int)(eye_h * FACE_IRIS_SIZE_RATIO);
        int pupil_sz = (int)(iris_sz * FACE_PUPIL_SIZE_RATIO);
        iris_sz  = (iris_sz  % 2 == 0) ? iris_sz  : iris_sz  + 1;
        pupil_sz = (pupil_sz % 2 == 0) ? pupil_sz : pupil_sz + 1;

        lv_obj_set_style_radius(left_iris_,   iris_sz / 2, 0);
        lv_obj_set_style_radius(right_iris_,  iris_sz / 2, 0);
        lv_obj_set_style_radius(left_pupil_,  pupil_sz / 2, 0);
        lv_obj_set_style_radius(right_pupil_, pupil_sz / 2, 0);

        lv_obj_set_size(left_iris_,   iris_sz, iris_sz);
        lv_obj_set_size(right_iris_,  iris_sz, iris_sz);
        lv_obj_set_size(left_pupil_,  pupil_sz, pupil_sz);
        lv_obj_set_size(right_pupil_, pupil_sz, pupil_sz);

        lv_obj_set_pos(left_iris_,
            cx - eye_spacing_x_ - iris_sz / 2,
            cy + eye_offset_y_  - iris_sz / 2);
        lv_obj_set_pos(right_iris_,
            cx + eye_spacing_x_ - iris_sz / 2,
            cy + eye_offset_y_  - iris_sz / 2);
        lv_obj_set_pos(left_pupil_,
            cx - eye_spacing_x_ - pupil_sz / 2,
            cy + eye_offset_y_  - pupil_sz / 2);
        lv_obj_set_pos(right_pupil_,
            cx + eye_spacing_x_ - pupil_sz / 2,
            cy + eye_offset_y_  - pupil_sz / 2);
    }
#endif

    // Thin, slightly off-centre mouth (concentrated look)
    int mouth_h = (int)(eye_size_base_ * 0.12f);
    mouth_h = (mouth_h < 3) ? 3 : mouth_h;
    int mouth_w = (int)(mouth_w_base_ * 0.55f);
    int mouth_radius = (int)(mouth_h * FACE_MOUTH_RADIUS_RATIO);
    lv_obj_set_style_radius(mouth_, mouth_radius, 0);
    AlignMouth(mouth_w, mouth_h, -(int)(eye_size_base_ * 0.05f));
}

// ---------------------------------------------------------------------------
// SpeakingBehavior
//   • Both eyes equally open (normal size)
//   • Mouth height animates randomly to simulate talking
// ---------------------------------------------------------------------------
void FaceEngineLcd::SpeakingBehavior(int eye_h) {
    uint32_t now = lv_tick_get();

    int eye_w  = (int)(eye_h * FACE_EYE_ASPECT_RATIO);
    int radius = (int)(eye_h * FACE_EYE_RADIUS_RATIO);
    lv_obj_set_style_radius(left_eye_,  radius, 0);
    lv_obj_set_style_radius(right_eye_, radius, 0);

    ApplyEyeSize(eye_w, eye_h);
    AlignEyes(0, 0);

#if FACE_ENABLE_IRIS
    SetIrisVisible(eye_h > eye_size_base_ * 0.3f);
    if (eye_h > eye_size_base_ * 0.3f) {
        int iris_sz  = (int)(eye_h * FACE_IRIS_SIZE_RATIO);
        int pupil_sz = (int)(iris_sz * FACE_PUPIL_SIZE_RATIO);
        iris_sz  = (iris_sz  % 2 == 0) ? iris_sz  : iris_sz  + 1;
        pupil_sz = (pupil_sz % 2 == 0) ? pupil_sz : pupil_sz + 1;

        lv_obj_set_style_radius(left_iris_,   iris_sz / 2, 0);
        lv_obj_set_style_radius(right_iris_,  iris_sz / 2, 0);
        lv_obj_set_style_radius(left_pupil_,  pupil_sz / 2, 0);
        lv_obj_set_style_radius(right_pupil_, pupil_sz / 2, 0);

        lv_obj_set_size(left_iris_,   iris_sz, iris_sz);
        lv_obj_set_size(right_iris_,  iris_sz, iris_sz);
        lv_obj_set_size(left_pupil_,  pupil_sz, pupil_sz);
        lv_obj_set_size(right_pupil_, pupil_sz, pupil_sz);

        int cx = disp_w_ / 2;
        int cy = disp_h_ / 2;

        lv_obj_set_pos(left_iris_,
            cx - eye_spacing_x_ - iris_sz / 2,
            cy + eye_offset_y_  - iris_sz / 2);
        lv_obj_set_pos(right_iris_,
            cx + eye_spacing_x_ - iris_sz / 2,
            cy + eye_offset_y_  - iris_sz / 2);
        lv_obj_set_pos(left_pupil_,
            cx - eye_spacing_x_ - pupil_sz / 2,
            cy + eye_offset_y_  - pupil_sz / 2);
        lv_obj_set_pos(right_pupil_,
            cx + eye_spacing_x_ - pupil_sz / 2,
            cy + eye_offset_y_  - pupil_sz / 2);
    }
#endif

    // Mouth animation: pick a new random height target periodically
    uint32_t interval = FACE_SPEAK_UPDATE_MIN_MS + (rand() % FACE_SPEAK_UPDATE_RAND_MS);
    if (now - speak_last_update_ > interval) {
        speak_last_update_ = now;

        int max_h = (int)(eye_size_base_ * 0.80f);  // maximum open height
        int r = rand() % 100;
        if (r < 20)
            speak_mouth_target_ = (int)(max_h * 0.12f);  // nearly closed
        else if (r < 50)
            speak_mouth_target_ = (int)(max_h * 0.40f);  // half open
        else if (r < 80)
            speak_mouth_target_ = (int)(max_h * 0.70f);  // mostly open
        else
            speak_mouth_target_ = max_h;                  // wide open
    }

    // Smooth interpolation toward target
    int step = (int)(eye_size_base_ * 0.06f);
    step = (step < 2) ? 2 : step;

    if (speak_mouth_current_ < speak_mouth_target_)
        speak_mouth_current_ += step;
    else if (speak_mouth_current_ > speak_mouth_target_)
        speak_mouth_current_ -= step;

    // Clamp
    int min_h = (int)(eye_size_base_ * 0.08f);
    if (speak_mouth_current_ < min_h) speak_mouth_current_ = min_h;
    if (speak_mouth_current_ > (int)(eye_size_base_ * 0.80f))
        speak_mouth_current_ = (int)(eye_size_base_ * 0.80f);

    int mouth_h      = speak_mouth_current_;
    int mouth_radius = (int)(mouth_h * FACE_MOUTH_RADIUS_RATIO);
    lv_obj_set_style_radius(mouth_, mouth_radius, 0);
    AlignMouth(mouth_w_base_, mouth_h, 0);
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

void FaceEngineLcd::ApplyEyeSize(int eye_w, int eye_h) {
    lv_obj_set_size(left_eye_,  eye_w, eye_h);
    lv_obj_set_size(right_eye_, eye_w, eye_h);
}

// Position both eyes symmetrically around the screen centre.
// cx_offset / cy_offset: additional pixel shift (for gaze drift, etc.)
void FaceEngineLcd::AlignEyes(int cx_offset, int cy_offset) {
    int eye_w = lv_obj_get_width(left_eye_);
    int eye_h = lv_obj_get_height(left_eye_);

    int cx = disp_w_ / 2;
    int cy = disp_h_ / 2;

    lv_obj_set_pos(left_eye_,
        cx - eye_spacing_x_ - eye_w / 2 + cx_offset,
        cy + eye_offset_y_  - eye_h / 2 + cy_offset);

    lv_obj_set_pos(right_eye_,
        cx + eye_spacing_x_ - eye_w / 2 + cx_offset,
        cy + eye_offset_y_  - eye_h / 2 + cy_offset);
}

// Position the mouth centred horizontally, at mouth_offset_y_ + cy_offset.
void FaceEngineLcd::AlignMouth(int mouth_w, int mouth_h, int cy_offset) {
    int cx = disp_w_ / 2;
    int cy = disp_h_ / 2;

    lv_obj_set_size(mouth_, mouth_w, mouth_h);
    lv_obj_set_pos(mouth_,
        cx - mouth_w / 2,
        cy + mouth_offset_y_ - mouth_h / 2 + cy_offset);
}

// Show or hide iris + pupil objects
void FaceEngineLcd::SetIrisVisible(bool visible) {
#if FACE_ENABLE_IRIS
    if (visible) {
        lv_obj_remove_flag(left_iris_,   LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(right_iris_,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(left_pupil_,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(right_pupil_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(left_iris_,   LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(right_iris_,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(left_pupil_,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(right_pupil_, LV_OBJ_FLAG_HIDDEN);
    }
#else
    (void)visible;
#endif
}
