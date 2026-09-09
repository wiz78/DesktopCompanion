//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/powerSense.h"

#include <Arduino.h>

PowerSense::PowerSense( const uint8_t pin ) : pin( pin )
{
}

void PowerSense::setup()
{
    pinMode( pin, INPUT_PULLUP );
}

void PowerSense::update( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();
    const bool rawPin = ( digitalRead( pin ) == HIGH );
    updateRaw( rawPin, currentMs );
}

void PowerSense::updateRaw( const bool rawPin, const uint32_t nowMs )
{
    const bool usbActive = inverted ? !rawPin : rawPin;
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();

    if( !initialized ) {
        usbPowered = usbActive;
        candidateState = usbActive;
        candidateChangeTimeMs = currentMs;
        stateChanged = true;
        initialized = true;
        return;
    }

    if( usbActive != candidateState ) {
        candidateState = usbActive;
        candidateChangeTimeMs = currentMs;
    } else if( candidateState != usbPowered ) {
        if( currentMs - candidateChangeTimeMs >= DEBOUNCE_MS ) {
            usbPowered = candidateState;
            stateChanged = true;
        }
    }
}

bool PowerSense::wasPowerSourceChanged()
{
    if( stateChanged ) {
        stateChanged = false;
        return true;
    }

    return false;
}
