//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_PROXIMITYSENSOR_H
#define FRIENDLYBOT_DRIVERS_PROXIMITYSENSOR_H

#include <cstdint>

#include <Adafruit_VL53L0X.h>
#include <Wire.h>

class DeskOccupantFilter
{
public:
    static constexpr uint16_t DEFAULT_TOLERANCE_MM = 200;
    static constexpr uint16_t DEFAULT_DELTA_WAKE_MM = 200;
    static constexpr uint32_t DEFAULT_LOCK_TIME_MS = 30000;
    static constexpr uint16_t ARRIVAL_DISTANCE_MM = 1000;
    static constexpr uint16_t CLOSE_DISTANCE_MM = 200;
    static constexpr uint16_t CLOSE_DISTANCE_DEPARTURE_MM = 250;
    static constexpr uint16_t DEPARTURE_DISTANCE_MM = 1200;
    static constexpr uint32_t ARRIVAL_DEBOUNCE_MS = 1000;
    static constexpr uint8_t CLOSE_SAMPLE_COUNT = 2;
    static constexpr uint8_t CLOSE_SAMPLE_COUNT_WAIT_UNTIL_LEAVING = 255;
    static constexpr uint32_t DEPARTURE_DEBOUNCE_MS = 1500;

    explicit DeskOccupantFilter( uint16_t toleranceMm = DEFAULT_TOLERANCE_MM,
                                 uint16_t deltaWakeMm = DEFAULT_DELTA_WAKE_MM,
                                 uint32_t lockTimeMs = DEFAULT_LOCK_TIME_MS,
                                 uint32_t arrivalDebounceMs = ARRIVAL_DEBOUNCE_MS,
                                 uint8_t closeSampleCount = CLOSE_SAMPLE_COUNT,
                                 uint32_t departureDebounceMs = DEPARTURE_DEBOUNCE_MS );

    void update( uint16_t distanceMm, uint32_t nowMs );

    [[nodiscard]] bool shouldWakeUp();
    [[nodiscard]] bool isBaselineLocked() const
    {
        return baselineLocked;
    }
    [[nodiscard]] bool isSleepPermitted() const
    {
        return baselineLocked;
    }
    [[nodiscard]] uint16_t getBaselineMm() const
    {
        return baselineDistance;
    }
    [[nodiscard]] bool isOccupied() const
    {
        return occupied;
    }

private:
    uint16_t toleranceMm;
    uint16_t deltaWakeMm;
    uint32_t lockTimeMs;
    uint32_t arrivalDebounceMs;
    uint8_t requiredCloseSamples;
    uint32_t departureDebounceMs;

    uint32_t arrivalCandidateStartTime = 0;
    bool arrivalCandidateActive = false;
    uint8_t closeSampleCounter = 0;
    uint32_t departureCandidateStartTime = 0;
    bool departureCandidateActive = false;

    uint16_t baselineDistance = 0;
    uint32_t baselineStartTime = 0;
    bool baselineLocked = false;
    bool wakeTriggered = false;
    bool occupied = true;
};

class ProximitySensor
{
public:
    ProximitySensor();

    bool begin( TwoWire *wire = &Wire, uint8_t i2cAddress = 0x29 );
    void update( uint32_t nowMs );
    void feedRawDistance( uint16_t distanceMm, uint32_t nowMs );

    [[nodiscard]] uint16_t getDistanceMm() const
    {
        return currentDistance;
    }
    [[nodiscard]] bool isTargetPresent( const uint16_t thresholdMm = 1000 ) const
    {
        return currentDistance <= thresholdMm;
    }
    [[nodiscard]] bool shouldWakeUp()
    {
        return filter.shouldWakeUp();
    }
    [[nodiscard]] DeskOccupantFilter& getFilter()
    {
        return filter;
    }
    [[nodiscard]] const DeskOccupantFilter& getFilter() const
    {
        return filter;
    }

private:
    Adafruit_VL53L0X lox;
    bool initialized = false;
    uint32_t lastRangingTime = 0;

    uint16_t currentDistance = 2000;
    DeskOccupantFilter filter;
};

#endif // FRIENDLYBOT_DRIVERS_PROXIMITYSENSOR_H
