//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "hardwareManager.h"

#include <cmath>

#include <Arduino.h>
#include <Wire.h>

#if __has_include( <Preferences.h> )
#include <Preferences.h>
#endif

HardwareManager& HardwareManager::instance()
{
    static HardwareManager inst;
    return inst;
}

HardwareManager::HardwareManager() : actionButton( PIN_BUTTON_ACTION, true, 30 ),
                                     resetButton( PIN_BUTTON_RESET, true, 30 ), leftPot( PIN_POT_LEFT, 0.2f, 15 ),
                                     rightPot( PIN_POT_RIGHT, 0.2f, 15 ), ldr( PIN_LDR, 0.15f, 800, 300 ),
                                     powerSense( PIN_USB_DETECT ), buzzer( PIN_BUZZER )
{
}

void HardwareManager::begin()
{
#if __has_include( <Preferences.h> )
    Preferences prefs; if( prefs.begin( "friendlybot", true ) ) {
        setInvertLeftPot( prefs.getUChar( "inv_pot_l", 0 ) != 0 );
        setInvertRightPot( prefs.getUChar( "inv_pot_r", 0 ) != 0 );
        setInvertPowerSense( prefs.getUChar( "inv_pwr", 0 ) != 0 );
        setTemperatureOffsetC( prefs.getFloat( "temp_offset", 0.0f ) );
        setNightThreshold( prefs.getUShort( "night_thresh", 300 ) );
        setSlapThresholdG( prefs.getFloat( "slap_thresh", 2.0f ) );
        const uint8_t orientCal = prefs.getUChar( "orient_cal", 0 );
        if( orientCal == 1 ) {
            const float ox = prefs.getFloat( "orient_x", 0.0f );
            const float oy = prefs.getFloat( "orient_y", 0.0f );
            const float oz = prefs.getFloat( "orient_z", 0.0f );
            motion.setDefaultOrientation( ox, oy, oz );
        }
        prefs.end();
    }
#endif

    Serial.println( "[HAL] Initializing I2C Bus on GPIO 8 (SDA) / 9 (SCL)..." );
    Wire.begin( PIN_I2C_SDA, PIN_I2C_SCL );
    Wire.setClock( 400000 );

    actionButton.setup();
    resetButton.setup();
    leftPot.setup();
    rightPot.setup();
    ldr.setup();
    powerSense.setup();
    powerSense.update();
    buzzer.setup();

    display.setup();
    motion.begin( &Wire, 0x68 );
    proximity.begin( &Wire, 0x29 );

    Serial.println( "[HAL] HardwareManager initialization complete." );
}

void HardwareManager::update()
{
    const uint32_t now = millis();

    actionButton.update( now );
    resetButton.update( now );
    leftPot.update();
    rightPot.update();
    ldr.update();
    powerSense.update();
    buzzer.update( now );
    motion.update( now );
    proximity.update( now );

    if( motion.hasOrientationCalibration() ) {
        const bool upsideDown = motion.isUpsideDown();
        if( display.isFlipped() != upsideDown ) {
            display.setFlipped( upsideDown );
        }
    }

    display.update( now );
}

bool HardwareManager::saveDefaultOrientation()
{
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    motion.getFilteredAccel( ax, ay, az );

    const float len = std::sqrt( ( ax * ax ) + ( ay * ay ) + ( az * az ) );
    if( len < 0.001f ) {
        return false;
    }

    const float ux = ax / len;
    const float uy = ay / len;
    const float uz = az / len;

    motion.setDefaultOrientation( ux, uy, uz );

#if __has_include( <Preferences.h> )
    Preferences prefs; if( prefs.begin( "friendlybot", false ) ) {
        prefs.putUChar( "orient_cal", 1 );
        prefs.putFloat( "orient_x", ux );
        prefs.putFloat( "orient_y", uy );
        prefs.putFloat( "orient_z", uz );
        prefs.end();
    }
#endif

    display.showToast( "Orientation Saved", 2000, millis() );
    return true;
}

bool HardwareManager::clearDefaultOrientation()
{
    motion.clearDefaultOrientation();
    display.setFlipped( false );
#if __has_include( <Preferences.h> )
    Preferences prefs; if( prefs.begin( "friendlybot", false ) ) {
        prefs.remove( "orient_cal" );
        prefs.remove( "orient_x" );
        prefs.remove( "orient_y" );
        prefs.remove( "orient_z" );
        prefs.end();
    }
#endif
    return true;
}
