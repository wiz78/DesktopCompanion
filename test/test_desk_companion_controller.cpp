// test/test_desk_companion_controller.cpp
#include "controllers/deskCompanionController.h"
#include "display.h"
#include "drivers/ldrSensor.h"
#include "drivers/motionSensor.h"
#include "drivers/proximitySensor.h"
#include "i18n.h"
#include "sentenceEngine.h"
#include "services/timeService.h"
#include "services/weatherService.h"

#include <cassert>
#include <iostream>

void testOfflineBoot()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( false ); // Not configured online

    assert( controller.getState() == CompanionState::SetupPortal );
    assert( display.isPoweredOn() );
    controller.onUserActionClick( 0 );
    controller.update( 0 );

    assert( controller.getState() == CompanionState::Offline );
    assert( display.getHeaderLeft() == "OFFLINE" );
    std::cout << "[PASS] testOfflineBoot\n";
}

static void setMockValidWeather( WeatherService &weatherService )
{
    WeatherData w;
    w.tempC = 21.0f;
    w.humidity = 50;
    w.condition = "Clear";
    w.valid = true;
    weatherService.updateWeather( w );
}

[[nodiscard]] static std::string getCurrentlyDisplayedText( const DisplayManager &display )
{
    if( display.isMarqueeActive() ) {
        return display.getMarqueeText();
    }
    if( !display.getBodyLines().empty() ) {
        return display.getBodyLines()[ 0 ];
    }
    return "";
}

void testOnlineWakeAndAphorismTimer()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true ); // Configured online

    // Starts awake with an aphorism
    assert( controller.getState() == CompanionState::AwakeAphorism );

    // At 14.9s, still in AwakeAphorism
    setMockMillis( 14900 );
    controller.update( 14900 );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    // At 15.1s, transitions to AwakeWeather
    setMockMillis( 15100 );
    controller.update( 15100 );
    assert( controller.getState() == CompanionState::AwakeWeather );
    std::cout << "[PASS] testOnlineWakeAndAphorismTimer\n";
}

