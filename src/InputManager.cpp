#include "InputManager.h"
#include "Config.h"

InputManager* InputManager::instance = nullptr;

InputManager::InputManager() 
    : lastEncoderPosition(0), touchPressed(false), 
      lastTouchX(0), lastTouchY(0), touchPressTime(0),
      clickCount(0), lastClickTime(0),
      eventHead(0), eventTail(0),
      onEncoderRotate(nullptr), onButtonClick(nullptr),
      onButtonDoubleClick(nullptr), onButtonTripleClick(nullptr),
      onButtonLongPress(nullptr), onTouchPress(nullptr), onTouchTap(nullptr),
      button(ENCODER_BUTTON, true, true) {
    instance = this;
}

bool InputManager::init() {
#if BOARD_HAS_ENCODER
    // Initialize encoder
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachFullQuad(ENCODER_PIN_A, ENCODER_PIN_B);
    encoder.clearCount();
    encoder.setCount(0);
#endif
    lastEncoderPosition = 0;
    
    // Initialize button
    button.attachClick(buttonClickCallback);
    button.attachDoubleClick(buttonDoubleClickCallback);
    button.attachLongPressStart(buttonLongPressCallback);
    button.setClickMs(250);
    button.setPressMs(800);
    
    #if DEBUG_SERIAL
    Serial.println("Input Manager initialized");
    #endif
    
    return true;
}

void InputManager::update() {
    // Update button
    button.tick();

#if BOARD_HAS_ENCODER
    // Check encoderwith sensitivity adjustment
    // The encoder is in 4x quadrature mode, so divide by 4 for proper sensitivity
    int32_t rawPosition = encoder.getCount();
    int32_t currentPosition = rawPosition / 4;  // Divide by 4 to get actual detent clicks
    
    #if DEBUG_SERIAL
    static uint32_t lastDebugTime = 0;
    if (millis() - lastDebugTime > 2000) {
        Serial.printf("[ENC] raw=%d divided=%d last=%d\n", rawPosition, currentPosition, lastEncoderPosition);
        lastDebugTime = millis();
    }
    #endif
    
    if (currentPosition != lastEncoderPosition) {
        int32_t delta = currentPosition - lastEncoderPosition;
        lastEncoderPosition = currentPosition;
        
        #if DEBUG_SERIAL
        Serial.println("========================================");
        Serial.printf("ENCODER ROTATED: raw_delta=%d, actual_delta=%d\n", rawPosition, delta);
        Serial.println("========================================");
        #endif
        
        InputEvent event;
        event.type = (delta > 0) ? INPUT_ENCODER_CW : INPUT_ENCODER_CCW;
        event.encoderDelta = delta;
        event.timestamp = millis();
        enqueueEvent(event);
        
        if (onEncoderRotate) {
            onEncoderRotate(delta);
        }
    }
#endif // BOARD_HAS_ENCODER

    // Check touch
    checkTouch();
}

#if defined(BOARD_CYD)

// One detent of virtual encoder rotation from the PREV / NEXT keys
void InputManager::emitEncoder(int32_t delta) {
    softPosition += delta;
    lastEncoderPosition = softPosition;

    InputEvent event;
    event.type = (delta > 0) ? INPUT_ENCODER_CW : INPUT_ENCODER_CCW;
    event.encoderDelta = delta;
    event.timestamp = millis();
    enqueueEvent(event);

    if (onEncoderRotate) {
        onEncoderRotate(delta);
    }
}

