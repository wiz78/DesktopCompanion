#ifndef TEST_MOCKS_ADAFRUIT_MPU6050_H
#define TEST_MOCKS_ADAFRUIT_MPU6050_H

#include "Wire.h"

#include <cstdint>

struct sensors_vec_t
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct sensors_event_t
{
    sensors_vec_t acceleration;
    sensors_vec_t gyro;
    float temperature = 20.0f;
};

enum mpu6050_accel_range_t
{
    MPU6050_RANGE_8_G
};

enum mpu6050_gyro_range_t
{
    MPU6050_RANGE_500_DEG
};

enum mpu6050_bandwidth_t
{
    MPU6050_BAND_21_HZ,
    MPU6050_BAND_44_HZ
};

enum mpu6050_highpass_t
{
    MPU6050_HIGHPASS_0_63_HZ
};

class Adafruit_MPU6050
{
public:
    bool begin( uint8_t i2cAddr = 0x68, TwoWire *wire = &Wire )
    {
        (void)i2cAddr;
        (void)wire;
        return true;
    }

    void setAccelerometerRange( mpu6050_accel_range_t range )
    {
        (void)range;
    }

    void setGyroRange( mpu6050_gyro_range_t range )
    {
        (void)range;
    }

    void setFilterBandwidth( mpu6050_bandwidth_t bandwidth )
    {
        (void)bandwidth;
    }

    void getEvent( sensors_event_t *accel, sensors_event_t *gyro, sensors_event_t *temp )
    {
        (void)accel;
        (void)gyro;
        (void)temp;
    }

    void setHighPassFilter( mpu6050_highpass_t filter )
    {
        (void)filter;
    }

    void setMotionDetectionThreshold( uint8_t threshold )
    {
        (void)threshold;
    }

    void setMotionDetectionDuration( uint8_t duration )
    {
        (void)duration;
    }

    void setInterruptPinLatch( bool latch )
    {
        (void)latch;
    }

    void setInterruptPinPolarity( bool activeHigh )
    {
        (void)activeHigh;
    }

    void setMotionInterrupt( bool enable )
    {
        (void)enable;
    }
};

#endif // TEST_MOCKS_ADAFRUIT_MPU6050_H
