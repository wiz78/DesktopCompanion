//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/potentiometer.h"

#include <algorithm>
#include <cmath>

#include <Arduino.h>

Potentiometer::Potentiometer( const uint8_t pin, const float filterAlpha, const uint16_t deadband ) : pin( pin ),
    filterAlpha( filterAlpha ), deadband( deadband )
{
}

void Potentiometer::setup()
{
    pinMode( pin, INPUT );
}

void Potentiometer::update()
{
    // Oversample 4x
    uint32_t sum = 0;
    for( int i = 0; i < 4; ++i ) {
        sum += analogRead( pin );
    }
    updateRaw( static_cast<uint16_t>(sum / 4) );
}

void Potentiometer::updateRaw( const uint16_t rawAdc )
{
    currentRaw = rawAdc;

    uint16_t adjustedRaw = rawAdc;
    if( deadband > 0 ) {
        if( adjustedRaw <= deadband ) {
            adjustedRaw = 0;
        } else if( adjustedRaw >= ( 4095 - deadband ) ) {
            adjustedRaw = 4095;
        }
    }

    if( !initialized ) {
        stableRaw = adjustedRaw;
        filteredValue = static_cast<float>(adjustedRaw);
        lastReportedNorm = getNormalized();
        initialized = true;
        return;
    }

    if( std::abs( static_cast<int>(adjustedRaw) - static_cast<int>(stableRaw) ) >= deadband ) {
        stableRaw = adjustedRaw;
    }

    filteredValue = ( filterAlpha * static_cast<float>(stableRaw) ) + ( ( 1.0f - filterAlpha ) * filteredValue );
    filteredValue = std::clamp( filteredValue, 0.0f, 4095.0f );
}

uint8_t Potentiometer::getStep( const uint8_t numSteps, const float hysteresis )
{
    if( numSteps <= 1 ) {
        return 0;
    }

    const float norm = getNormalized();
    const float stepWidth = 1.0f / static_cast<float>(numSteps);

    // Calculate candidate step
    const auto candidate = static_cast<uint8_t>(std::clamp( static_cast<int>(norm / stepWidth), 0,
                                                            static_cast<int>(numSteps - 1) ));

    if( candidate > lastStep ) {
        if( norm >= ( ( static_cast<float>(candidate) * stepWidth ) + hysteresis ) ) {
            lastStep = candidate;
        } else if( candidate - 1 > lastStep ) {
            lastStep = candidate - 1;
        }
    } else if( candidate < lastStep ) {
        if( norm <= ( ( static_cast<float>(candidate + 1) * stepWidth ) - hysteresis ) ) {
            lastStep = candidate;
        } else if( candidate + 1 < lastStep ) {
            lastStep = candidate + 1;
        }
    }

    return lastStep;
}

float Potentiometer::getMappedNonLinear( const float minVal, const float maxVal, const float curvePower ) const
{
    const float norm = getNormalized();
    const float curved = std::pow( norm, curvePower );

    return minVal + ( curved * ( maxVal - minVal ) );
}

bool Potentiometer::hasChanged( const float threshold )
{
    if( const float norm = getNormalized(); std::abs( norm - lastReportedNorm ) >= threshold ) {
        lastReportedNorm = norm;
        return true;
    }

    return false;
}
