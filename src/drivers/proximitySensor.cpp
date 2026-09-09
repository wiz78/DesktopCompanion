//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/proximitySensor.h"

#include <cmath>

#include <Arduino.h>

DeskOccupantFilter::DeskOccupantFilter( const uint16_t toleranceMm, const uint16_t deltaWakeMm,
                                        const uint32_t lockTimeMs, const uint32_t arrivalDebounceMs,
                                        const uint8_t closeSampleCount,
                                        const uint32_t departureDebounceMs ) : toleranceMm( toleranceMm ),
                                                                               deltaWakeMm( deltaWakeMm ),
                                                                               lockTimeMs( lockTimeMs ),
                                                                               arrivalDebounceMs( arrivalDebounceMs ),
                                                                               requiredCloseSamples( closeSampleCount ),
                                                                               departureDebounceMs(
                                                                                   departureDebounceMs )
{
}

void DeskOccupantFilter::update( const uint16_t distanceMm, const uint32_t nowMs )
{
    // Check close wave:
    if( distanceMm >= CLOSE_DISTANCE_DEPARTURE_MM )
        closeSampleCounter = 0;
    else if(( distanceMm <= CLOSE_DISTANCE_MM ) && ( closeSampleCounter != CLOSE_SAMPLE_COUNT_WAIT_UNTIL_LEAVING )) {

        closeSampleCounter++;

        if( closeSampleCounter >= requiredCloseSamples ) {
            occupied = true;
            wakeTriggered = true;
            arrivalCandidateStartTime = 0;
            arrivalCandidateActive = false;
            closeSampleCounter = CLOSE_SAMPLE_COUNT_WAIT_UNTIL_LEAVING;
            return;
        }
    }

    if( !occupied ) {
        departureCandidateStartTime = 0;
        departureCandidateActive = false;

        // Check desk arrival:
        if( distanceMm <= ARRIVAL_DISTANCE_MM ) {
            if( !arrivalCandidateActive ) {
                arrivalCandidateStartTime = nowMs;
                arrivalCandidateActive = true;
            } else if( nowMs - arrivalCandidateStartTime >= arrivalDebounceMs ) {
                occupied = true;
                wakeTriggered = true;
                baselineDistance = distanceMm;
                baselineStartTime = nowMs;
                baselineLocked = false;
                arrivalCandidateStartTime = 0;
                arrivalCandidateActive = false;
                return;
            }
        } else {
            arrivalCandidateStartTime = 0;
            arrivalCandidateActive = false;
        }

        return;
    }

    arrivalCandidateStartTime = 0;
    arrivalCandidateActive = false;

    // Check departure:
    if( distanceMm > DEPARTURE_DISTANCE_MM ) {
        if( !departureCandidateActive ) {
            departureCandidateStartTime = nowMs;
            departureCandidateActive = true;
        } else if( nowMs - departureCandidateStartTime >= departureDebounceMs ) {
            occupied = false;
            baselineLocked = false;
            baselineStartTime = 0;
            departureCandidateStartTime = 0;
            departureCandidateActive = false;
            return;
        }

        return;
    }

    departureCandidateStartTime = 0;
    departureCandidateActive = false;

    // Lean-in wake-up check: sudden approach closer than baseline
    if( baselineLocked && ( distanceMm + deltaWakeMm <= baselineDistance ) ) {
        wakeTriggered = true;
        baselineDistance = distanceMm;
        baselineStartTime = nowMs;
        baselineLocked = false;
        return;
    }

    // Check stability to lock baseline
    if( std::abs( static_cast<int>(distanceMm) - static_cast<int>(baselineDistance) ) <= toleranceMm ) {
        if( baselineStartTime == 0 ) {
            baselineStartTime = nowMs;
        } else if( !baselineLocked && ( nowMs - baselineStartTime >= lockTimeMs ) ) {
            baselineLocked = true;
        }
    } else {
        // Shift baseline tracker
        baselineDistance = distanceMm;
        baselineStartTime = nowMs;
        baselineLocked = false;
    }
}

bool DeskOccupantFilter::shouldWakeUp()
{
    if( wakeTriggered ) {
        wakeTriggered = false;
        return true;
    }

    return false;
}

ProximitySensor::ProximitySensor() = default;

bool ProximitySensor::begin( TwoWire *wire, [[maybe_unused]] const uint8_t i2cAddress )
{
    if( !lox.begin( VL53L0X_I2C_ADDR, false, wire ) ) {
        Serial.println( "VL53L0X init failed!" );
        return false;
    }

    initialized = true;
    Serial.println( "VL53L0X initialized successfully." );
    return true;
}

void ProximitySensor::update( const uint32_t nowMs )
{
    if( !initialized ) {
        return;
    }

    if( nowMs - lastRangingTime >= 60 ) {
        VL53L0X_RangingMeasurementData_t measure;

        lox.rangingTest( &measure, false );

        if( measure.RangeStatus != 4 ) {
            feedRawDistance( measure.RangeMilliMeter, nowMs );
        } else {
            feedRawDistance( 2000, nowMs );
        }

        lastRangingTime = nowMs;
    }
}

void ProximitySensor::feedRawDistance( const uint16_t distanceMm, const uint32_t nowMs )
{
    currentDistance = distanceMm;
    filter.update( distanceMm, nowMs );
}
