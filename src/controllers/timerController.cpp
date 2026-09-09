//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/timerController.h"

#include <Arduino.h>

TimerController::TimerController( DisplayManager& display, Potentiometer& leftPot, Buzzer& buzzer, MotionSensor& motion,
                                  ProximitySensor& proximity ) : StateMachine( TimerState::Idle, {
                                                                                   {
                                                                                       TimerState::Idle,
                                                                                       &TimerController::idleHandler
                                                                                   },
                                                                                   {
                                                                                       TimerState::Setting,
                                                                                       &TimerController::settingHandler
                                                                                   },
                                                                                   {
                                                                                       TimerState::Running,
                                                                                       &TimerController::runningHandler
                                                                                   },
                                                                                   {
                                                                                       TimerState::Paused,
                                                                                       &TimerController::pausedHandler
                                                                                   },
                                                                                   {
                                                                                       TimerState::Alarming,
                                                                                       &TimerController::alarmingHandler
                                                                                   },
                                                                               } ), display( display ),
                                                                 leftPot( leftPot ), buzzer( buzzer ), motion( motion ),
                                                                 proximity( proximity ),
                                                                 digitsBarRenderer(
                                                                     std::make_unique<DigitsBarRenderer>() ),
                                                                 digitalSandRenderer(
                                                                     std::make_unique<DigitalSandRenderer>() )
{
}

void TimerController::init()
{
    selectedPresetIndex = TimerPresets::getStepIndexForNormalized( leftPot.getNormalized() );
    targetSeconds = TimerPresets::getDurationSeconds( selectedPresetIndex );
    remainingSeconds = targetSeconds;
}

ITimerRenderer& TimerController::getActiveRenderer()
{
    if( visualMode == TimerVisualMode::DigitalSand && digitalSandRenderer ) {
        return *digitalSandRenderer;
    }
    return *digitsBarRenderer;
}

void TimerController::setVisualMode( const TimerVisualMode mode )
{
    visualMode = mode;
}

void TimerController::resetInteractionTimer( const uint32_t nowMs )
{
    lastInteractionMs = ( nowMs == 0 ) ? millis() : nowMs;
    if( dimmed ) {
        dimmed = false;
        display.setBrightnessLevel( 3 );
    }
    if( !display.isPoweredOn() ) {
        display.setPower( true );
    }
}

void TimerController::wakeDisplay( const uint32_t nowMs )
{
    resetInteractionTimer( nowMs );
}

void TimerController::activate()
{
    display.stopSentenceScroll();
    resetInteractionTimer();
    resetStateTimer();
    onPotMoved();
    setState( TimerState::Setting );
    getActiveRenderer().renderSetting( display, targetSeconds );
}

void TimerController::startWithDuration( const uint16_t seconds )
{
    resetInteractionTimer();
    targetSeconds = ( seconds == 0 ) ? 30 : seconds;
    remainingSeconds = targetSeconds;
    countdownStartTimeMs = millis();
    pausedAccumulatedMs = 0;
    buzzer.beep( 30, countdownStartTimeMs );
    setState( TimerState::Running );
    getActiveRenderer().renderRunning( display, remainingSeconds, targetSeconds );
}

void TimerController::triggerAlarm()
{
    const uint32_t nowMs = millis();
    remainingSeconds = 0;
    buzzerSilenced = false;
    buzzer.playAlarmPattern( nowMs );
    lastInvertPhase = 255;
    setState( TimerState::Alarming );
    wakeDisplay( nowMs );
    getActiveRenderer().renderAlarming( display, false );
}

void TimerController::onPotMoved()
{
    if( getState() == TimerState::Setting ) {
        resetInteractionTimer();
        resetStateTimer();
        selectedPresetIndex = TimerPresets::getStepIndexForNormalized( leftPot.getNormalized() );
        targetSeconds = TimerPresets::getDurationSeconds( selectedPresetIndex );
        remainingSeconds = targetSeconds;
        getActiveRenderer().renderSetting( display, targetSeconds );
    }
}

