#pragma once

#include <lvgl.h>
#include <stdint.h>

// =============================================================================
// FaceEngineLcd - Animated face engine for color LCD displays
// Compatible with: SpiLcdDisplay, RgbLcdDisplay, MipiLcdDisplay
// Supported drivers: GC9A01, ST7789, ILI9341, ST7796, ILI9488, NT35510, etc.
// =============================================================================
//
// CALIBRATION GUIDE:
// ------------------
// All dimension values are expressed as PERCENTAGES of the display size,
// so the face scales automatically for any resolution (240x240, 320x240,
// 480x320, 800x480, etc.).
//
// To tune for your specific driver / screen size, adjust only the constants
// in the "CALIBRATION" section below.  No other code needs to change.
//
// =============================================================================

// ---------------------------------------------------------------------------
// CALIBRATION: Eye geometry  (values = % of display dimension, 0.0 – 1.0)
// ---------------------------------------------------------------------------

// Fraction of the shorter display dimension used as the base eye size
#define FACE_EYE_SIZE_RATIO         0.20f   // eye height when fully open

// Horizontal distance between each eye centre and the screen centre
//   increase → eyes further apart
#define FACE_EYE_SPACING_X_RATIO   0.22f

// Vertical offset of eye centres from screen centre (negative = up)
#define FACE_EYE_OFFSET_Y_RATIO   -0.12f

// Eye width = height * this ratio (1.0 = square, <1.0 = taller than wide)
#define FACE_EYE_ASPECT_RATIO       0.75f

// Corner radius of each eye rectangle (pixels, relative to eye height)
#define FACE_EYE_RADIUS_RATIO       0.20f

// ---------------------------------------------------------------------------
// CALIBRATION: Mouth geometry
// ---------------------------------------------------------------------------

// Mouth width as fraction of display width
#define FACE_MOUTH_WIDTH_RATIO      0.18f

// Mouth Y offset from screen centre (positive = below centre)
#define FACE_MOUTH_OFFSET_Y_RATIO   0.28f

// Mouth corner radius
#define FACE_MOUTH_RADIUS_RATIO     0.35f   // fraction of mouth height

// ---------------------------------------------------------------------------
// CALIBRATION: Colors  (RGB565 hex – use any standard color picker)
// ---------------------------------------------------------------------------

// Background fill (set to 0x000000 for black, 0xFFFFFF for white)
#define FACE_COLOR_BACKGROUND       lv_color_hex(0x000000)

// Eye / mouth fill color
#define FACE_COLOR_FEATURE          lv_color_hex(0xFFFFFF)

// Iris accent color (drawn on top of eye; set same as feature to disable)
#define FACE_COLOR_IRIS             lv_color_hex(0x00BFFF)

// Pupil color
#define FACE_COLOR_PUPIL            lv_color_hex(0x000000)

// ---------------------------------------------------------------------------
// CALIBRATION: Iris & pupil
// ---------------------------------------------------------------------------

// Draw iris accent circle?  1 = yes, 0 = no (plain white eye)
#define FACE_ENABLE_IRIS            1

// Iris diameter as fraction of eye height
#define FACE_IRIS_SIZE_RATIO        0.55f

// Pupil diameter as fraction of iris diameter
#define FACE_PUPIL_SIZE_RATIO       0.45f

// ---------------------------------------------------------------------------
// CALIBRATION: Animation timing  (milliseconds)
// ---------------------------------------------------------------------------

// How often Update() is called by the LVGL timer
#define FACE_UPDATE_INTERVAL_MS     50      // ~20 fps

// Blink: average frames between blinks (at 20 fps → ~6 s avg)
#define FACE_BLINK_PERIOD_FRAMES    120

// Speaking: how often the mouth target changes (ms)
#define FACE_SPEAK_UPDATE_MIN_MS    80
#define FACE_SPEAK_UPDATE_RAND_MS   60

// Idle: how often gaze drifts
#define FACE_IDLE_DRIFT_PERIOD_FRAMES 40

// ---------------------------------------------------------------------------
// FaceState – mirrors the original enum so callers use the same API
// ---------------------------------------------------------------------------
enum class FaceState { Idle, Listening, Speaking };

// ---------------------------------------------------------------------------
// FaceEngineLcd class
// ---------------------------------------------------------------------------
class FaceEngineLcd {
public:
    // Call once after the LcdDisplay has called SetupUI().
    // parent   : pass lv_screen_active() or any full-screen lv_obj
    // width    : display width  in pixels  (LV_HOR_RES)
    // height   : display height in pixels  (LV_VER_RES)
    void Init(lv_obj_t* parent, int width, int height);

    // Change the current animation state
    void SetState(FaceState state);

    // Called automatically by the internal LVGL timer; exposed for testing
    void Update();

private:
    // --- state machine helpers ---
    void IdleBehavior(int eye_h);
    void ListeningBehavior(int eye_h);
    void SpeakingBehavior(int eye_h);

    // --- layout helpers ---
    void ApplyEyeSize(int eye_w, int eye_h);
    void AlignEyes(int cx_offset, int cy_offset);
    void AlignMouth(int mouth_w, int mouth_h, int cy_offset);
    void SetIrisVisible(bool visible);

    // --- lvgl objects ---
    lv_obj_t* screen_bg_   = nullptr;  // full-screen black background
    lv_obj_t* left_eye_    = nullptr;
    lv_obj_t* right_eye_   = nullptr;
    lv_obj_t* left_iris_   = nullptr;
    lv_obj_t* right_iris_  = nullptr;
    lv_obj_t* left_pupil_  = nullptr;
    lv_obj_t* right_pupil_ = nullptr;
    lv_obj_t* mouth_       = nullptr;

    // --- display geometry (set in Init) ---
    int disp_w_    = 240;
    int disp_h_    = 240;

    // Derived pixel values (computed in Init from ratios × disp dimensions)
    int eye_size_base_   = 48;   // base eye height (px)
    int eye_spacing_x_   = 53;   // px from centre to each eye centre
    int eye_offset_y_    = -29;  // px from centre (negative = up)
    int mouth_w_base_    = 43;   // base mouth width (px)
    int mouth_offset_y_  = 67;   // px below centre

    // --- animation state ---
    FaceState state_      = FaceState::Idle;

    // Blink
    int  blink_phase_     = 0;

    // Idle gaze drift
    int  idle_offset_x_   = 0;
    int  idle_offset_y_   = 0;
    int  idle_drift_cnt_  = 0;

    // Speaking mouth
    uint32_t speak_last_update_  = 0;
    int      speak_mouth_target_ = 4;
    int      speak_mouth_current_= 4;
};
