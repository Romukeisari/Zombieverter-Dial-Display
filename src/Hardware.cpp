#include "Hardware.h"
#include <esp_sleep.h>

uint8_t Hardware::currentBrightness = DEFAULT_BRIGHTNESS;
bool Hardware::isSleeping = false;

bool Hardware::init() {
#if defined(BOARD_CYD)
    // RGB LED is active low and floats dimly lit — switch it off
    setLED(false);

    // Display, touch and the on-screen keys
    if (!CYD::begin()) return false;
    BOARD_DISPLAY.setBrightness(currentBrightness);
#else
    auto cfg = M5.config();

    M5.begin(cfg);

    // Hold power on
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);
    
    // Initialize display
    M5.Display.setRotation(0);
    M5.Display.setBrightness(currentBrightness);
    M5.Display.setColorDepth(16);
    M5.Display.setSwapBytes(false);
    M5.Display.fillScreen(TFT_BLACK);
#endif

    return true;
}

void Hardware::update() {
#if !defined(BOARD_CYD)
    M5.update();
#endif
}

void Hardware::setBacklight(uint8_t brightness) {
    currentBrightness = brightness;
    BOARD_DISPLAY.setBrightness(brightness);
}

uint8_t Hardware::getBacklight() {
    return currentBrightness;
}

void Hardware::sleep() {
    if (!isSleeping) {
        setBacklight(0);
        isSleeping = true;
    }
}

void Hardware::wake() {
    if (isSleeping) {
        setBacklight(currentBrightness);
        isSleeping = false;
    }
}

void Hardware::powerOn() {
#if !defined(BOARD_CYD)
    digitalWrite(POWER_HOLD_PIN, HIGH);
#endif
}

void Hardware::powerOff() {
#if !defined(BOARD_CYD)
    digitalWrite(POWER_HOLD_PIN, LOW);
#endif
    // Deep sleep
    esp_deep_sleep_start();
}

float Hardware::getBatteryVoltage() {
    // M5Stack Dial battery reading
    return 0.0f; // Placeholder - implement based on your hardware
}

bool Hardware::isCharging() {
    return false; // Placeholder - implement based on your hardware
}

void Hardware::setLED(bool state) {
#if defined(BOARD_CYD)
    setLEDColor(state ? 255 : 0, state ? 255 : 0, state ? 255 : 0);
#else
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, state ? HIGH : LOW);
#endif
}

void Hardware::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
#if defined(BOARD_CYD)
    // Active-low RGB LED, on/off per channel
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    digitalWrite(LED_PIN_R, r > 0 ? LOW : HIGH);
    digitalWrite(LED_PIN_G, g > 0 ? LOW : HIGH);
    digitalWrite(LED_PIN_B, b > 0 ? LOW : HIGH);
#else
    // M5Stack Dial has RGB LED, implement if available
    // For now, just use simple on/off
    setLED(r > 0 || g > 0 || b > 0);
#endif
}
