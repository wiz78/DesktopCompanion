#include "drivers/motionSensor.h"

#include <cassert>
#include <cmath>
#include <iostream>

void testRestingStateAndTemp()
{
    MotionSensor motion;

    // Resting flat at 1g Z-axis (ax=0, ay=0, az=9.81)
    motion.feedRawData( 0.0f, 0.0f, 9.81f, 22.5f, 0 );
    assert( !motion.wasSlapped() );
    assert( !motion.wasShaken() );
    assert( std::abs( motion.getTemperatureC() - 22.5f ) < 0.1f );

    float pitch = 0.0f;
    float roll = 0.0f;
    motion.getTiltAngles( pitch, roll );
    assert( std::abs( pitch ) < 0.5f );
    assert( std::abs( roll ) < 0.5f );
}

void testSlapDetection()
{
    MotionSensor motion;

    // Baseline
    motion.feedRawData( 0.0f, 0.0f, 9.81f, 22.5f, 0 );
    assert( !motion.wasSlapped() );

    // Sudden slap spike: G = 30 m/s^2 (~3g)
    motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 50 );
    assert( motion.wasSlapped() );
    assert( !motion.wasSlapped() ); // Single shot consumption

    // Cooldown check (< 500ms should not trigger a second slap)
    motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 200 );
    assert( !motion.wasSlapped() );

    // After cooldown (> 500ms)
    motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 600 );
    assert( motion.wasSlapped() );
    assert( !motion.wasSlapped() );
}

void testShakeDetection()
{
    MotionSensor motion;

    // Rapid shaking oscillation across X axis
    for( int i = 0; i < 8; ++i ) {
        const float ax = ( ( i % 2 == 0 ) ? 18.0f : -18.0f );
        motion.feedRawData( ax, 0.0f, 9.81f, 22.5f, 100 + ( i * 40 ) );
    }
    assert( motion.wasShaken() );
    assert( !motion.wasShaken() ); // Single shot consumption
}

void testContinuousSinusoidalShake()
{
    MotionSensor motion;

    // Simulate physical smooth shake with intermediate zero-crossing samples:
    // +12 -> +8 -> 0 -> -6 -> -12 -> -8 -> 0 -> +8 -> +14 -> 0 -> -13
    const float samples[ ] = { 12.0f, 8.0f, 0.0f, -6.0f, -12.0f, -8.0f, 0.0f, 8.0f, 14.0f, 0.0f, -13.0f };
    uint32_t t = 100;
    for( const float ax : samples ) {
        motion.feedRawData( ax, 0.0f, 9.81f, 22.5f, t );
        t += 30; // 30ms intervals (~33Hz loop)
    }

    assert( motion.wasShaken() );
    assert( !motion.wasShaken() );
}

void testTiltAngles()
{
    MotionSensor motion;

    // Tilted right: ay = 9.81, az = 0 -> roll = 90 deg
    motion.feedRawData( 0.0f, 9.81f, 0.0f, 22.5f, 0 );
    float pitch = 0.0f;
    float roll = 0.0f;
    motion.getTiltAngles( pitch, roll );
    assert( std::abs( pitch ) < 1.0f );
    assert( std::abs( roll - 90.0f ) < 1.0f );

    // Tilted forward: ax = 9.81, ay = 0, az = 0 -> pitch = -90 deg
    motion.feedRawData( 9.81f, 0.0f, 0.0f, 22.5f, 10 );
    motion.getTiltAngles( pitch, roll );
    assert( std::abs( pitch - ( -90.0f ) ) < 1.0f );
}

void testCustomThreshold()
{
    MotionSensor motion;
    motion.setSlapThresholdG( 5.0f );

    // 3g spike should not trigger when threshold is 5g
    motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 50 );
    assert( !motion.wasSlapped() );

    // 6g spike (6 * 9.81 ~ 58.8 m/s^2)
    motion.feedRawData( 40.0f, 40.0f, 20.0f, 22.5f, 600 );
    assert( motion.wasSlapped() );
}

void testTemperatureOffsetAndThresholds()
{
    MotionSensor motion;
    motion.feedRawData( 0.0f, 0.0f, 9.81f, 22.5f, 0 );
    assert( std::abs( motion.getTemperatureOffsetC() - 0.0f ) < 0.01f );
    motion.setTemperatureOffsetC( -2.5f );
    assert( std::abs( motion.getTemperatureOffsetC() - ( -2.5f ) ) < 0.01f );
    assert( std::abs( motion.getTemperatureC() - 20.0f ) < 0.1f ); // 22.5 - 2.5 = 20.0
    assert( std::abs( motion.getSlapThresholdG() - 1.2f ) < 0.01f );
    motion.setSlapThresholdG( 1.5f );
    assert( std::abs( motion.getSlapThresholdG() - 1.5f ) < 0.01f );
}

