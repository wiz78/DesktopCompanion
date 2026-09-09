//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/ldrSensor.h"

#include <algorithm>

#include <Arduino.h>

LdrSensor::LdrSensor( const uint8_t pin, const float filterAlpha, const uint16_t dimThreshold,
                      const uint16_t nightThreshold ) : pin( pin ), filterAlpha( filterAlpha ),
                                                        dimThreshold( dimThreshold ), nightThreshold( nightThreshold )
{
}

void LdrSensor::setup()
{
    pinMode( pin, INPUT );
}

void LdrSensor::update()
{
    updateRaw( analogRead( pin ) );
}

void LdrSensor::updateRaw( const uint16_t rawAdc )
{
    if( !initialized ) {
        filteredValue = static_cast<float>(rawAdc);
        initialized = true;
        return;
    }

    filteredValue = ( filterAlpha * static_cast<float>(rawAdc) ) + ( ( 1.0f - filterAlpha ) * filteredValue );
    filteredValue = std::clamp( filteredValue, 0.0f, 4095.0f );
}
