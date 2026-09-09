#include "drivers/ldrSensor.h"
#include "drivers/powerSense.h"

#include <cassert>
#include <cmath>
#include <iostream>

void testLdrSensor()
{
    LdrSensor ldr( 3, 0.2f, 800, 300 ); // Pin 3, alpha=0.2, dimThresh=800, nightThresh=300
    assert( ldr.getPin() == 3 );

    // Bright daylight (e.g. ADC 2500)
    for( int i = 0; i < 30; ++i ) {
        ldr.updateRaw( 2500 );
    }
    assert( !ldr.shouldDim() );
    assert( !ldr.isNight() );
    assert( ldr.getRawValue() >= 2490 && ldr.getRawValue() <= 2510 );
    assert( ldr.getNormalized() > 0.5f );

    // Dim room (e.g. ADC 600)
    for( int i = 0; i < 30; ++i ) {
        ldr.updateRaw( 600 );
    }
    assert( ldr.shouldDim() );
    assert( !ldr.isNight() );

    // Deep dark night (e.g. ADC 100)
    for( int i = 0; i < 30; ++i ) {
        ldr.updateRaw( 100 );
    }
    assert( ldr.shouldDim() );
    assert( ldr.isNight() );

    // Test dynamic threshold changes
    ldr.setThresholds( 50, 20 );
    assert( !ldr.shouldDim() );
    assert( !ldr.isNight() );

    // Test LdrSensor night threshold setter/getter
    LdrSensor ldrDynamic( 3, 0.15f, 800, 300 );
    assert( ldrDynamic.getNightThreshold() == 300 );
    ldrDynamic.setNightThreshold( 500 );
    assert( ldrDynamic.getNightThreshold() == 500 );
}

void testPowerSense()
{
    PowerSense power( 4 );
    assert( power.getPin() == 4 );

    // Initial USB connection
    power.updateRaw( true, 100 ); // USB plugged in
    assert( power.isUsbPowered() );
    assert( power.wasPowerSourceChanged() );
    assert( !power.wasPowerSourceChanged() ); // Single shot consumption

    // Same state update shouldn't trigger change
    power.updateRaw( true, 150 );
    assert( power.isUsbPowered() );
    assert( !power.wasPowerSourceChanged() );

    // Switch to battery (debounced after >= 100ms)
    power.updateRaw( false, 200 );
    power.updateRaw( false, 305 );
    assert( !power.isUsbPowered() );
    assert( power.wasPowerSourceChanged() );
    assert( !power.wasPowerSourceChanged() );

    // Switch back to USB (debounced after >= 100ms)
    power.updateRaw( true, 400 );
    power.updateRaw( true, 505 );
    assert( power.isUsbPowered() );
    assert( power.wasPowerSourceChanged() );
    assert( !power.wasPowerSourceChanged() );
}

void test_powersense_debounce()
{
    PowerSense ps( 4 );
    ps.updateRaw( true, 0 ); // Initial state: USB
    assert( ps.isUsbPowered() == true );
    assert( ps.wasPowerSourceChanged() == true );

    // Glitch / bounce for 50ms should not change state
    ps.updateRaw( false, 10 );
    assert( ps.isUsbPowered() == true );
    assert( ps.wasPowerSourceChanged() == false );

    ps.updateRaw( false, 60 );
    assert( ps.isUsbPowered() == true );

    // After 100ms of stable false, state changes
    ps.updateRaw( false, 115 );
    assert( ps.isUsbPowered() == false );
    assert( ps.wasPowerSourceChanged() == true );
}

void testInvertedPowerSense()
{
    PowerSense pwrInv( 4 );
    assert( !pwrInv.isInverted() );
    pwrInv.setInverted( true );
    assert( pwrInv.isInverted() );
    pwrInv.updateRaw( false, 0 ); // rawPin = LOW (false) -> when inverted, usbActive = true
    assert( pwrInv.isUsbPowered() == true );
    pwrInv.updateRaw( true, 0 ); // rawPin = HIGH (true) -> when inverted, usbActive = false
    pwrInv.updateRaw( true, 105 );
    assert( pwrInv.isUsbPowered() == false );
}

int main()
{
    testLdrSensor();
    testPowerSense();
    test_powersense_debounce();
    testInvertedPowerSense();

    std::cout << "test_ldr_powersense: All tests passed successfully!\n";
    return 0;
}
