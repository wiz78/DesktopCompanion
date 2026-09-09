//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_DESKCOMPANIONCONTROLLER_H
#define FRIENDLYBOT_CONTROLLERS_DESKCOMPANIONCONTROLLER_H

#include "display.h"
#include "drivers/ldrSensor.h"
#include "drivers/motionSensor.h"
#include "drivers/proximitySensor.h"
#include "sentenceEngine.h"
#include "services/timeService.h"
#include "services/weatherService.h"
#include "stateMachine.h"

#include <cstdint>
#include <string>

enum class CompanionState : uint8_t
{
    SetupPortal = 0,
    Offline,
    Sleeping,
    AwakeAphorism,
    AwakeWeather,
    AwakeWeatherCondition,
    RewindAphorism
};

class DeskCompanionController : public StateMachine<DeskCompanionController, CompanionState>
{
public:
    using StateMachine::getState;

    static constexpr uint32_t APHORISM_DURATION_MS = 15000;
    static constexpr uint32_t INACTIVITY_SLEEP_MS = 30000;
    static constexpr uint32_t DOUBLE_CLICK_WINDOW_MS = 400;

    DeskCompanionController(
        DisplayManager& display,
        TimeService& timeService,
        WeatherService& weatherService,
        SentenceEngine& sentenceEngine,
        ProximitySensor& proximity,
        LdrSensor& ldr,
        MotionSensor& motion
    );

    void init( bool configuredOnline );
    void update( uint32_t nowMs = 0 );
    void onUserActionClick( uint32_t nowMs = 0 );
    void setConfiguredOnline( bool online );

    [[nodiscard]] bool isNightMode() const
    {
        return nightMode;
    }
    [[nodiscard]] bool canEnterSleep() const
    {
        return getState() == CompanionState::Sleeping;
    }
    [[nodiscard]] bool isAboutToSleep( uint32_t nowMs = 0 ) const;

private:
    DisplayManager& display;
    TimeService& timeService;
    WeatherService& weatherService;
    SentenceEngine& sentenceEngine;
    ProximitySensor& proximity;
    LdrSensor& ldr;
    MotionSensor& motion;

    bool isOnlineConfigured = false;
    bool nightMode = false;

    uint32_t lastKnownNowMs = 0;
    uint32_t lastDisplayBrightnessTime = 0;
    uint32_t lastActivityTime = 0;
    uint32_t lastDisplayUpdateTime = 0;
    uint32_t lastAphorismTime = 0;
    uint32_t lastSentenceClickTime = 0;
    uint32_t currentAphorismDurationMs = APHORISM_DURATION_MS;
    uint32_t weatherDurationMs = 0;

    std::string currentSentence;

    void setupPortalHandler();
    void offlineHandler();
    void sleepingHandler();
    void awakeAphorismHandler();
    void awakeWeatherHandler();
    void awakeWeatherConditionHandler();
    void rewindAphorismHandler();

    void checkWeatherInactivity();
    void goToSleep();

    void updateHeader( uint32_t nowMs = 0 );
    void updateWeatherDisplay();

    void triggerNewAphorism( uint32_t nowMs = 0 );
};

#endif // FRIENDLYBOT_CONTROLLERS_DESKCOMPANIONCONTROLLER_H
