#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// Hardware Configuration
// ============================================

// UI canvas — the dial UI is always laid out for 240x240. On the CYD it sits
// at the top-left of the 320x240 panel; the rest holds the on-screen keys.
#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       240
#define SCREEN_CENTER_X     120
#define SCREEN_CENTER_Y     120
#define SCREEN_RADIUS       120

#if defined(BOARD_CYD)
// --------------------------------------------
// ESP32-2432S028 "Cheap Yellow Display"
// --------------------------------------------
#define SCREEN_IS_ROUND     false

// No encoder: the BOOT button acts as the push button, and the on-screen
// PREV / OK / NEXT keys (BoardCYD.h) cover rotation and click.
#define ENCODER_BUTTON      0

// CAN transceiver (e.g. SN65HVD230) on the CN1 connector: GND, IO22, IO27, 3V3
#define CAN_TX_PIN          27
#define CAN_RX_PIN          22

// RGB LED, active low
#define LED_PIN_R           4
#define LED_PIN_G           16
#define LED_PIN_B           17

#else
// --------------------------------------------
// M5Stack Dial
// --------------------------------------------
#define SCREEN_IS_ROUND     true

// Encoder Pins (Built-in on M5Stack Dial)
#define ENCODER_PIN_A       41  // Swapped with PIN_B
#define ENCODER_PIN_B       40  // Swapped with PIN_A
#define ENCODER_BUTTON      42

// Touch Screen (FT6336U)
#define TOUCH_SDA           11
#define TOUCH_SCL           12
#define TOUCH_INT           14
#define TOUCH_RST           -1

// CAN Bus (External Unit - Grove Port)
// USE PORT B (GREY CONNECTOR) - This is the one that works!
// Port B uses GPIO 2 and GPIO 1 (confirmed by testing)
#define CAN_TX_PIN          2   // Port B (Grey) - Pin 1 (Yellow wire)
#define CAN_RX_PIN          1   // Port B (Grey) - Pin 2 (White wire)

// Power & LED
#define POWER_HOLD_PIN      46
#define LED_PIN             21

#endif // BOARD_CYD

// ============================================
// WiFi Configuration
// ============================================

// WiFi AP Mode (for configuration)
#define WIFI_AP_SSID        "ZombieVerter-Display"
#define WIFI_AP_PASSWORD    "zombieverter"
#define WIFI_AP_CHANNEL     1

// Web Server
#define WEB_SERVER_PORT     80

// File Upload
#define MAX_JSON_SIZE       65536  // 64KB - enough for large param files

// ============================================
// Application Configuration
// ============================================

// CAN Bus Settings
#define CAN_BAUDRATE        500000  // 500kbps - standard for ZombieVerter
#define CAN_NODE_ID         3       // YOUR ZombieVerter is on Node 3!

// UI Settings
#define MAX_SCREENS         6
#define MAX_WIDGETS_PER_SCREEN  8
#define DEFAULT_BRIGHTNESS  128
#define SLEEP_TIMEOUT_MS    300000  // 5 minutes

// Data Settings
#define MAX_PARAMETERS      250
#define TX_QUEUE_SIZE       16
#define RX_QUEUE_SIZE       32
#define PARAM_UPDATE_INTERVAL_MS  100

// Motor speed (spot 2016) is the input to the Gear/Motor change interlock, so it
// is polled on its own timer; the round-robin above takes ~25 s per pass.
#define SPEED_POLL_INTERVAL_MS    200
#define SPEED_STALE_MS            1000   // older than this = unknown speed, changes refused
#define GEAR_CHANGE_MAX_RPM       50     // Gear/Motor edits refused above this |rpm|

// Debug
#define DEBUG_SERIAL        false
#define DEBUG_CAN           false  // Enable to see CAN messages
#define DEBUG_SDO           false  // Enable to see SDO TX/RX traffic (very chatty)
#define DEBUG_TOUCH         false

#endif // CONFIG_H
