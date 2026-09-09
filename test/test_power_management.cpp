#include "services/powerManager.h"
#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/motionSensor.h"
#include "drivers/powerSense.h"
#include "i18n.h"
#include "pins.h"
#include "services/networkWorker.h"

#include <cassert>
#include <iostream>
#include <string>

#include <WiFi.h>
#include <esp_sleep.h>

void test_power_transitions()
{
    std::cout << "Running test_power_transitions...\n";
    PowerSense powerSense( PIN_USB_DETECT );
    DisplayManager display;
    display.setup();
    Buzzer buzzer( PIN_BUZZER );
    buzzer.setup();
    auto &netWorker = NetworkWorker::instance();

    PowerManager pm;
    pm.init( powerSense, display, buzzer, netWorker );

    // 1. Initial connection on USB at t = 100ms
    powerSense.updateRaw( true, 100 );
    pm.update( 100 );
    assert( buzzer.isSounding() );
    assert( display.isToastActive( 100 ) );
    assert( netWorker.getPowerProfile() == PowerProfile::Usb );
    assert( pm.getLastActivityMs() == 100 );

    // Clear buzzer & toast
    buzzer.stop();
    display.clear();

    // 2. Bounce / glitch before debounce threshold (50ms < 100ms)
    powerSense.updateRaw( false, 150 );
    pm.update( 150 );
    assert( !buzzer.isSounding() );
    assert( !display.isToastActive( 150 ) );
    assert( netWorker.getPowerProfile() == PowerProfile::Usb );

    // 3. Stable battery state after >= 100ms debounce (at t = 255ms, 105ms elapsed)
    powerSense.updateRaw( false, 255 );
    pm.update( 255 );
    assert( buzzer.isSounding() );
    assert( display.isToastActive( 255 ) );
    assert( netWorker.getPowerProfile() == PowerProfile::Battery );
    assert( pm.getLastActivityMs() == 255 );

    // Clear buzzer & toast
    buzzer.stop();
    display.clear();

    // 4. Stable USB state after >= 100ms debounce
    powerSense.updateRaw( true, 300 );
    pm.update( 300 );
    assert( !buzzer.isSounding() ); // Not debounced yet
    powerSense.updateRaw( true, 405 );
    pm.update( 405 );
    assert( buzzer.isSounding() );
    assert( display.isToastActive( 405 ) );
    assert( netWorker.getPowerProfile() == PowerProfile::Usb );
    assert( pm.getLastActivityMs() == 405 );

    // 5. Localization test: Italian
    auto &i18n = I18n::instance();
    i18n.setLanguage( Language::Italian );

    powerSense.updateRaw( false, 500 );
    powerSense.updateRaw( false, 605 );
    pm.update( 605 );
    assert( display.isToastActive( 605 ) );

    // Switch back to USB in Italian
    powerSense.updateRaw( true, 700 );
    powerSense.updateRaw( true, 805 );
    pm.update( 805 );
    assert( display.isToastActive( 805 ) );

    // Reset language to English
    i18n.setLanguage( Language::English );
    std::cout << "PASS: test_power_transitions\n";
}

