#ifndef BOARD_H
#define BOARD_H

// Board selection. The default build targets the M5Stack Dial; -DBOARD_CYD
// selects the ESP32-2432S028 "Cheap Yellow Display" (see BoardCYD.h).
//
// BOARD_DISPLAY is the LovyanGFX-style display object the UI flushes to.

#if defined(BOARD_CYD)
  #include "BoardCYD.h"
  #define BOARD_DISPLAY       cydDisplay
  #define BOARD_HAS_ENCODER   0   // on-screen keys stand in for the dial
  #define BOARD_HAS_RFID      0   // no built-in reader
  #define BOARD_NAME          "CYD Display"
#else
  #include <M5Unified.h>
  #define BOARD_DISPLAY       M5.Display
  #define BOARD_HAS_ENCODER   1
  #define BOARD_HAS_RFID      1
  #define BOARD_NAME          "M5Dial Display"
#endif

#endif // BOARD_H