void testInactivitySleepAndProximityWake()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // Progress to weather
    setMockMillis( 16000 );
    controller.update( 16000 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    // User departs desk
    proximity.feedRawDistance( 2000, 20000 );
    proximity.feedRawDistance( 2000, 22000 );

    // Inactivity timeout: 30s from last movement (time 16000 + 30000 = 46000)
    setMockMillis( 46100 );
    controller.update( 46100 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !display.isPoweredOn() );

    // Approach event wakes it up! (sustained arrival <= 1000mm for >= 1000ms)
    setMockMillis( 47000 );
    proximity.feedRawDistance( 800, 47000 ); // Target at 800mm
    controller.update( 47000 );
    assert( controller.getState() == CompanionState::Sleeping ); // Debouncing arrival

    setMockMillis( 48000 );
    proximity.feedRawDistance( 800, 48000 );
    controller.update( 48000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( display.isPoweredOn() );
    std::cout << "[PASS] testInactivitySleepAndProximityWake\n";
}

void testUserActionClickAdvancesAphorism()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // In AwakeWeather state, button press wakes/advances
    setMockMillis( 16000 );
    controller.update( 16000 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    const uint32_t clickTime = 20000;
    setMockMillis( clickTime );
    controller.onUserActionClick( clickTime );
    assert( display.isPoweredOn() );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    setMockMillis( clickTime + 1000 );
    controller.update( clickTime + 1000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    setMockMillis( clickTime + 15001 );
    controller.update( clickTime + 15001 );
    assert( controller.getState() == CompanionState::AwakeWeather );
    std::cout << "[PASS] testUserActionClickAdvancesAphorism\n";
}

void testLdrDimmingAndNightMode()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3, 1.0f, 800, 300 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // Bright light: not dimmed, not night
    ldr.updateRaw( 2000 );
    setMockMillis( 100 );
    controller.update( 100 );
    assert( !controller.isNightMode() );

    // Dim light: dimmed, but not night
    ldr.updateRaw( 500 );
    setMockMillis( 200 );
    controller.update( 200 );
    assert( !controller.isNightMode() );

    // Dark: night mode
    ldr.updateRaw( 100 );
    setMockMillis( 300 );
    controller.update( 300 );
    assert( controller.isNightMode() );
    std::cout << "[PASS] testLdrDimmingAndNightMode\n";
}

void testUserActionClickFromSleepKeepsAphorismDuration()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // Transition to AwakeWeather
    setMockMillis( 16000 );
    controller.update( 16000 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    // Inactivity sleep after 30s
    setMockMillis( 46100 );
    controller.update( 46100 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !display.isPoweredOn() );

    // Wake up from sleep via onUserActionClick at timestamp 50000
    setMockMillis( 50000 );
    controller.onUserActionClick( 50000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( display.isPoweredOn() );

    // At timestamp 55000 (5s into aphorism), should remain in AwakeAphorism
    setMockMillis( 55000 );
    controller.update( 55000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    // At timestamp 65001 (15.001s into aphorism), should transition to AwakeWeather
    setMockMillis( 65001 );
    controller.update( 65001 );
    assert( controller.getState() == CompanionState::AwakeWeather );
    std::cout << "[PASS] testUserActionClickFromSleepKeepsAphorismDuration\n";
}

void testOfflineAphorismRotation()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( false ); // Not configured online
    controller.onUserActionClick( 0 );

    assert( controller.getState() == CompanionState::Offline );
    const std::string s1 = getCurrentlyDisplayedText( display );

    // At 14s, still in Offline state with initial aphorism
    setMockMillis( 14000 );
    controller.update( 14000 );
    assert( controller.getState() == CompanionState::Offline );
    const std::string s1_still = getCurrentlyDisplayedText( display );
    assert( s1 == s1_still );

    // Keep active with proximity activity before 30s inactivity sleep (fast close wave <= 200mm)
    proximity.feedRawDistance( 150, 14500 );
    proximity.feedRawDistance( 150, 14560 );

    // At 15.1s, aphorism rotates
    setMockMillis( 15100 );
    controller.update( 15100 );
    assert( controller.getState() == CompanionState::Offline );
    const std::string s2 = getCurrentlyDisplayedText( display );
    assert( s1 != s2 );

    // Keep active with proximity activity
    setMockMillis( 25000 );
    proximity.feedRawDistance( 800, 25000 );
    controller.update( 25000 );
    assert( controller.getState() == CompanionState::Offline );

    // At 30.2s (15.1s after rotation at 15100), aphorism rotates again
    setMockMillis( 30200 );
    controller.update( 30200 );
    assert( controller.getState() == CompanionState::Offline );
    const std::string s3 = getCurrentlyDisplayedText( display );
    assert( s2 != s3 );

    std::cout << "[PASS] testOfflineAphorismRotation\n";
}

void testIsAboutToSleep()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // In AwakeAphorism state, not about to sleep
    setMockMillis( 5000 );
    controller.update( 5000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( !controller.isAboutToSleep() );

    // Transition to AwakeWeather at 15100 (lastActivityTime is 15100)
    setMockMillis( 15100 );
    controller.update( 15100 );
    assert( controller.getState() == CompanionState::AwakeWeather );
    assert( !controller.isAboutToSleep() );

    // At 15100 + 29499 = 44599ms: elapsed inactivity = 29499ms < 29500ms -> isAboutToSleep == false
    setMockMillis( 44599 );
    controller.update( 44599 );
    assert( !controller.isAboutToSleep() );
    assert( !controller.isAboutToSleep( 44599 ) );

    // At 15100 + 29500 = 44600ms: elapsed inactivity = 29500ms >= 29500ms -> isAboutToSleep == true
    setMockMillis( 44600 );
    controller.update( 44600 );
    assert( controller.isAboutToSleep() );
    assert( controller.isAboutToSleep( 44600 ) );

    // At 15100 + 29999 = 45099ms -> isAboutToSleep == true
    setMockMillis( 45099 );
    controller.update( 45099 );
    assert( controller.isAboutToSleep() );
    assert( controller.isAboutToSleep( 45099 ) );

    // At 15100 + 30000 = 45100ms -> transitions to Sleeping -> isAboutToSleep == false
    setMockMillis( 45100 );
    controller.update( 45100 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !controller.isAboutToSleep() );
    assert( !controller.isAboutToSleep( 45100 ) );

    // Test offline mode isAboutToSleep
    setMockMillis( 0 );
    DeskCompanionController offlineController( display, timeService, weatherService, sentenceEngine, proximity, ldr,
                                               motion );
    offlineController.init( false );
    offlineController.onUserActionClick( 0 );
    assert( offlineController.getState() == CompanionState::Offline );

    // Initial lastActivityTime is 0
    setMockMillis( 29499 );
    offlineController.update( 29499 );
    assert( !offlineController.isAboutToSleep() );
    assert( !offlineController.isAboutToSleep( 29499 ) );

    setMockMillis( 29500 );
    offlineController.update( 29500 );
    assert( offlineController.isAboutToSleep() );
    assert( offlineController.isAboutToSleep( 29500 ) );

    setMockMillis( 30000 );
    offlineController.update( 30000 );
    assert( offlineController.getState() == CompanionState::Sleeping );
    assert( !offlineController.isAboutToSleep() );
    assert( !offlineController.isAboutToSleep( 30000 ) );

    std::cout << "[PASS] testIsAboutToSleep\n";
}

void testOfflineWakeRetainsAphorism()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( false ); // Not configured online
    controller.onUserActionClick( 0 );

    assert( controller.getState() == CompanionState::Offline );

    // User departs desk
    proximity.feedRawDistance( 2000, 20000 );
    proximity.feedRawDistance( 2000, 22000 );

    // Inactivity sleep after 30s
    setMockMillis( 30000 );
    controller.update( 30000 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !display.isPoweredOn() );

    // Wake up via proximity at timestamp 50000 (sustained arrival across >= 1000ms)
    setMockMillis( 49000 );
    proximity.feedRawDistance( 800, 49000 );
    controller.update( 49000 );
    assert( controller.getState() == CompanionState::Sleeping ); // Debouncing arrival

    setMockMillis( 50000 );
    proximity.feedRawDistance( 800, 50000 );
    controller.update( 50000 );
    assert( controller.getState() == CompanionState::Offline );
    assert( display.isPoweredOn() );
    const std::string wakeSentence = getCurrentlyDisplayedText( display );
    assert( !wakeSentence.empty() );

    // Subsequent tick 1ms later must retain the aphorism (not immediately skipped)
    setMockMillis( 50001 );
    controller.update( 50001 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == wakeSentence );

    // At 14s after wake (64000ms), aphorism is still retained
    setMockMillis( 64000 );
    controller.update( 64000 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == wakeSentence );

    // Keep active with proximity activity before 30s inactivity sleep
    setMockMillis( 64500 );
    proximity.feedRawDistance( 800, 64500 );

    // At 15.001s after wake (65001ms), aphorism rotates
    setMockMillis( 65001 );
    controller.update( 65001 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) != wakeSentence );

    // Now test waking from sleep via onUserActionClick
    setMockMillis( 95000 );
    controller.update( 95000 );
    assert( controller.getState() == CompanionState::Sleeping );

    setMockMillis( 100000 );
    controller.onUserActionClick( 100000 );
    assert( controller.getState() == CompanionState::Offline );
    const std::string clickWakeSentence = getCurrentlyDisplayedText( display );
    assert( !clickWakeSentence.empty() );

    // Subsequent tick 1ms later must retain the aphorism
    setMockMillis( 100001 );
    controller.update( 100001 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == clickWakeSentence );

    std::cout << "[PASS] testOfflineWakeRetainsAphorism\n";
}

