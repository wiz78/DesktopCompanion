#include "hardwareManager.h"

#include <Preferences.h>
#include <cassert>
#include <cmath>
#include <iostream>

void testHardwareManagerDefaultsAndSetters()
{
    Preferences prefs;
    prefs.clear();

    HardwareManager &hal = HardwareManager::instance();
    hal.begin();

    // Verify defaults
    assert( !hal.isInvertLeftPot() );
    assert( !hal.isInvertRightPot() );
    assert( !hal.isInvertPowerSense() );
    assert( std::fabs( hal.getTemperatureOffsetC() - 0.0f ) < 0.001f );
    assert( hal.getNightThreshold() == 300 );
    assert( std::fabs( hal.getSlapThresholdG() - 2.0f ) < 0.001f );

    // Test setters and getters
    hal.setInvertLeftPot( true );
    assert( hal.isInvertLeftPot() );
    assert( hal.getLeftPot().isInverted() );

    hal.setInvertRightPot( true );
    assert( hal.isInvertRightPot() );
    assert( hal.getRightPot().isInverted() );

    hal.setInvertPowerSense( true );
    assert( hal.isInvertPowerSense() );
    assert( hal.getPowerSense().isInverted() );

    hal.setTemperatureOffsetC( -2.5f );
    assert( std::fabs( hal.getTemperatureOffsetC() - ( -2.5f ) ) < 0.001f );
    assert( std::fabs( hal.getMotion().getTemperatureOffsetC() - ( -2.5f ) ) < 0.001f );

    hal.setNightThreshold( 450 );
    assert( hal.getNightThreshold() == 450 );
    assert( hal.getLdr().getNightThreshold() == 450 );

    hal.setSlapThresholdG( 3.5f );
    assert( std::fabs( hal.getSlapThresholdG() - 3.5f ) < 0.001f );
    assert( std::fabs( hal.getMotion().getSlapThresholdG() - 3.5f ) < 0.001f );
}

void testHardwareManagerNvsLoading()
{
    Preferences prefs;
    prefs.begin( "friendlybot" );
    prefs.putUChar( "inv_pot_l", 1 );
    prefs.putUChar( "inv_pot_r", 0 );
    prefs.putUChar( "inv_pwr", 1 );
    prefs.putFloat( "temp_offset", 3.25f );
    prefs.putUShort( "night_thresh", 600 );
    prefs.putFloat( "slap_thresh", 1.8f );
    prefs.end();

    HardwareManager &hal = HardwareManager::instance();
    // Reset state before loading to confirm NVS overrides
    hal.setInvertLeftPot( false );
    hal.setInvertRightPot( true );
    hal.setInvertPowerSense( false );
    hal.setTemperatureOffsetC( 0.0f );
    hal.setNightThreshold( 100 );
    hal.setSlapThresholdG( 5.0f );

    hal.begin();

    assert( hal.isInvertLeftPot() );
    assert( !hal.isInvertRightPot() );
    assert( hal.isInvertPowerSense() );
    assert( std::fabs( hal.getTemperatureOffsetC() - 3.25f ) < 0.001f );
    assert( hal.getNightThreshold() == 600 );
    assert( std::fabs( hal.getSlapThresholdG() - 1.8f ) < 0.001f );

    // Clear preferences for cleanup
    prefs.clear();
}