void test_sleep_decision_matrix()
{
    std::cout << "Running test_sleep_decision_matrix...\n";
    PowerManager pm;

    bool timerCanSleep = true;
    bool gagCanSleep = true;
    bool arbitratorCanSleep = true;

    pm.registerSleepGate( [ &timerCanSleep ]() { return timerCanSleep; } );
    pm.registerSleepGate( [ &gagCanSleep ]() { return gagCanSleep; } );
    pm.registerSleepGate( [ &arbitratorCanSleep ]() { return arbitratorCanSleep; } );

    pm.resetActivity( 1000 );
    assert( pm.getLastActivityMs() == 1000 );

    // Inactivity timeout of 30 seconds
    assert( !pm.shouldEnterSleep( 1000 ) );
    assert( !pm.shouldEnterSleep( 15000 ) );
    assert( !pm.shouldEnterSleep( 30999 ) );

    // When 30 seconds have elapsed and all controllers are idle -> true
    assert( pm.shouldEnterSleep( 31000 ) );
    assert( pm.canEnterSleep() );

    // Safety gate: timer active -> false
    timerCanSleep = false;
    assert( !pm.canEnterSleep() );
    assert( !pm.shouldEnterSleep( 31000 ) );
    timerCanSleep = true;
    assert( pm.canEnterSleep() );
    assert( pm.shouldEnterSleep( 31000 ) );

    // Safety gate: gag active -> false
    gagCanSleep = false;
    assert( !pm.canEnterSleep() );
    assert( !pm.shouldEnterSleep( 31000 ) );
    gagCanSleep = true;
    assert( pm.canEnterSleep() );
    assert( pm.shouldEnterSleep( 31000 ) );

    // Safety gate: arbitrator active -> false
    arbitratorCanSleep = false;
    assert( !pm.canEnterSleep() );
    assert( !pm.shouldEnterSleep( 31000 ) );
    arbitratorCanSleep = true;
    assert( pm.canEnterSleep() );
    assert( pm.shouldEnterSleep( 31000 ) );

    // Activity reset pushes timeout back by 30 seconds
    pm.resetActivity( 40000 );
    assert( !pm.shouldEnterSleep( 40000 ) );
    assert( !pm.shouldEnterSleep( 69999 ) );
    assert( pm.shouldEnterSleep( 70000 ) );

    // Clear gates and verify default is true
    pm.clearSleepGates();
    assert( pm.canEnterSleep() );
    assert( pm.shouldEnterSleep( 70000 ) );

    std::cout << "PASS: test_sleep_decision_matrix\n";
}

void test_30min_wifi_gate()
{
    std::cout << "Running test_30min_wifi_gate...\n";
    PowerManager pm;
    // Cold boot (lastSyncEpoch == 0) -> true
    assert( pm.shouldSyncWifiOnWake( 1000, 0 ) == true );
    // 15 mins elapsed (< 1800s) -> false
    assert( pm.shouldSyncWifiOnWake( 1900, 1000 ) == false );
    // 31 mins elapsed (>= 1800s) -> true
    assert( pm.shouldSyncWifiOnWake( 2860, 1000 ) == true );
    // Exactly 30 mins elapsed (== 1800s) -> true
    assert( pm.shouldSyncWifiOnWake( 2800, 1000 ) == true );
    // Clock anomaly / backwards jump -> true
    assert( pm.shouldSyncWifiOnWake( 500, 1000 ) == true );
    std::cout << "PASS: test_30min_wifi_gate\n";
}

void test_deep_sleep_entry()
{
    std::cout << "Running test_deep_sleep_entry...\n";
    PowerSense powerSense( PIN_USB_DETECT );
    DisplayManager display;
    display.setup();
    display.setPower( true );
    assert( display.isPoweredOn() );
    Buzzer buzzer( PIN_BUZZER );
    auto &netWorker = NetworkWorker::instance();

    PowerManager pm;
    pm.init( powerSense, display, buzzer, netWorker );

    MotionSensor motion;
    motion.begin();

    resetMockSleepState();
    WiFi.mode( WIFI_STA );
    assert( WiFi.getMode() == WIFI_STA );

    pm.enterDeepSleep( motion );

    // 1. OLED panel shut down
    assert( !display.isPoweredOn() );

    // 2. GPIO wakeups configured
    const uint64_t expectedLowMask = ( 1ULL << PIN_BUTTON_ACTION );
    const uint64_t expectedHighMask = ( 1ULL << PIN_MPU_INT ) | ( 1ULL << PIN_USB_DETECT );
    assert( ( g_mockSleepWakeupPinMaskLow & expectedLowMask ) == expectedLowMask );
    assert( ( g_mockSleepWakeupPinMaskHigh & expectedHighMask ) == expectedHighMask );

    // 3. WiFi radio turned off
    assert( WiFi.getMode() == WIFI_OFF );

    // 4. Deep sleep entered
    assert( g_mockDeepSleepStarted );

    std::cout << "PASS: test_deep_sleep_entry\n";
}

int main()
{
    test_power_transitions();
    test_sleep_decision_matrix();
    test_30min_wifi_gate();
    test_deep_sleep_entry();
    std::cout << "All power management tests passed!\n";
    return 0;
}
