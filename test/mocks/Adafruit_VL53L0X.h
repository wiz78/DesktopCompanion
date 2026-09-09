#ifndef TEST_MOCKS_ADAFRUIT_VL53L0X_H
#define TEST_MOCKS_ADAFRUIT_VL53L0X_H

#include "Wire.h"

#include <cstdint>

static constexpr uint8_t VL53L0X_I2C_ADDR = 0x29;

struct VL53L0X_RangingMeasurementData_t
{
    uint8_t RangeStatus = 0;
    uint16_t RangeMilliMeter = 0;
};

class Adafruit_VL53L0X
{
public:
    bool begin( uint8_t i2cAddr = VL53L0X_I2C_ADDR, bool debug = false, TwoWire *wire = &Wire )
    {
        (void)i2cAddr;
        (void)debug;
        (void)wire;
        return true;
    }

    void rangingTest( VL53L0X_RangingMeasurementData_t *measure, bool debug = false )
    {
        (void)debug;
        if( measure != nullptr ) {
            measure->RangeStatus = 0;
            measure->RangeMilliMeter = 2000;
        }
    }
};

#endif // TEST_MOCKS_ADAFRUIT_VL53L0X_H
