//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_POTENTIOMETER_H
#define FRIENDLYBOT_DRIVERS_POTENTIOMETER_H

#include <cstdint>

class Potentiometer
{
public:
    explicit Potentiometer( uint8_t pin, float filterAlpha = 0.2f, uint16_t deadband = 15 );

    void setup();
    void update();
    void updateRaw( uint16_t rawAdc );

    [[nodiscard]] uint8_t getPin() const
    {
        return pin;
    }
    [[nodiscard]] uint16_t getRawValue() const
    {
        return currentRaw;
    }
    [[nodiscard]] float getNormalized() const
    {
        const float norm = filteredValue / 4095.0f;
        return inverted ? ( 1.0f - norm ) : norm;
    }
    void setInverted( const bool invert )
    {
        inverted = invert;
    }
    [[nodiscard]] bool isInverted() const
    {
        return inverted;
    }
    [[nodiscard]] uint8_t getStep( uint8_t numSteps, float hysteresis = 0.03f );
    [[nodiscard]] float getMappedNonLinear( float minVal, float maxVal, float curvePower = 2.0f ) const;
    [[nodiscard]] bool hasChanged( float threshold = 0.02f );

private:
    uint8_t pin;
    float filterAlpha;
    uint16_t deadband;

    uint16_t currentRaw = 0;
    uint16_t stableRaw = 0;
    float filteredValue = 0.0f;
    float lastReportedNorm = 0.0f;
    uint8_t lastStep = 0;
    bool initialized = false;
    bool inverted = false;
};

#endif // FRIENDLYBOT_DRIVERS_POTENTIOMETER_H
