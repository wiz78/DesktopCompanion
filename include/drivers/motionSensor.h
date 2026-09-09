//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_MOTIONSENSOR_H
#define FRIENDLYBOT_DRIVERS_MOTIONSENSOR_H

#include <cstdint>

#include <Adafruit_MPU6050.h>
#include <Wire.h>

class MotionSensor
{
public:
    MotionSensor();

    bool begin( TwoWire *wire = &Wire, uint8_t i2cAddress = 0x68 );
    void update( uint32_t nowMs = 0 );
    void feedRawData( float ax, float ay, float az, float tempC, uint32_t nowMs );

    [[nodiscard]] bool wasSlapped();
    [[nodiscard]] bool wasShaken();
    [[nodiscard]] float getTemperatureC() const
    {
        return temperatureC + tempOffsetC;
    }
    void setTemperatureOffsetC( const float offset )
    {
        tempOffsetC = offset;
    }
    [[nodiscard]] float getTemperatureOffsetC() const
    {
        return tempOffsetC;
    }
    [[nodiscard]] float getAccelX() const
    {
        return accelX;
    }
    [[nodiscard]] float getAccelY() const
    {
        return accelY;
    }
    [[nodiscard]] float getAccelZ() const
    {
        return accelZ;
    }
    [[nodiscard]] float getTotalG() const;
    void getTiltAngles( float& pitchDeg, float& rollDeg ) const;
    void enableWakeOnMotion( uint8_t threshold = 20 );

    void setSlapThresholdG( const float g )
    {
        slapThresholdG = g;
    }
    [[nodiscard]] float getSlapThresholdG() const
    {
        return slapThresholdG;
    }

    void setDefaultOrientation( float ux, float uy, float uz );
    void clearDefaultOrientation();
    [[nodiscard]] bool hasOrientationCalibration() const
    {
        return hasCalibratedOrientation;
    }
    [[nodiscard]] bool isUpsideDown() const
    {
        return upsideDown;
    }
    void getFilteredAccel( float& ax, float& ay, float& az ) const
    {
        ax = filteredAccelX;
        ay = filteredAccelY;
        az = filteredAccelZ;
    }

private:
    Adafruit_MPU6050 mpu;
    bool initialized = false;

    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 9.81f;
    float temperatureC = 20.0f;
    float tempOffsetC = 0.0f;

    float slapThresholdG = 1.2f;
    uint32_t lastSlapTime = 0;
    bool slapped = false;

    // Shake detector state
    int directionReversals = 0;
    int8_t lastShakeSign = 0;
    uint32_t shakeWindowStart = 0;
    bool shaken = false;

    // Orientation calibration and upside-down state
    bool hasCalibratedOrientation = false;
    float defaultGravityX = 0.0f;
    float defaultGravityY = 0.0f;
    float defaultGravityZ = 0.0f;

    float filteredAccelX = 0.0f;
    float filteredAccelY = 0.0f;
    float filteredAccelZ = 9.81f;
    bool filterInitialized = false;

    bool upsideDown = false;
    bool candidateUpsideDown = false;
    uint32_t candidateStartTime = 0;
};

#endif // FRIENDLYBOT_DRIVERS_MOTIONSENSOR_H