// Resistive panel: touches on the 240x240 UI become the same press/release/tap
// events the Dial produces; touches on the key strip become encoder steps
// (PREV/NEXT, auto-repeating while held) and button click / double-click /
// long-press (OK), with the same timings OneButton uses for the real button.
void InputManager::checkTouch() {
    static const uint32_t DEBOUNCE_MS     = 30;
    static const uint32_t REPEAT_DELAY_MS = 500;
    static const uint32_t REPEAT_MS       = 150;
    static const uint32_t LONG_PRESS_MS   = 800;
    static const uint32_t CLICK_MS        = 250;

    uint32_t now = millis();
    uint16_t x = 0, y = 0;
    bool down = CYD::readTouch(&x, &y);

    if (down != touchRaw) {
        touchRaw = down;
        touchRawSince = now;
    }
    // Keep following the finger until the press is accepted, so the position
    // used is the settled one rather than the first noisy sample
    if (down && !touchPressed) {
        lastTouchX = x;
        lastTouchY = y;
    }

    if (touchRaw != touchPressed && now - touchRawSince >= DEBOUNCE_MS) {
        touchPressed = touchRaw;

        if (touchPressed) {
            touchPressTime = now;
            activeKey = CYD::softKeyAt(lastTouchX, lastTouchY);

            #if DEBUG_TOUCH
            Serial.printf("Touch press: %d, %d key=%d\n", lastTouchX, lastTouchY, activeKey);
            #endif

            if (activeKey == SOFTKEY_NONE) {
                InputEvent event;
                event.type = INPUT_TOUCH_PRESS;
                event.touchX = lastTouchX;
                event.touchY = lastTouchY;
                event.timestamp = now;
                enqueueEvent(event);
                if (onTouchPress) {
                    onTouchPress(lastTouchX, lastTouchY);
                }
            } else {
                CYD::drawSoftKeys((SoftKey)activeKey);
                if (activeKey == SOFTKEY_OK) {
                    okLongFired = false;
                } else {
                    emitEncoder(activeKey == SOFTKEY_NEXT ? 1 : -1);
                    keyRepeatAt = now + REPEAT_DELAY_MS;
                }
            }
        } else {
            if (activeKey == SOFTKEY_NONE) {
                InputEvent event;
                event.type = INPUT_TOUCH_RELEASE;
                event.touchX = lastTouchX;
                event.touchY = lastTouchY;
                event.timestamp = now;
                enqueueEvent(event);

                if (now - touchPressTime < 500) {
                    InputEvent tapEvent = event;
                    tapEvent.type = INPUT_TOUCH_TAP;
                    enqueueEvent(tapEvent);
                    if (onTouchTap) {
                        onTouchTap(lastTouchX, lastTouchY);
                    }
                }
            } else {
                if (activeKey == SOFTKEY_OK && !okLongFired) {
                    if (okClickPending) {
                        okClickPending = false;
                        buttonDoubleClickCallback();
                    } else {
                        okClickPending = true;
                        okClickTime = now;
                    }
                }
                activeKey = SOFTKEY_NONE;
                CYD::drawSoftKeys(SOFTKEY_NONE);
            }
        }
    }

    if (touchPressed) {
        if (activeKey == SOFTKEY_PREV || activeKey == SOFTKEY_NEXT) {
            if ((int32_t)(now - keyRepeatAt) >= 0) {
                emitEncoder(activeKey == SOFTKEY_NEXT ? 1 : -1);
                keyRepeatAt = now + REPEAT_MS;
            }
        } else if (activeKey == SOFTKEY_OK && !okLongFired &&
                   now - touchPressTime >= LONG_PRESS_MS) {
            okLongFired = true;
            okClickPending = false;
            buttonLongPressCallback();
        }
    } else if (okClickPending && now - okClickTime >= CLICK_MS) {
        // No second tap arrived, so it was a single click
        okClickPending = false;
        buttonClickCallback();
    }
}

#else // M5Stack Dial

void InputManager::checkTouch() {
    // NOTE: M5.update() is called by Hardware::update() in the main loop.
    // Do NOT call it here — M5Unified touch state is consumed on first call,
    // so a second M5.update() would clear wasPressed()/wasReleased() before
    // we can read them.
    auto touch = M5.Touch.getDetail();
    
    if (touch.wasPressed()) {
        touchPressed = true;
        lastTouchX = touch.x;
        lastTouchY = touch.y;
        touchPressTime = millis();

        InputEvent event;
        event.type = INPUT_TOUCH_PRESS;
        event.touchX = touch.x;
        event.touchY = touch.y;
        event.timestamp = millis();
        enqueueEvent(event);

        // Fire press callback immediately — no tap-window dependency
        if (onTouchPress) {
            onTouchPress(touch.x, touch.y);
        }

        #if DEBUG_TOUCH
        Serial.printf("Touch press: %d, %d\n", touch.x, touch.y);
        #endif
    }
    else if (touch.wasReleased()) {
        InputEvent event;
        event.type = INPUT_TOUCH_RELEASE;
        event.touchX = touch.x;
        event.touchY = touch.y;
        event.timestamp = millis();
        enqueueEvent(event);
        
        // Check for tap (quick press/release) — 500ms to account for LVGL loop lag
        if (millis() - touchPressTime < 500) {
            InputEvent tapEvent;
            tapEvent.type = INPUT_TOUCH_TAP;
            tapEvent.touchX = touch.x;
            tapEvent.touchY = touch.y;
            tapEvent.timestamp = millis();
            enqueueEvent(tapEvent);
            
            if (onTouchTap) {
                onTouchTap(touch.x, touch.y);
            }
            
            #if DEBUG_TOUCH
            Serial.printf("Touch tap: %d, %d\n", touch.x, touch.y);
            #endif
        }
        
        touchPressed = false;
    }
}

