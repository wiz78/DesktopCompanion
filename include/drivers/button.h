//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_BUTTON_H
#define FRIENDLYBOT_DRIVERS_BUTTON_H

#include <cstdint>

class Button
{
public:
    explicit Button( uint8_t pin, bool activeLow = true, uint32_t debounceMs = 30 );

    void setup();
    void update( uint32_t nowMs );
    void update( bool rawPinLevel, uint32_t nowMs );

    [[nodiscard]] uint8_t getPin() const
    {
        return pin;
    }
    [[nodiscard]] bool isPressed() const
    {
        return debouncedState;
    }
    [[nodiscard]] uint32_t getHoldDurationMs( uint32_t nowMs ) const;
    [[nodiscard]] bool wasClicked();
    [[nodiscard]] bool wasLongPressed( uint32_t thresholdMs, uint32_t nowMs );

private:
    uint8_t pin;
    bool activeLow;
    uint32_t debounceMs;

    bool lastRawState = false;
    bool debouncedState = false;
    uint32_t lastDebounceTime = 0;
    uint32_t pressStartTime = 0;

    bool clicked = false;
    bool longPressFired = false;
};

#endif // FRIENDLYBOT_DRIVERS_BUTTON_H
