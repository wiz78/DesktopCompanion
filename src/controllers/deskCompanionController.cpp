//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/deskCompanionController.h"
#include "i18n.h"

#include "services/configServer.h"
#include "services/networkWorker.h"

#include <cmath>
#include <cstdio>

DeskCompanionController::DeskCompanionController( DisplayManager &display, TimeService &timeService,
                                                  WeatherService &weatherService, SentenceEngine &sentenceEngine,
                                                  ProximitySensor &proximity, LdrSensor &ldr, MotionSensor &motion ) :
    StateMachine( CompanionState::SetupPortal, {
                      { CompanionState::SetupPortal, &DeskCompanionController::setupPortalHandler },
                      { CompanionState::Offline, &DeskCompanionController::offlineHandler },
                      { CompanionState::Sleeping, &DeskCompanionController::sleepingHandler },
                      { CompanionState::AwakeAphorism, &DeskCompanionController::awakeAphorismHandler },
                      { CompanionState::AwakeWeather, &DeskCompanionController::awakeWeatherHandler },
                      { CompanionState::AwakeWeatherCondition, &DeskCompanionController::awakeWeatherConditionHandler },
                      { CompanionState::RewindAphorism, &DeskCompanionController::rewindAphorismHandler }, } ),
    display( display ), timeService( timeService ), weatherService( weatherService ), sentenceEngine( sentenceEngine ),
    proximity( proximity ), ldr( ldr ), motion( motion )
{
}

void DeskCompanionController::init( const bool configuredOnline )
{
    isOnlineConfigured = configuredOnline;
    lastActivityTime = 0;
    lastAphorismTime = 0;
    lastDisplayUpdateTime = 0;
    lastSentenceClickTime = 0;

    display.setPower( true );
    if( configuredOnline ) {
        setState( CompanionState::AwakeAphorism );
        triggerNewAphorism( 0 );
    } else {
        setState( CompanionState::SetupPortal );
    }
}

void DeskCompanionController::setConfiguredOnline( const bool online )
{
    isOnlineConfigured = online;

    if( !isOnlineConfigured ) {
        display.setPower( true );
        setState( CompanionState::SetupPortal );
    } else if( getState() == CompanionState::SetupPortal || getState() == CompanionState::Offline ) {
        display.setPower( true );
        setState( CompanionState::AwakeAphorism );
        triggerNewAphorism( lastActivityTime );
    }
}

void DeskCompanionController::triggerNewAphorism( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();

    lastAphorismTime = currentMs;

    updateHeader( currentMs );

    if( sentenceEngine.getNextSentence( currentSentence ) ) {
        display.displaySentence( currentSentence, TextFormatter::MAX_WRAPPED_CHARS, currentMs );
        currentAphorismDurationMs = std::max( APHORISM_DURATION_MS, display.getSentenceDurationMs() );
    } else {
        currentSentence.clear();
        currentAphorismDurationMs = APHORISM_DURATION_MS;
    }
}

void DeskCompanionController::updateHeader( [[maybe_unused]] const uint32_t nowMs )
{
    const int roomTemp = static_cast<int>(std::round( motion.getTemperatureC() ));
    char tempBuf[ 16 ];
    std::snprintf( tempBuf, sizeof( tempBuf ), "%d C", roomTemp );

    std::string offlineStr = "OFFLINE";
    if( ConfigServer::instance().isApActive() ) {
        offlineStr = "[AP] OFFL";
    }

    if( !isOnlineConfigured || !timeService.isTimeSynced() ) {
        display.setHeader( offlineStr, tempBuf );
        return;
    }

    tm timeInfo{};
    if( timeService.getLocalTimeInfo( timeInfo ) ) {
        std::string timeStr = TimeService::formatTime( timeInfo );
        if( ConfigServer::instance().isApActive() ) {
            timeStr = "[AP] " + timeStr;
        }
        const std::string dateStr = TimeService::formatDate( timeInfo, I18n::instance().getLanguage() );
        display.setHeader( timeStr, dateStr );
    } else {
        display.setHeader( "--:--", tempBuf );
    }
}