#endif // BOARD_CYD

bool InputManager::hasEvent() {
    return eventHead != eventTail;
}

InputEvent InputManager::getNextEvent() {
    InputEvent event;
    event.type = INPUT_NONE;
    dequeueEvent(event);
    return event;
}

int32_t InputManager::getEncoderPosition() {
#if BOARD_HAS_ENCODER
    return encoder.getCount();
#else
    return softPosition * 4;   // same 4-counts-per-detent scale as the real encoder
#endif
}

void InputManager::resetEncoderPosition() {
#if BOARD_HAS_ENCODER
    encoder.clearCount();
    encoder.setCount(0);
#else
    softPosition = 0;
#endif
    lastEncoderPosition = 0;
}

void InputManager::setOnEncoderRotate(void (*callback)(int32_t)) {
    onEncoderRotate = callback;
}

void InputManager::setOnButtonClick(void (*callback)()) {
    onButtonClick = callback;
}

void InputManager::setOnButtonDoubleClick(void (*callback)()) {
    onButtonDoubleClick = callback;
}

void InputManager::setOnButtonTripleClick(void (*callback)()) {
    onButtonTripleClick = callback;
}

void InputManager::setOnButtonLongPress(void (*callback)()) {
    onButtonLongPress = callback;
}

void InputManager::setOnTouchTap(void (*callback)(uint16_t, uint16_t)) {
    onTouchTap = callback;
}

void InputManager::setOnTouchPress(void (*callback)(uint16_t, uint16_t)) {
    onTouchPress = callback;
}

bool InputManager::enqueueEvent(InputEvent& event) {
    uint8_t next = (eventHead + 1) % 8;
    if (next == eventTail) return false;
    
    eventQueue[eventHead] = event;
    eventHead = next;
    return true;
}

bool InputManager::dequeueEvent(InputEvent& event) {
    if (eventHead == eventTail) return false;
    
    event = eventQueue[eventTail];
    eventTail = (eventTail + 1) % 8;
    return true;
}

void InputManager::buttonClickCallback() {
    if (!instance) return;

    uint32_t now = millis();

    // If too long since last click, reset count
    if (now - instance->lastClickTime > TRIPLE_CLICK_WINDOW_MS) {
        instance->clickCount = 0;
    }
    instance->clickCount++;
    instance->lastClickTime = now;

    // Triple-click fires immediately on 3rd click
    if (instance->clickCount >= 3) {
        instance->clickCount = 0;
        if (instance->onButtonTripleClick) {
            instance->onButtonTripleClick();
        }
        InputEvent event;
        event.type = INPUT_BUTTON_TRIPLE_CLICK;
        event.timestamp = now;
        instance->enqueueEvent(event);
        return;
    }
    // Single/double clicks are deferred — OneButton's doubleClick callback
    // handles those. We only fire single-click here if it's clearly just one.
    // OneButton already handles single vs double disambiguation, so we let
    // the existing onButtonClick callback handle single clicks as before.
    if (instance->onButtonClick) {
        instance->onButtonClick();
    }
    InputEvent event;
    event.type = INPUT_BUTTON_CLICK;
    event.timestamp = now;
    instance->enqueueEvent(event);
}

void InputManager::buttonDoubleClickCallback() {
    if (!instance) return;
    instance->clickCount = 0;  // reset — double already consumed these clicks
    if (instance->onButtonDoubleClick) {
        instance->onButtonDoubleClick();
    }
    InputEvent event;
    event.type = INPUT_BUTTON_DOUBLE_CLICK;
    event.timestamp = millis();
    instance->enqueueEvent(event);
}

void InputManager::buttonTripleClickCallback() {
    if (instance && instance->onButtonTripleClick) {
        instance->onButtonTripleClick();
    }
    if (instance) {
        InputEvent event;
        event.type = INPUT_BUTTON_TRIPLE_CLICK;
        event.timestamp = millis();
        instance->enqueueEvent(event);
    }
}

void InputManager::buttonLongPressCallback() {
    if (instance && instance->onButtonLongPress) {
        instance->onButtonLongPress();
    }
    
    if (instance) {
        InputEvent event;
        event.type = INPUT_BUTTON_LONG_PRESS;
        event.timestamp = millis();
        instance->enqueueEvent(event);
    }
}