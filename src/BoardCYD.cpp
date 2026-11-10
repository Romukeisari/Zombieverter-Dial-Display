#if defined(BOARD_CYD)

#include "BoardCYD.h"
#include "Config.h"

LGFX_CYD cydDisplay;

// Pin assignments and touch calibration follow LovyanGFX's own autodetect
// entry for the Sunton ESP32-2432S028 (both panel variants).
LGFX_CYD::LGFX_CYD() {
    {
        auto cfg = _bus.config();
        cfg.spi_host    = HSPI_HOST;
        cfg.spi_mode    = 0;
        cfg.freq_write  = 40000000;
        cfg.freq_read   = 16000000;
        cfg.spi_3wire   = false;
        cfg.use_lock    = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk    = 14;
        cfg.pin_mosi    = 13;
        cfg.pin_miso    = 12;
        cfg.pin_dc      = 2;
        _bus.config(cfg);
        _panel.setBus(&_bus);
    }
    {
        auto cfg = _panel.config();
        cfg.pin_cs       = 15;
        cfg.pin_rst      = -1;
        cfg.pin_busy     = -1;
        cfg.panel_width  = 240;
        cfg.panel_height = 320;
        cfg.readable     = true;
        cfg.invert       = false;
        cfg.rgb_order    = false;
        cfg.bus_shared   = false;
#if !defined(CYD_PANEL_ST7789)
        cfg.offset_rotation = 2;
#endif
        _panel.config(cfg);
    }
    {
        auto cfg = _light.config();
        cfg.pin_bl      = 21;
        cfg.invert      = false;
        cfg.freq        = 12000;
        cfg.pwm_channel = 7;
        _light.config(cfg);
        _panel.setLight(&_light);
    }
    {
        // The XPT2046 sits on its own pins, driven by software SPI
        auto cfg = _touch.config();
        cfg.x_min      = 300;
        cfg.x_max      = 3900;
        cfg.y_min      = 3700;
        cfg.y_max      = 200;
        cfg.pin_int    = -1;
        cfg.bus_shared = false;
        cfg.spi_host   = -1;
        cfg.pin_sclk   = 25;
        cfg.pin_mosi   = 32;
        cfg.pin_miso   = 39;
        cfg.pin_cs     = 33;
#if defined(CYD_PANEL_ST7789)
        cfg.offset_rotation = 2;
#endif
        _touch.config(cfg);
        _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
}

namespace {

struct Rect { int16_t x, y, w, h; };

// Key strip geometry — to the right of the UI in landscape, below it in portrait
bool stripIsVertical() {
    return cydDisplay.width() > cydDisplay.height();
}

Rect keyRect(SoftKey key) {
    int index = (int)key - 1;   // PREV, OK, NEXT → 0, 1, 2
    if (stripIsVertical()) {
        int16_t w = cydDisplay.width() - SCREEN_WIDTH;
        int16_t h = cydDisplay.height() / 3;
        return { (int16_t)SCREEN_WIDTH, (int16_t)(index * h), w, h };
    }
    int16_t w = cydDisplay.width() / 3;
    int16_t h = cydDisplay.height() - SCREEN_HEIGHT;
    return { (int16_t)(index * w), (int16_t)SCREEN_HEIGHT, w, h };
}

void drawKey(SoftKey key, bool pressed) {
    const uint16_t bg     = pressed ? 0x04B9 : 0x18E3;   // cyan-ish when held, dark grey idle
    const uint16_t border = 0x4A69;
    const uint16_t fg     = TFT_WHITE;

    Rect r = keyRect(key);
    cydDisplay.fillRoundRect(r.x + 3, r.y + 3, r.w - 6, r.h - 6, 8, bg);
    cydDisplay.drawRoundRect(r.x + 3, r.y + 3, r.w - 6, r.h - 6, 8, border);

    int16_t cx = r.x + r.w / 2;
    int16_t cy = r.y + r.h / 2;
    const int16_t s = 14;

    if (key == SOFTKEY_OK) {
        cydDisplay.setFont(&fonts::FreeSansBold12pt7b);
        cydDisplay.setTextDatum(middle_center);
        cydDisplay.setTextColor(fg);
        cydDisplay.drawString("OK", cx, cy);
        return;
    }

    // Arrow pointing "back" for PREV and "forward" for NEXT: up/down when the
    // keys are stacked, left/right when they sit side by side.
    int16_t dir = (key == SOFTKEY_PREV) ? -1 : 1;
    if (stripIsVertical()) {
        cydDisplay.fillTriangle(cx, cy + dir * s, cx - s, cy - dir * s, cx + s, cy - dir * s, fg);
    } else {
        cydDisplay.fillTriangle(cx + dir * s, cy, cx - dir * s, cy - s, cx - dir * s, cy + s, fg);
    }
}

} // namespace

bool CYD::begin() {
    if (!cydDisplay.init()) return false;
    cydDisplay.setRotation(CYD_ROTATION);
    cydDisplay.setColorDepth(16);
    cydDisplay.setSwapBytes(false);
    cydDisplay.fillScreen(TFT_BLACK);
    drawSoftKeys(SOFTKEY_NONE);
    return true;
}

bool CYD::readTouch(uint16_t* x, uint16_t* y) {
    lgfx::touch_point_t tp;
    if (cydDisplay.getTouch(&tp, 1) == 0) return false;
    if (tp.x < 0 || tp.y < 0 || tp.x >= cydDisplay.width() || tp.y >= cydDisplay.height()) return false;
    *x = (uint16_t)tp.x;
    *y = (uint16_t)tp.y;
    return true;
}

SoftKey CYD::softKeyAt(uint16_t x, uint16_t y) {
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) return SOFTKEY_NONE;
    for (uint8_t k = SOFTKEY_PREV; k <= SOFTKEY_NEXT; k++) {
        Rect r = keyRect((SoftKey)k);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return (SoftKey)k;
    }
    return SOFTKEY_NONE;
}

void CYD::drawSoftKeys(SoftKey pressed) {
    cydDisplay.startWrite();
    for (uint8_t k = SOFTKEY_PREV; k <= SOFTKEY_NEXT; k++) {
        drawKey((SoftKey)k, pressed == (SoftKey)k);
    }
    cydDisplay.endWrite();
}

#endif // BOARD_CYD