void DeskCompanionController::updateWeatherDisplay()
{
    static constexpr char DEGREES = 0xF7;

    updateHeader( lastKnownNowMs );
    display.clearBody();

    if( const WeatherData w = weatherService.getLatestWeather(); w.valid ) {
        const char *extPrefix = I18n::instance().get( StringId::TempExternalPrefix );
        const char *humPrefix = I18n::instance().get( StringId::HumidityPrefix );
        char line1[ 32 ];
        char line2[ 32 ];

        std::snprintf( line1, sizeof( line1 ), "%s%d%cC", extPrefix, static_cast<int>(std::round( w.tempC )), DEGREES );
        std::snprintf( line2, sizeof( line2 ), "%s%d%%", humPrefix, w.humidity );

        display.setBodyText( { line1, line2 }, 2 );

    } else
        goToSleep();
}

void DeskCompanionController::onUserActionClick( const uint32_t nowMs )
{
    const uint32_t clickTime = ( nowMs != 0 ) ? nowMs : ( lastKnownNowMs ? lastKnownNowMs : millis() );

    lastActivityTime = clickTime;
    lastKnownNowMs = clickTime;

    if( getState() == CompanionState::SetupPortal ) {
        lastSentenceClickTime = 0;
        display.setPower( true );
        setState( CompanionState::Offline );
        triggerNewAphorism( clickTime );
    } else if( getState() == CompanionState::Sleeping || getState() == CompanionState::AwakeWeather ) {
        lastSentenceClickTime = 0;
        display.setPower( true );
        if( isOnlineConfigured ) {
            setState( CompanionState::AwakeAphorism );
        } else {
            setState( CompanionState::Offline );
        }
        triggerNewAphorism( clickTime );
    } else if( getState() == CompanionState::AwakeAphorism || getState() == CompanionState::Offline ) {
        if( lastSentenceClickTime != 0 && ( clickTime - lastSentenceClickTime ) <= DOUBLE_CLICK_WINDOW_MS ) {
            lastSentenceClickTime = 0;
            if( getState() == CompanionState::AwakeAphorism ) {
                resetStateTimer();
            }
            triggerNewAphorism( clickTime );
        } else {
            lastSentenceClickTime = clickTime;
            setState( CompanionState::RewindAphorism );
            rewindAphorismHandler();
        }
    }
}

bool DeskCompanionController::isAboutToSleep( const uint32_t nowMs ) const
{
    if( getState() == CompanionState::AwakeWeather || (
            getState() == CompanionState::Offline && !isOnlineConfigured ) ) {
        const uint32_t effectiveNowMs = ( nowMs == 0 ) ? lastKnownNowMs : nowMs;
        if( effectiveNowMs >= lastActivityTime ) {
            const uint32_t sleepTimeout = ( getState() == CompanionState::Offline )
                                              ? std::max( INACTIVITY_SLEEP_MS, currentAphorismDurationMs )
                                              : INACTIVITY_SLEEP_MS;
            return ( effectiveNowMs - lastActivityTime ) >= ( sleepTimeout - 500 );
        }
    }
    return false;
}

void DeskCompanionController::update( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();

    lastKnownNowMs = currentMs;

    // Ambient light auto-dimming & night gag mute
    if( currentMs - lastDisplayBrightnessTime > 3000 ) {
        display.setBrightnessLevel( ldr.getIdealDisplayBrightnessLevel() );
        lastDisplayBrightnessTime = currentMs;
    }

    nightMode = ldr.isNight();

    // Proximity wake detection
    if( proximity.shouldWakeUp() ) {
        lastActivityTime = currentMs;
        if( getState() == CompanionState::Sleeping || getState() == CompanionState::AwakeWeather ) {
            display.setPower( true );
            if( isOnlineConfigured ) {
                setState( CompanionState::AwakeAphorism );
            } else {
                setState( CompanionState::Offline );
            }
            triggerNewAphorism( currentMs );
            return;
        }
    }

    runStateMachine();
}

