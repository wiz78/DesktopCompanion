//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/button.h"

#include <Arduino.h>

Button::Button( const uint8_t pin, const bool activeLow, const uint32_t debounceMs ) : pin( pin ),
    activeLow( activeLow ), debounceMs( debounceMs )
{
}

void Button::setup()
{
    pinMode( pin, activeLow ? INPUT_PULLUP : INPUT );
}

void Button::update( const uint32_t nowMs )
{
    const bool raw = digitalRead( pin );
    update( raw, nowMs );
}

void Button::update( const bool rawPinLevel, const uint32_t nowMs )
{
    const bool pressed = activeLow ? !rawPinLevel : rawPinLevel;

    if( pressed != lastRawState ) {
        lastDebounceTime = nowMs;
        lastRawState = pressed;
    }

    if( pressed ) {
        if( !debouncedState && ( ( nowMs - lastDebounceTime ) >= debounceMs ) ) {
            debouncedState = true;
            pressStartTime = lastDebounceTime;
            longPressFired = false;
        }
    } else {
        if( debouncedState ) {
            debouncedState = false;
            if( !longPressFired ) {
                clicked = true;
            }
        }
    }
}

uint32_t Button::getHoldDurationMs( const uint32_t nowMs ) const
{
    if( !debouncedState ) {
        return 0;
    }

    return nowMs - pressStartTime;
}

bool Button::wasClicked()
{
    if( clicked ) {
        clicked = false;
        return true;
    }

    return false;
}

bool Button::wasLongPressed( const uint32_t thresholdMs, const uint32_t nowMs )
{
    if( debouncedState && !longPressFired && ( getHoldDurationMs( nowMs ) >= thresholdMs ) ) {
        longPressFired = true;
        return true;
    }

    return false;
}