void testInvalidWeatherGoesToSleep()
{
    setMockMillis( 0 );
    DisplayManager display;
    display.setup();
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // Progress to weather screen at 16000ms - invalid weather goes to sleep
    setMockMillis( 16000 );
    controller.update( 16000 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !display.isPoweredOn() );
    std::cout << "[PASS] testInvalidWeatherGoesToSleep\n";
}

void testSentenceSingleClickRewind()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    assert( controller.getState() == CompanionState::AwakeAphorism );
    const std::string initialSentence = getCurrentlyDisplayedText( display );
    assert( !initialSentence.empty() );

    // Let 5000ms elapse while displaying/scrolling sentence
    setMockMillis( 5000 );
    controller.update( 5000 );
    display.update( 5000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    // Single click at 5000ms rewinds the current sentence
    controller.onUserActionClick( 5000 );
    controller.update( 5000 );

    // State returns to / remains in AwakeAphorism and displayed text is rewound (not replaced)
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( getCurrentlyDisplayedText( display ) == initialSentence );

    // Sentence reading timer is restored to full duration from 5000ms
    // At 5000 + 14900 = 19900ms, still in AwakeAphorism
    setMockMillis( 19900 );
    controller.update( 19900 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( getCurrentlyDisplayedText( display ) == initialSentence );

    // At 5000 + 15100 = 20100ms, transitions to AwakeWeather
    setMockMillis( 20100 );
    controller.update( 20100 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    std::cout << "[PASS] testSentenceSingleClickRewind\n";
}

void testSentenceDoubleClickAdvance()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    assert( controller.getState() == CompanionState::AwakeAphorism );
    const std::string s1 = getCurrentlyDisplayedText( display );
    assert( !s1.empty() );

    // Advance to 5000ms
    setMockMillis( 5000 );
    controller.update( 5000 );

    // First click at 5000ms rewinds current sentence
    controller.onUserActionClick( 5000 );
    controller.update( 5000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( getCurrentlyDisplayedText( display ) == s1 );

    // Second click at 5200ms (within 400ms window) advances to next sentence
    setMockMillis( 5200 );
    controller.onUserActionClick( 5200 );
    controller.update( 5200 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    const std::string s2 = getCurrentlyDisplayedText( display );
    assert( !s2.empty() );
    assert( s2 != s1 );

    // Timer reset from 5200ms
    setMockMillis( 5200 + 14900 );
    controller.update( 5200 + 14900 );
    assert( controller.getState() == CompanionState::AwakeAphorism );

    setMockMillis( 5200 + 15100 );
    controller.update( 5200 + 15100 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    std::cout << "[PASS] testSentenceDoubleClickAdvance\n";
}

void testNonSentenceSingleClickImmediatelyAdvances()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    setMockValidWeather( weatherService );
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();
    timeService.markSynced( true );

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( true );

    // Initial boot aphorism
    assert( controller.getState() == CompanionState::AwakeAphorism );
    const std::string bootSentence = getCurrentlyDisplayedText( display );
    assert( !bootSentence.empty() );

    // Transition to AwakeWeather at 16000ms
    setMockMillis( 16000 );
    controller.update( 16000 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    // Single click while in AwakeWeather at 20000ms immediately advances to a new sentence
    setMockMillis( 20000 );
    controller.onUserActionClick( 20000 );
    controller.update( 20000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( display.isPoweredOn() );
    const std::string weatherWakeSentence = getCurrentlyDisplayedText( display );
    assert( !weatherWakeSentence.empty() );
    assert( weatherWakeSentence != bootSentence );

    // Transition to AwakeWeather and then sleep
    setMockMillis( 36000 );
    controller.update( 36000 );
    assert( controller.getState() == CompanionState::AwakeWeather );

    // User departs desk, sleep after 30s inactivity
    proximity.feedRawDistance( 2000, 40000 );
    setMockMillis( 66100 );
    controller.update( 66100 );
    assert( controller.getState() == CompanionState::Sleeping );
    assert( !display.isPoweredOn() );

    // Single click while Sleeping at 70000ms immediately wakes and advances to a new sentence
    setMockMillis( 70000 );
    controller.onUserActionClick( 70000 );
    controller.update( 70000 );
    assert( controller.getState() == CompanionState::AwakeAphorism );
    assert( display.isPoweredOn() );
    const std::string sleepWakeSentence = getCurrentlyDisplayedText( display );
    assert( !sleepWakeSentence.empty() );
    assert( sleepWakeSentence != weatherWakeSentence );

    std::cout << "[PASS] testNonSentenceSingleClickImmediatelyAdvances\n";
}

void testOfflineSentenceSingleClickRewindAndDoubleClickAdvance()
{
    setMockMillis( 0 );
    DisplayManager display;
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    ProximitySensor proximity;
    LdrSensor ldr( 3 );
    MotionSensor motion;

    sentenceEngine.begin();

    DeskCompanionController controller( display, timeService, weatherService, sentenceEngine, proximity, ldr, motion );
    controller.init( false ); // Not configured online

    // Dismiss setup portal into Offline state
    assert( controller.getState() == CompanionState::SetupPortal );
    controller.onUserActionClick( 0 );
    controller.update( 0 );
    assert( controller.getState() == CompanionState::Offline );

    const std::string s1 = getCurrentlyDisplayedText( display );
    assert( !s1.empty() );

    // Let 5000ms elapse while sentence is displaying
    setMockMillis( 5000 );
    controller.update( 5000 );
    assert( controller.getState() == CompanionState::Offline );

    // Single click at 5000ms rewinds s1 and stays in Offline
    controller.onUserActionClick( 5000 );
    controller.update( 5000 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == s1 );

    // At 5000 + 14900 = 19900ms, s1 must still be displaying because timer was reset
    setMockMillis( 19900 );
    controller.update( 19900 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == s1 );

    // Double-click at 20000ms and 20250ms (within 400ms) advances to next sentence
    setMockMillis( 20000 );
    controller.onUserActionClick( 20000 );
    controller.update( 20000 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == s1 ); // First click rewinds

    setMockMillis( 20250 );
    controller.onUserActionClick( 20250 );
    controller.update( 20250 );
    assert( controller.getState() == CompanionState::Offline );
    const std::string s2 = getCurrentlyDisplayedText( display );
    assert( !s2.empty() );
    assert( s2 != s1 ); // Second click within 400ms advances to next sentence

    // Two clicks separated by 500ms (> 400ms window) are treated as two single-clicks (both rewind)
    setMockMillis( 25000 );
    controller.onUserActionClick( 25000 );
    controller.update( 25000 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == s2 );

    setMockMillis( 25500 );
    controller.onUserActionClick( 25500 );
    controller.update( 25500 );
    assert( controller.getState() == CompanionState::Offline );
    assert( getCurrentlyDisplayedText( display ) == s2 );

    std::cout << "[PASS] testOfflineSentenceSingleClickRewindAndDoubleClickAdvance\n";
}

int main()
{
    std::cout << "Testing DeskCompanionController renaming and sleep hook...\n";
    testOfflineBoot();
    testOnlineWakeAndAphorismTimer();
    testInactivitySleepAndProximityWake();
    testUserActionClickAdvancesAphorism();
    testLdrDimmingAndNightMode();
    testUserActionClickFromSleepKeepsAphorismDuration();
    testOfflineAphorismRotation();
    testIsAboutToSleep();
    testOfflineWakeRetainsAphorism();
    testInvalidWeatherGoesToSleep();
    testSentenceSingleClickRewind();
    testSentenceDoubleClickAdvance();
    testNonSentenceSingleClickImmediatelyAdvances();
    testOfflineSentenceSingleClickRewindAndDoubleClickAdvance();
    std::cout << "DeskCompanionController tests passed!\n";
    return 0;
}