void DeskCompanionController::sleepingHandler()
{
    // Waiting for wakeTrigger or action button
}

void DeskCompanionController::awakeAphorismHandler()
{
    if( ( lastKnownNowMs - lastDisplayUpdateTime ) >= 1000 ) {
        updateHeader( lastKnownNowMs );
        lastDisplayUpdateTime = lastKnownNowMs;
    }

    if( stateElapsed( currentAphorismDurationMs ) ) {
        setState( CompanionState::AwakeWeather );
        lastActivityTime = lastKnownNowMs;
        display.setPower( true );
        updateWeatherDisplay();
    }
}

void DeskCompanionController::awakeWeatherHandler()
{
    if( stateElapsed( 2000 )) {
        weatherDurationMs = 2000;
        setState( CompanionState::AwakeWeatherCondition );
    } else if( ( lastKnownNowMs - lastDisplayUpdateTime ) >= 1000 ) {
        updateWeatherDisplay();
        lastDisplayUpdateTime = lastKnownNowMs;
    }

    checkWeatherInactivity();
}

void DeskCompanionController::awakeWeatherConditionHandler()
{
    if( stateElapsed( weatherDurationMs ))
        setState( CompanionState::AwakeWeather );
    else if( ( lastKnownNowMs - lastDisplayUpdateTime ) >= 1000 ) {

        lastDisplayUpdateTime = lastKnownNowMs;

        updateHeader( lastKnownNowMs );
        display.clearBody();

        if( const WeatherData w = weatherService.getLatestWeather(); w.valid ) {

            weatherDurationMs = 2000;

            if( !display.displaySentence( w.condition, 30, lastKnownNowMs, 300, 0 ))
                weatherDurationMs += display.getSentenceDurationMs();

        } else
            goToSleep();
    }

    checkWeatherInactivity();
}

void DeskCompanionController::rewindAphorismHandler()
{
    const uint32_t currentMs = ( lastKnownNowMs != 0 ) ? lastKnownNowMs : millis();
    lastAphorismTime = currentMs;

    updateHeader( currentMs );

    if( !currentSentence.empty() ) {
        display.displaySentence( currentSentence, TextFormatter::MAX_WRAPPED_CHARS, currentMs );
        currentAphorismDurationMs = std::max( APHORISM_DURATION_MS, display.getSentenceDurationMs() );
    }

    if( isOnlineConfigured ) {
        setState( CompanionState::AwakeAphorism );
    } else {
        setState( CompanionState::Offline );
    }
}

void DeskCompanionController::checkWeatherInactivity()
{
    if( ( lastKnownNowMs - lastActivityTime ) >= INACTIVITY_SLEEP_MS )
        goToSleep();
}

void DeskCompanionController::goToSleep()
{
    setState( CompanionState::Sleeping );
    display.setPower( false );
}

void DeskCompanionController::setupPortalHandler()
{
    if( ( lastKnownNowMs - lastDisplayUpdateTime ) >= 1000 ) {
        display.displaySetupScreen( ConfigServer::instance().getApSsid().c_str(),
                                    ConfigServer::instance().getApIp().c_str(),
                                    NetworkWorker::instance().getHostname().c_str(), lastKnownNowMs );
        lastDisplayUpdateTime = lastKnownNowMs;
    }
}

void DeskCompanionController::offlineHandler()
{
    if( ( lastKnownNowMs - lastDisplayUpdateTime ) >= 1000 ) {
        updateHeader( lastKnownNowMs );
        lastDisplayUpdateTime = lastKnownNowMs;
    }

    if( ( lastKnownNowMs - lastAphorismTime ) >= currentAphorismDurationMs ) {
        triggerNewAphorism( lastKnownNowMs );
        lastAphorismTime = lastKnownNowMs;
    }

    const uint32_t sleepTimeout = std::max( INACTIVITY_SLEEP_MS, currentAphorismDurationMs );
    if( ( lastKnownNowMs - lastActivityTime ) >= sleepTimeout ) {
        setState( CompanionState::Sleeping );
        display.setPower( false );
    }
}
