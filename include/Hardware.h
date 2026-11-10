#ifndef HARDWARE_H
#define HARDWARE_H

#include "Board.h"
#include <Arduino.h>
#include "Config.h"

class Hardware {
public:
    static bool init();
    static void update();
    
    // Display control
    static void setBacklight(uint8_t brightness);
    static uint8_t getBacklight();
    static void sleep();
    static void wake();
    
    // Power management
    static void powerOn();
    static void powerOff();
    static float getBatteryVoltage();
    static bool isCharging();
    
    // LED control
    static void setLED(bool state);
    static void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
    
private:
    static uint8_t currentBrightness;
    static bool isSleeping;
};

#endif // HARDWARE_H
