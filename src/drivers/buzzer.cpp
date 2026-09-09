//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/buzzer.h"

#include <Arduino.h>

Buzzer::Buzzer( const uint8_t pin ) : pin( pin )
{
}

void Buzzer::setup()
{
    pinMode( pin, OUTPUT );
    digitalWrite( pin, LOW );
}

void Buzzer::setOutput( const bool on )
{
    sounding = on;
    digitalWrite( pin, on ? HIGH : LOW );
}

void Buzzer::stop()
{
    totalSteps = 0;
    currentStep = 0;
    repeating = false;
    setOutput( false );
}

void Buzzer::beep( const uint16_t durationMs, const uint32_t nowMs )
{
    stop();
    stepDurations[ 0 ] = durationMs;
    stepStates[ 0 ] = true;
    totalSteps = 1;
    currentStep = 0;
    stepStartTime = ( nowMs == 0 ) ? millis() : nowMs;
    setOutput( true );
}

void Buzzer::doubleBeep( const uint16_t durationMs, const uint16_t gapMs, const uint32_t nowMs )
{
    stop();
    stepDurations[ 0 ] = durationMs;
    stepStates[ 0 ] = true;
    stepDurations[ 1 ] = gapMs;
    stepStates[ 1 ] = false;
    stepDurations[ 2 ] = durationMs;
    stepStates[ 2 ] = true;
    totalSteps = 3;
    currentStep = 0;
    stepStartTime = ( nowMs == 0 ) ? millis() : nowMs;
    setOutput( true );
}

void Buzzer::playAlarmPattern( const uint32_t nowMs )
{
    stop();

    stepDurations[ 0 ] = 150;
    stepStates[ 0 ] = true;
    stepDurations[ 1 ] = 100;
    stepStates[ 1 ] = false;
    stepDurations[ 2 ] = 180;
    stepStates[ 2 ] = true;
    stepDurations[ 3 ] = 800;
    stepStates[ 3 ] = false;

    totalSteps = 4;
    currentStep = 0;
    repeating = true;
    stepStartTime = ( nowMs == 0 ) ? millis() : nowMs;

    setOutput( true );
}

void Buzzer::update( const uint32_t nowMs )
{
    if( totalSteps == 0 ) {
        return;
    }

    if( ( nowMs - stepStartTime ) >= stepDurations[ currentStep ] ) {
        currentStep++;

        if( currentStep >= totalSteps ) {
            if( repeating ) {
                currentStep = 0;
                stepStartTime = nowMs;
                setOutput( stepStates[ 0 ] );
            } else {
                stop();
            }
        } else {
            stepStartTime = nowMs;
            setOutput( stepStates[ currentStep ] );
        }
    }
}
