//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "buildInfo.h"
#include "controllers/arbitratorController.h"
#include "controllers/deskCompanionController.h"
#include "controllers/gagController.h"
#include "controllers/systemOrchestrator.h"
#include "controllers/timerController.h"
#include "hardwareManager.h"
#include "i18n.h"
#include "sentenceEngine.h"
#include "services/networkWorker.h"
#include "services/powerManager.h"
#include "services/serialCli.h"
#include "services/timeService.h"
#include "services/weatherService.h"

#include <Arduino.h>
#if __has_include( <LittleFS.h> )
#include <LittleFS.h>
#endif
#if defined(ESP32) || defined(ARDUINO)
#include <esp_sleep.h>
#endif
#include <ctime>
#include <memory>

#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

// Define RTC variables
RTC_DATA_ATTR static time_t rtcLastWifiAttemptEpoch = 0;
RTC_DATA_ATTR static uint32_t rtcWakeCount = 0;

namespace
{
    SentenceEngine sentenceEngine;
    TimeService timeService;
    WeatherService weatherService;
    std::unique_ptr<DeskCompanionController> deskCompanionController;
    std::unique_ptr<GagController> gagController;
    std::unique_ptr<ArbitratorController> arbitratorController;
    std::unique_ptr<TimerController> timerController;
    std::unique_ptr<SystemOrchestrator> orchestrator;
    SerialCli serialCli;
} // namespace

void setup()
{
    Serial.begin( 115200 );
    delay( 1000 );

    Serial.println( "========================================" );
    Serial.println( "Two-Eyed Bot Firmware Initializing..." );
    Serial.printf( "Build Timestamp: %s\n", BUILD_TIMESTAMP );
    Serial.println( "========================================" );

#if defined(ESP32) || defined(ARDUINO)
    const esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause(); rtcWakeCount++; Serial.printf(
        "[POWER] Wakeup cause: %d, RTC wake count: %u\n", static_cast<int>(wakeCause), rtcWakeCount );
#else
    rtcWakeCount++;
#endif

#if __has_include( <LittleFS.h> )
    LittleFS.begin( true );
#endif

    I18n::instance().init();
    sentenceEngine.begin();

    auto& hal = HardwareManager::instance();
    hal.begin();

    deskCompanionController = std::make_unique<DeskCompanionController>(
        hal.getDisplay(), timeService, weatherService, sentenceEngine, hal.getProximity(), hal.getLdr(),
        hal.getMotion() );

    gagController = std::make_unique<GagController>( hal.getDisplay(), hal.getBuzzer(), hal.getMotion(), hal.getLdr(),
                                                     hal.getPowerSense() );
    gagController->init();

    arbitratorController = std::make_unique<ArbitratorController>( hal.getDisplay(), hal.getRightPot(),
                                                                   hal.getMotion() );
    arbitratorController->init();

    timerController = std::make_unique<TimerController>( hal.getDisplay(), hal.getLeftPot(), hal.getBuzzer(),
                                                         hal.getMotion(), hal.getProximity() );
    timerController->init();

    const auto initialProfile = hal.getPowerSense().isUsbPowered() ? PowerProfile::Usb : PowerProfile::Battery;
    NetworkWorker::instance().setPowerProfile( initialProfile );
    NetworkWorker::instance().begin( &timeService, &weatherService, &sentenceEngine );
    timerController->setVisualMode( static_cast<TimerVisualMode>(NetworkWorker::instance().getTimerMode()) );
    deskCompanionController->init( NetworkWorker::instance().isConfigured() );

    const bool enterDiag = ( digitalRead( hal.getActionButton().getPin() ) == LOW );
    orchestrator = std::make_unique<SystemOrchestrator>( hal, *deskCompanionController, *gagController,
                                                         *arbitratorController, *timerController, sentenceEngine );
    orchestrator->init( enterDiag );

    auto& power = PowerManager::instance();
    power.init( hal.getPowerSense(), hal.getDisplay(), hal.getBuzzer(), NetworkWorker::instance() );

    power.registerSleepGate( []() {
        return orchestrator ? orchestrator->canEnterSleep() : true;
    } );
    power.registerSleepGate( []() {
        return !HardwareManager::instance().getPowerSense().isUsbPowered();
    } );

    serialCli.init( orchestrator.get(), &hal, &NetworkWorker::instance(), &timeService, &weatherService,
                    &sentenceEngine, &power, deskCompanionController.get(), gagController.get(),
                    arbitratorController.get(), timerController.get() );
    serialCli.setRtcStats( &rtcWakeCount, &rtcLastWifiAttemptEpoch );

    if( !hal.getPowerSense().isUsbPowered() )
        if( const auto nowEpoch = time( nullptr ); power.shouldSyncWifiOnWake( nowEpoch, rtcLastWifiAttemptEpoch ) ) {
            Serial.println( "[POWER] Battery wake: triggering background WiFi sync..." );
            rtcLastWifiAttemptEpoch = nowEpoch;
            NetworkWorker::instance().triggerBatterySync();
        }

    power.resetActivity( millis() );

    hal.getBuzzer().doubleBeep();
}

void loop()
{
    if( timeService.isTimeSynced() && ( time( nullptr ) > 1700000000 ) && ( rtcLastWifiAttemptEpoch < 1700000000 ) )
        time( &rtcLastWifiAttemptEpoch );

    auto& hal = HardwareManager::instance();
    hal.update();

    const uint32_t nowMs = millis();
    auto& power = PowerManager::instance();
    power.update( nowMs );

    if( orchestrator ) {
        orchestrator->update( nowMs );
    }
    serialCli.update( nowMs );

    if( power.shouldEnterSleep( nowMs ) ) {
        Serial.println( "[POWER] Entering deep sleep..." );
        power.enterDeepSleep( hal.getMotion() );
    }
}
