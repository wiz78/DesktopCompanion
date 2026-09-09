//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_LDRSENSOR_H
#define FRIENDLYBOT_DRIVERS_LDRSENSOR_H

#include <Arduino.h>

#include <cstdint>

class LdrSensor
{
public:
    explicit LdrSensor(
        uint8_t pin,
        float filterAlpha = 0.15f,
        uint16_t dimThreshold = 800,
        uint16_t nightThreshold = 300
    );

    void setup();
    void update();
    void updateRaw( uint16_t rawAdc );

    [[nodiscard]] uint8_t getPin() const
    {
        return pin;
    }
    [[nodiscard]] uint16_t getRawValue() const
    {
        return static_cast<uint16_t>( filteredValue );
    }
    [[nodiscard]] float getNormalized() const
    {
        return filteredValue / 4095.0f;
    }
    [[nodiscard]] int getIdealDisplayBrightnessLevel() const
    {
        return static_cast<int>( map( std::min( getRawValue(), dimThreshold ), 0, dimThreshold, 0, 3 ));
    }
    [[nodiscard]] bool isNight() const
    {
        return filteredValue < static_cast<float>( nightThreshold );
    }

    void setThresholds( const uint16_t dim, const uint16_t night )
    {
        dimThreshold = dim;
        nightThreshold = night;
    }
    void setNightThreshold( const uint16_t night )
    {
        nightThreshold = night;
    }
    [[nodiscard]] uint16_t getNightThreshold() const
    {
        return nightThreshold;
    }

private:
    uint8_t pin;
    float filterAlpha;
    uint16_t dimThreshold;
    uint16_t nightThreshold;
    float filteredValue = 2048.0f;
    bool initialized = false;
};

#endif // FRIENDLYBOT_DRIVERS_LDRSENSOR_H
