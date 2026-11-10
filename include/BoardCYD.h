#ifndef BOARD_CYD_H
#define BOARD_CYD_H

// ESP32-2432S028 "Cheap Yellow Display" support.
//
// The dial UI stays 240x240 and is drawn at the top-left of the 320x240 panel.
// The leftover 80 px strip holds three touch keys that replace the Dial's
// rotary encoder and push button:
//
//     PREV  = one detent counter-clockwise (auto-repeats while held)
//     OK    = button click (double-tap = double-click, hold = long-press)
//     NEXT  = one detent clockwise         (auto-repeats while held)
//
// Build flags:
//     -DCYD_PANEL_ILI9341   original single-USB board
//     -DCYD_PANEL_ST7789    two-USB board (CYD2USB)
//     -DCYD_ROTATION=n      0-3, default 1 (landscape, keys on the right).
//                           Portrait rotations put the keys along the bottom.

#if defined(BOARD_CYD)

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#ifndef CYD_ROTATION
#define CYD_ROTATION 1
#endif

class LGFX_CYD : public lgfx::LGFX_Device {
public:
    LGFX_CYD();

private:
#if defined(CYD_PANEL_ST7789)
    lgfx::Panel_ST7789  _panel;
#else
    lgfx::Panel_ILI9341 _panel;
#endif
    lgfx::Bus_SPI       _bus;
    lgfx::Light_PWM     _light;
    lgfx::Touch_XPT2046 _touch;
};

extern LGFX_CYD cydDisplay;

enum SoftKey : uint8_t {
    SOFTKEY_NONE = 0,   // touch is on the UI area
    SOFTKEY_PREV,
    SOFTKEY_OK,
    SOFTKEY_NEXT
};

namespace CYD {
    // Start the display and touch panel and draw the key strip
    bool begin();

    // Current touch position in screen coordinates; false when not touched
    bool readTouch(uint16_t* x, uint16_t* y);

    // Which key (if any) a screen position falls on
    SoftKey softKeyAt(uint16_t x, uint16_t y);

    // Redraw the key strip, highlighting the key being held
    void drawSoftKeys(SoftKey pressed = SOFTKEY_NONE);
}

#endif // BOARD_CYD
#endif // BOARD_CYD_H