void testOrientationCalibrationAndUpsideDown()
{
    MotionSensor motion;
    assert( !motion.hasOrientationCalibration() );
    assert( !motion.isUpsideDown() );

    // Set default down vector to -Y (e.g. perfboard mount where gravity pulls -Y)
    motion.setDefaultOrientation( 0.0f, -9.81f, 0.0f );
    assert( motion.hasOrientationCalibration() );
    assert( !motion.isUpsideDown() );

    // Feed normal upright orientation (0, -9.81, 0)
    for( uint32_t t = 0; t <= 500; t += 50 )
    {
        motion.feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    assert( !motion.isUpsideDown() );

    float fx = 0.0f;
    float fy = 0.0f;
    float fz = 0.0f;
    motion.getFilteredAccel( fx, fy, fz );
    assert( std::abs( fx - 0.0f ) < 0.1f );
    assert( std::abs( fy - ( -9.81f ) ) < 0.5f );

    // Feed inverted / upside-down orientation (0, +9.81, 0)
    // Filter converges toward +9.81 over ~450ms, then 400ms debounce window starts
    for( uint32_t t = 550; t <= 1000; t += 50 )
    {
        motion.feedRawData( 0.0f, 9.81f, 0.0f, 20.0f, t );
        assert( !motion.isUpsideDown() ); // Filter converging / debounce in progress
    }

    // Advance beyond 400ms debounce window (1000 + 400 = 1400ms)
    for( uint32_t t = 1050; t <= 1450; t += 50 )
    {
        motion.feedRawData( 0.0f, 9.81f, 0.0f, 20.0f, t );
    }
    assert( motion.isUpsideDown() );

    // Extreme G spike (slap / shake) should NOT revert or corrupt orientation
    motion.feedRawData( 0.0f, -40.0f, 0.0f, 20.0f, 1500 );
    assert( motion.isUpsideDown() );

    // Return to upright orientation with filter convergence and debounce
    for( uint32_t t = 1550; t <= 2450; t += 50 )
    {
        motion.feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    assert( !motion.isUpsideDown() );

    // Clear calibration
    motion.clearDefaultOrientation();
    assert( !motion.hasOrientationCalibration() );
    assert( !motion.isUpsideDown() );
}

void testOrientationNoiseFiltering()
{
    MotionSensor motion;
    motion.setDefaultOrientation( 0.0f, -9.81f, 0.0f );
    assert( motion.hasOrientationCalibration() );

    // Establish upright baseline
    for( uint32_t t = 0; t <= 500; t += 50 )
    {
        motion.feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    assert( !motion.isUpsideDown() );

    // High frequency alternating noise (e.g. vibrations fluctuating between inverted and upright)
    // Raw Y alternates between +9.81 and -9.81 every 20ms
    for( uint32_t t = 520; t <= 1200; t += 20 )
    {
        const float noiseY = ( ( ( ( t / 20 ) % 2 ) == 0 ) ? 9.81f : -9.81f );
        motion.feedRawData( 0.0f, noiseY, 0.0f, 20.0f, t );
        // The EMA filter averages the high frequency fluctuation so magnitude stays smoothed
        // and does NOT flip to upside down
        assert( !motion.isUpsideDown() );
    }

    // Filtered value should be near 0 due to alternating symmetric noise, smoothing the raw spikes
    float fx = 0.0f;
    float fy = 0.0f;
    float fz = 0.0f;
    motion.getFilteredAccel( fx, fy, fz );
    assert( std::abs( fy ) < 3.0f );
    assert( !motion.isUpsideDown() );

    // Now feed steady inverted orientation: filter settles and debounces to upside-down
    for( uint32_t t = 1250; t <= 2200; t += 50 )
    {
        motion.feedRawData( 0.0f, 9.81f, 0.0f, 20.0f, t );
    }
    assert( motion.isUpsideDown() );
}

int main()
{
    testRestingStateAndTemp();
    testSlapDetection();
    testShakeDetection();
    testContinuousSinusoidalShake();
    testTiltAngles();
    testCustomThreshold();
    testTemperatureOffsetAndThresholds();
    testOrientationCalibrationAndUpsideDown();
    testOrientationNoiseFiltering();

    std::cout << "test_motion_sensor: All tests passed successfully!\n";
    return 0;
}
