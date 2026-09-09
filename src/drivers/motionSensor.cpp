//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "drivers/motionSensor.h"

#include <cmath>

#include <Arduino.h>

MotionSensor::MotionSensor() = default;

bool MotionSensor::begin( TwoWire *wire, const uint8_t i2cAddress )
{
    if( !mpu.begin( i2cAddress, wire ) ) {
        Serial.println( "MPU6050 init failed!" );
        return false;
    }

    mpu.setAccelerometerRange( MPU6050_RANGE_8_G );
    mpu.setGyroRange( MPU6050_RANGE_500_DEG );
    mpu.setFilterBandwidth( MPU6050_BAND_44_HZ );

    initialized = true;
    Serial.println( "MPU6050 initialized successfully." );
    return true;
}

void MotionSensor::update( const uint32_t nowMs )
{
    if( !initialized ) {
        return;
    }

    sensors_event_t a;
    sensors_event_t g;
    sensors_event_t temp;
    mpu.getEvent( &a, &g, &temp );

    const uint32_t effectiveNowMs = ( nowMs == 0 ) ? millis() : nowMs;

    feedRawData( a.acceleration.x, a.acceleration.y, a.acceleration.z, temp.temperature, effectiveNowMs );
}

float MotionSensor::getTotalG() const
{
    return std::sqrt( ( accelX * accelX ) + ( accelY * accelY ) + ( accelZ * accelZ ) ) / 9.80665f;
}

void MotionSensor::setDefaultOrientation( const float ux, const float uy, const float uz )
{
    if( const float len = std::sqrt( ( ux * ux ) + ( uy * uy ) + ( uz * uz ) ); len > 0.0001f ) {
        defaultGravityX = ux / len;
        defaultGravityY = uy / len;
        defaultGravityZ = uz / len;
        hasCalibratedOrientation = true;
    }
}

void MotionSensor::clearDefaultOrientation()
{
    hasCalibratedOrientation = false;
    defaultGravityX = 0.0f;
    defaultGravityY = 0.0f;
    defaultGravityZ = 0.0f;
    upsideDown = false;
    candidateUpsideDown = false;
    candidateStartTime = 0;
}

void MotionSensor::feedRawData( const float ax, const float ay, const float az, const float tempC,
                                const uint32_t nowMs )
{
    accelX = ax;
    accelY = ay;
    accelZ = az;
    temperatureC = tempC;

    // 1. Vector low-pass filter and upside-down orientation detection
    if( !filterInitialized ) {
        filteredAccelX = ax;
        filteredAccelY = ay;
        filteredAccelZ = az;
        filterInitialized = true;
    } else {
        static constexpr float ALPHA = 0.8f;
        filteredAccelX = ( ALPHA * filteredAccelX ) + ( ( 1.0f - ALPHA ) * ax );
        filteredAccelY = ( ALPHA * filteredAccelY ) + ( ( 1.0f - ALPHA ) * ay );
        filteredAccelZ = ( ALPHA * filteredAccelZ ) + ( ( 1.0f - ALPHA ) * az );
    }

    if( hasCalibratedOrientation ) {
        const float filteredMag = std::sqrt(
                ( filteredAccelX * filteredAccelX ) + ( filteredAccelY * filteredAccelY ) + (
                    filteredAccelZ * filteredAccelZ ) );
        const float filteredG = filteredMag / 9.80665f;

        if( ( filteredG >= 0.75f ) && ( filteredG <= 1.25f ) && ( filteredMag > 0.001f ) ) {
            const float vx = filteredAccelX / filteredMag;
            const float vy = filteredAccelY / filteredMag;
            const float vz = filteredAccelZ / filteredMag;
            const float dot = ( defaultGravityX * vx ) + ( defaultGravityY * vy ) + ( defaultGravityZ * vz );

            bool targetState = upsideDown;
            if( !upsideDown && ( dot < -0.3f ) ) {
                targetState = true;
            } else if( upsideDown && ( dot > 0.1f ) ) {
                targetState = false;
            }

            if( targetState != candidateUpsideDown ) {
                candidateUpsideDown = targetState;
                candidateStartTime = nowMs;
            } else if( targetState != upsideDown ) {
                if( ( nowMs - candidateStartTime ) >= 400 ) {
                    upsideDown = targetState;
                }
            }
        }
    }

    // 2. Slap Detection: Total G-force magnitude
    if( const float totalG = getTotalG(); ( totalG >= slapThresholdG ) && (
                                              ( lastSlapTime == 0 ) || ( ( nowMs - lastSlapTime ) > 500 ) ) ) {
        slapped = true;
        lastSlapTime = ( nowMs == 0 ) ? 1 : nowMs;
    }

    // 3. Shake Detection: Rapid back and forth acceleration reversals
    static constexpr float SHAKE_THRESHOLD = 7.0f;
    static constexpr uint32_t SHAKE_WINDOW_MS = 650;

    const float lateral = ( std::abs( ax ) > std::abs( ay ) ) ? ax : ay;

    if( lastShakeSign == 0 ) {
        if( lateral > SHAKE_THRESHOLD ) {
            lastShakeSign = 1;
            shakeWindowStart = nowMs;
            directionReversals = 0;
        } else if( lateral < -SHAKE_THRESHOLD ) {
            lastShakeSign = -1;
            shakeWindowStart = nowMs;
            directionReversals = 0;
        }
    } else if( ( lastShakeSign > 0 && lateral < -SHAKE_THRESHOLD ) || (
                   lastShakeSign < 0 && lateral > SHAKE_THRESHOLD ) ) {
        if( ( nowMs - shakeWindowStart ) > SHAKE_WINDOW_MS ) {
            shakeWindowStart = nowMs;
            directionReversals = 1;
            lastShakeSign = ( lateral > 0 ) ? 1 : -1;
        } else {
            directionReversals++;
            lastShakeSign = ( lateral > 0 ) ? 1 : -1;
            if( directionReversals >= 3 ) {
                shaken = true;
                directionReversals = 0;
                lastShakeSign = 0;
            }
        }
    } else if( ( nowMs - shakeWindowStart ) > SHAKE_WINDOW_MS ) {
        directionReversals = 0;
        lastShakeSign = 0;
    }
}

bool MotionSensor::wasSlapped()
{
    if( slapped ) {
        slapped = false;
        return true;
    }

    return false;
}

bool MotionSensor::wasShaken()
{
    if( shaken ) {
        shaken = false;
        return true;
    }

    return false;
}

void MotionSensor::getTiltAngles( float &pitchDeg, float &rollDeg ) const
{
    pitchDeg = std::atan2( -accelX, std::sqrt( ( accelY * accelY ) + ( accelZ * accelZ ) ) ) * ( 180.0f / 3.14159265f );
    rollDeg = std::atan2( accelY, accelZ ) * ( 180.0f / 3.14159265f );
}

void MotionSensor::enableWakeOnMotion( const uint8_t threshold )
{
    if( initialized ) {
        mpu.setHighPassFilter( MPU6050_HIGHPASS_0_63_HZ );
        mpu.setMotionDetectionThreshold( threshold );
        mpu.setMotionDetectionDuration( 2 );
        mpu.setInterruptPinLatch( true );
        mpu.setInterruptPinPolarity( true ); // Active HIGH
        mpu.setMotionInterrupt( true );
    }
}