void TimerController::onActionButton()
{
    const uint32_t nowMs = millis();
    resetInteractionTimer( nowMs );

    switch( getState() ) {
        case TimerState::Setting:
            countdownStartTimeMs = nowMs;
            pausedAccumulatedMs = 0;
            remainingSeconds = targetSeconds;
            buzzer.beep( 30, nowMs );
            setState( TimerState::Running );
            break;

        case TimerState::Running:
            pauseStartMs = nowMs;
            lastBlinkPhase = 255;
            setState( TimerState::Paused );
            break;

        case TimerState::Paused:
            pausedAccumulatedMs += ( nowMs - pauseStartMs );
            setState( TimerState::Running );
            break;

        case TimerState::Alarming:
            buzzer.stop();
            display.invertDisplay( false );
            setState( TimerState::Idle );
            display.clear();
            display.getDriver().display();
            break;

        case TimerState::Idle:
            break;
    }
}

void TimerController::onActionButtonLongPress()
{
    if( getState() == TimerState::Running || getState() == TimerState::Paused ) {
        resetInteractionTimer();
        remainingSeconds = targetSeconds;
        setState( TimerState::Setting );
        getActiveRenderer().renderSetting( display, targetSeconds );
    }
}

void TimerController::onSlap()
{
    if( getState() == TimerState::Alarming ) {
        buzzer.stop();
        display.invertDisplay( false );
        setState( TimerState::Idle );
        display.clear();
        display.getDriver().display();
    } else if( getState() == TimerState::Running ) {
        wakeDisplay();
    }
}

void TimerController::onProximityWake()
{
    if( getState() == TimerState::Running ) {
        if( proximity.getDistanceMm() < CLOSE_PROXIMITY_MM ) {
            wakeDisplay();
        }
    }
}

void TimerController::idleHandler()
{
}

void TimerController::settingHandler()
{
    if( stateElapsed( SETTING_INACTIVITY_TIMEOUT_MS ) ) {
        setState( TimerState::Idle );
        display.clear();
        display.getDriver().display();
        display.setPower( false );
    }
}

void TimerController::runningHandler()
{
    const uint32_t nowMs = millis();
    const uint32_t activeElapsedMs = ( nowMs - countdownStartTimeMs ) - pausedAccumulatedMs;
    const auto elapsedSec = static_cast<uint16_t>(activeElapsedMs / 1000);

    if( elapsedSec >= targetSeconds ) {
        remainingSeconds = 0;
        buzzerSilenced = false;
        buzzer.playAlarmPattern( nowMs );
        lastInvertPhase = 255;
        setState( TimerState::Alarming );
        wakeDisplay( nowMs );
        return;
    }

    remainingSeconds = targetSeconds - elapsedSec;

    // Power management
    const uint32_t idleTime = nowMs - lastInteractionMs;
    if( remainingSeconds <= AUTO_WAKE_SECONDS_THRESHOLD ) {
        if( !display.isPoweredOn() || dimmed ) {
            wakeDisplay( nowMs );
        }
    } else {
        if( idleTime >= SLEEP_TIMEOUT_MS ) {
            if( display.isPoweredOn() ) {
                display.setPower( false );
            }
        } else if( idleTime >= DIM_TIMEOUT_MS ) {
            if( !dimmed ) {
                dimmed = true;
                display.setBrightnessLevel( 1 );
            }
        }
    }

    if( display.isPoweredOn() ) {
        if( visualMode == TimerVisualMode::DigitalSand && digitalSandRenderer ) {
            digitalSandRenderer->updatePhysics( motion.getAccelX(), motion.getAccelY() );
        }
        getActiveRenderer().renderRunning( display, remainingSeconds, targetSeconds );
    }
}

void TimerController::pausedHandler()
{
    const uint32_t currentPhase = ( getStateTime() / 500 ) % 2;
    if( currentPhase != lastBlinkPhase ) {
        lastBlinkPhase = currentPhase;
        getActiveRenderer().renderPaused( display, remainingSeconds, targetSeconds, currentPhase == 0 );
    }
}

void TimerController::alarmingHandler()
{
    const uint32_t alarmElapsed = getStateTime();

    if( !buzzerSilenced && alarmElapsed >= ALARM_AUTO_SILENCE_MS ) {
        buzzerSilenced = true;
        buzzer.stop();
    }

    const uint32_t invertPhase = ( alarmElapsed / 250 ) % 2;
    if( invertPhase != lastInvertPhase ) {
        lastInvertPhase = invertPhase;
        getActiveRenderer().renderAlarming( display, invertPhase == 1 );
    }
}

void TimerController::update( const uint32_t nowMs )
{
    (void)nowMs;
    runStateMachine();
}