void testHardwareManagerOrientationCalibrationPersistence()
{
    Preferences prefs;
    prefs.clear();

    HardwareManager& hal = HardwareManager::instance();
    hal.begin();

    // Verify clearDefaultOrientation resets calibration
    hal.clearDefaultOrientation();
    assert( !hal.getMotion().hasOrientationCalibration() );

    // If filtered accel is near zero, saveDefaultOrientation should return false
    hal.getMotion().feedRawData( 0.0f, 0.0f, 0.0f, 20.0f, 0 );
    assert( !hal.saveDefaultOrientation() );
    assert( !hal.getMotion().hasOrientationCalibration() );

    // Feed normal orientation vector (e.g. gravity along -Y) and let filter converge
    for( uint32_t t = 10; t <= 500; t += 50 ) {
        hal.getMotion().feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    assert( hal.saveDefaultOrientation() );
    assert( hal.getMotion().hasOrientationCalibration() );
    assert( hal.getDisplay().getToastMessage() == "Orientation Saved" );

    // Verify stored values in Preferences
    prefs.begin( "friendlybot" );
    assert( prefs.getUChar( "orient_cal", 0 ) == 1 );
    assert( std::abs( prefs.getFloat( "orient_x", 0.0f ) - 0.0f ) < 0.01f );
    assert( std::abs( prefs.getFloat( "orient_y", 0.0f ) - ( -1.0f ) ) < 0.01f );
    assert( std::abs( prefs.getFloat( "orient_z", 0.0f ) - 0.0f ) < 0.01f );
    prefs.end();

    // Test clearDefaultOrientation clears motion calibration and removes NVS keys
    hal.clearDefaultOrientation();
    assert( !hal.getMotion().hasOrientationCalibration() );

    prefs.begin( "friendlybot" );
    assert( !prefs.isKey( "orient_cal" ) );
    assert( !prefs.isKey( "orient_x" ) );
    assert( !prefs.isKey( "orient_y" ) );
    assert( !prefs.isKey( "orient_z" ) );
    prefs.end();

    prefs.clear();
}

void testHardwareManagerNvsOrientationLoading()
{
    HardwareManager& hal = HardwareManager::instance();
    hal.clearDefaultOrientation();
    assert( !hal.getMotion().hasOrientationCalibration() );

    Preferences prefs;
    prefs.begin( "friendlybot" );
    prefs.putUChar( "orient_cal", 1 );
    prefs.putFloat( "orient_x", 0.0f );
    prefs.putFloat( "orient_y", -1.0f );
    prefs.putFloat( "orient_z", 0.0f );
    prefs.end();

    hal.begin();
    assert( hal.getMotion().hasOrientationCalibration() );

    // Cleanup
    hal.clearDefaultOrientation();
    prefs.clear();
}

void testHardwareManagerDisplayAutoFlip()
{
    Preferences prefs;
    prefs.clear();

    HardwareManager& hal = HardwareManager::instance();
    hal.begin();
    hal.clearDefaultOrientation();

    assert( !hal.getDisplay().isFlipped() );

    // Calibrate orientation with down vector (0, -1, 0)
    hal.getMotion().setDefaultOrientation( 0.0f, -1.0f, 0.0f );
    assert( hal.getMotion().hasOrientationCalibration() );

    // Upright orientation
    for( uint32_t t = 0; t <= 500; t += 50 ) {
        hal.getMotion().feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    hal.update();
    assert( !hal.getDisplay().isFlipped() );

    // Inverted orientation (converge + debounce > 400ms)
    for( uint32_t t = 550; t <= 1500; t += 50 ) {
        hal.getMotion().feedRawData( 0.0f, 9.81f, 0.0f, 20.0f, t );
    }
    assert( hal.getMotion().isUpsideDown() );

    hal.update();
    assert( hal.getDisplay().isFlipped() );

    // Return to upright orientation
    for( uint32_t t = 1550; t <= 2600; t += 50 ) {
        hal.getMotion().feedRawData( 0.0f, -9.81f, 0.0f, 20.0f, t );
    }
    assert( !hal.getMotion().isUpsideDown() );

    hal.update();
    assert( !hal.getDisplay().isFlipped() );

    // Cleanup
    hal.clearDefaultOrientation();
    prefs.clear();
}

int main()
{
    std::cout << "Running HardwareManager Unit Tests...\n";
    testHardwareManagerDefaultsAndSetters();
    std::cout << "PASS: testHardwareManagerDefaultsAndSetters\n";
    testHardwareManagerNvsLoading();
    std::cout << "PASS: testHardwareManagerNvsLoading\n";
    testHardwareManagerOrientationCalibrationPersistence();
    std::cout << "PASS: testHardwareManagerOrientationCalibrationPersistence\n";
    testHardwareManagerNvsOrientationLoading();
    std::cout << "PASS: testHardwareManagerNvsOrientationLoading\n";
    testHardwareManagerDisplayAutoFlip();
    std::cout << "PASS: testHardwareManagerDisplayAutoFlip\n";

    std::cout << "\n✅ ALL HARDWARE MANAGER TESTS PASSED!\n";
    return 0;
}
