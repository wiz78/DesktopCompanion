//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/systemOrchestrator.h"
#include "services/networkWorker.h"
#include "services/powerManager.h"

#include <Arduino.h>
#if __has_include( <Preferences.h> )
#include <Preferences.h>
#endif
#if __has_include( <LittleFS.h> )
#include <LittleFS.h>
#endif

SystemOrchestrator::SystemOrchestrator( HardwareManager& halRef, DeskCompanionController& companionRef,
                                        GagController& gagRef, ArbitratorController& arbitratorRef,
                                        TimerController& timerRef, SentenceEngine& sentenceEngineRef ) : hal( halRef ),
    companion( companionRef ), gag( gagRef ), arbitrator( arbitratorRef ), timer( timerRef ),
    sentenceEngine( sentenceEngineRef )
{
}

void SystemOrchestrator::init( const bool enterDiagnosticsAtBoot )
{
    if( enterDiagnosticsAtBoot ) {
        currentMode = SystemMode::Diagnostics;
        diagnosticsAwaitingRelease = true;
    } else {
        currentMode = SystemMode::DeskCompanion;
        diagnosticsAwaitingRelease = false;
    }
    targetMode = currentMode;
    previousMode = currentMode;
    resetButtonPressed = false;
    resetPressStartMs = 0;
    lastDisplayedRemaining = 0;
}

SystemMode SystemOrchestrator::getMode() const
{
    return currentMode;
}

bool SystemOrchestrator::canEnterSleep() const
{
    switch( currentMode ) {
        case SystemMode::FactoryReset:
        case SystemMode::Diagnostics:
        case SystemMode::Transitioning:
            return false;
        case SystemMode::Timer:
            return timer.canEnterSleep();
        case SystemMode::SlapGag:
            return gag.canEnterSleep();
        case SystemMode::Arbitrator:
            return arbitrator.canEnterSleep();
        case SystemMode::DeskCompanion:
            return companion.canEnterSleep();
        default:
            return true;
    }
}

void SystemOrchestrator::requestMode( const SystemMode target, const uint32_t nowMs )
{
    if( currentMode == target && currentMode != SystemMode::Transitioning ) {
        return;
    }

    PowerManager::instance().resetActivity( nowMs );

    if( !hal.getDisplay().isPoweredOn() ) {
        // Display is sleeping: bypass CRT shutter animation for instant wake-up
        activateMode( target, nowMs );
        return;
    }

    targetMode = target;
    transitionStartMs = nowMs;
    currentMode = SystemMode::Transitioning;
    hal.getDisplay().displayCrtShutter( 0 );
}

void SystemOrchestrator::activateMode( const SystemMode mode, const uint32_t nowMs )
{
    currentMode = mode;
    targetMode = mode;

    switch( mode ) {
        case SystemMode::DeskCompanion:
            hal.getDisplay().setPower( true );
            companion.onUserActionClick( nowMs );
            break;
        case SystemMode::Timer:
            hal.getDisplay().setPower( true );
            if( !timer.isActive() ) {
                timer.activate();
            }
            break;
        case SystemMode::Arbitrator:
            hal.getDisplay().setPower( true );
            if( !arbitrator.isActive() ) {
                arbitrator.activate();
            }
            break;
        case SystemMode::SlapGag:
            hal.getDisplay().setPower( true );
            if( !gag.isActive() ) {
                gag.trigger( nowMs, true );
            }
            break;
        case SystemMode::Diagnostics:
            hal.getDisplay().setPower( true );
            break;
        default:
            break;
    }
}

void SystemOrchestrator::update( const uint32_t nowMs )
{
    // P0: Factory reset button evaluation (unconditionally at the top of every cycle)
    if( hal.getResetButton().isPressed() ) {
        handleFactoryReset( nowMs );
        return;
    } else if( resetButtonPressed ) {
        PowerManager::instance().resetActivity( nowMs );
        resetButtonPressed = false;
        lastDisplayedRemaining = 0;
        Serial.println( "[RESET] Factory reset cancelled by user." );

        currentMode = previousMode;
        // Redraw display for restored mode
        if( currentMode == SystemMode::Timer ) {
            // Redrawn by timer on next update
        } else if( currentMode == SystemMode::Arbitrator ) {
            arbitrator.activate();
        } else if( currentMode == SystemMode::DeskCompanion ) {
            if( companion.getState() != CompanionState::Sleeping ) {
                hal.getDisplay().setPower( true );
                companion.onUserActionClick( nowMs );
            }
        }
        return;
    }

    switch( currentMode ) {
        case SystemMode::Transitioning:
            handleTransitioning( nowMs );
            break;

        case SystemMode::Diagnostics:
            handleDiagnostics( nowMs );
            break;

        case SystemMode::Timer:
            handleTimer( nowMs );
            break;

        case SystemMode::SlapGag:
            handleSlapGag( nowMs );
            break;

        case SystemMode::Arbitrator:
            handleArbitrator( nowMs );
            break;

        case SystemMode::DeskCompanion:
            handleDeskCompanion( nowMs );
            break;

        case SystemMode::FactoryReset:
            currentMode = previousMode;
            break;
    }
}

void SystemOrchestrator::handleFactoryReset( const uint32_t nowMs )
{
    PowerManager::instance().resetActivity( nowMs );

    if( !resetButtonPressed ) {
        resetButtonPressed = true;
        resetPressStartMs = nowMs;
        lastDisplayedRemaining = 0;
        if( currentMode != SystemMode::FactoryReset ) {
            previousMode = ( currentMode == SystemMode::Transitioning ) ? targetMode : currentMode;
            currentMode = SystemMode::FactoryReset;
        }
    }

    const uint32_t pressMs = nowMs - resetPressStartMs;
    if( pressMs < FACTORY_RESET_HOLD_MS ) {
        const uint8_t remaining = static_cast<uint8_t>(3 - ( pressMs / 1000 ));
        const uint8_t toDisplay = ( remaining > 0 ) ? remaining : 1;
        if( toDisplay != lastDisplayedRemaining ) {
            hal.getDisplay().displayResetCountdown( toDisplay );
            lastDisplayedRemaining = toDisplay;
        }
    } else {
        Serial.println( "[RESET] Factory reset triggered! Erasing settings and rebooting..." );
        hal.getBuzzer().playAlarmPattern();

#if __has_include( <Preferences.h> )
        Preferences prefs; if( prefs.begin( "friendlybot", false ) ) {
            prefs.clear();
            prefs.end();
        }
#endif

#if __has_include( <LittleFS.h> )
        if( LittleFS.exists( "/sentences.json" ) ) {
            LittleFS.remove( "/sentences.json" );
        }
#endif

        sentenceEngine.clearCustomSentences();
        NetworkWorker::instance().clearWifiCredentials();

        ESP.restart();
    }
}

void SystemOrchestrator::handleTransitioning( const uint32_t nowMs )
{
    if( const uint32_t dt = nowMs - transitionStartMs; dt < TRANSITION_FRAME0_MS ) {
        hal.getDisplay().displayCrtShutter( 0 );
    } else if( dt < TRANSITION_FRAME1_MS ) {
        hal.getDisplay().displayCrtShutter( 1 );
    } else if( dt < TRANSITION_FRAME2_MS ) {
        hal.getDisplay().displayCrtShutter( 2 );
    } else {
        hal.getDisplay().displayCrtShutter( 3 );
        activateMode( targetMode, nowMs );
    }
}

void SystemOrchestrator::handleDiagnostics( const uint32_t nowMs )
{
    if( diagnosticsAwaitingRelease ) {
        (void)hal.getActionButton().wasClicked();
        if( !hal.getActionButton().isPressed() ) {
            diagnosticsAwaitingRelease = false;
        }
    } else if( hal.getActionButton().wasClicked() ) {
        PowerManager::instance().resetActivity( nowMs );
        hal.getBuzzer().beep( 30 );
        requestMode( SystemMode::DeskCompanion, nowMs );
        return;
    }

    if( hal.getMotion().wasSlapped() ) {
        diagLastShockMs = nowMs;
        hal.getBuzzer().beep( 25, nowMs );
    }

    if( hal.getMotion().wasShaken() ) {
        diagLastShakeMs = nowMs;
        hal.getBuzzer().beep( 50, nowMs );
    }

    static constexpr uint32_t DIAG_LATCH_MS = 1500;
    const bool showShock = ( diagLastShockMs > 0 ) && ( ( nowMs - diagLastShockMs ) < DIAG_LATCH_MS );
    const bool showShake = ( diagLastShakeMs > 0 ) && ( ( nowMs - diagLastShakeMs ) < DIAG_LATCH_MS );

    hal.getDisplay().displayDiagnostics( hal.getLeftPot().getNormalized(),
                                         hal.getLeftPot().getMappedNonLinear( 0.5f, 30.0f ),
                                         hal.getRightPot().getStep( 4 ), hal.getLdr().getRawValue(),
                                         hal.getLdr().isNight(), hal.getPowerSense().isUsbPowered(),
                                         hal.getProximity().getDistanceMm(), hal.getMotion().getTemperatureC(),
                                         showShock, showShake );
}

void SystemOrchestrator::handleTimer( const uint32_t nowMs )
{
    const bool wasActive = timer.isActive();
    const TimerState tState = timer.getState();
    const bool isStrictLock = ( tState == TimerState::Running || tState == TimerState::Alarming );

    if( isStrictLock ) {
        // Priority P2: Right pot movements and random slap gags strictly ignored
        if( hal.getRightPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
            // Consume and ignore
        }

        if( hal.getActionButton().wasLongPressed( 1500, nowMs ) ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onActionButtonLongPress();
        } else if( hal.getActionButton().wasClicked() ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onActionButton();
        }

        if( hal.getMotion().wasSlapped() ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onSlap();
        }

        if( hal.getProximity().getDistanceMm() < 500 ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onProximityWake();
        }

        if( hal.getLeftPot().hasChanged( POT_INTERACTION_THRESHOLD ) ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onPotMoved();
        }
    } else {
        // Priority P4: Setting or Paused
        if( hal.getRightPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
            PowerManager::instance().resetActivity( nowMs );
            requestMode( SystemMode::Arbitrator, nowMs );
            return;
        }

        if( hal.getLeftPot().hasChanged( POT_INTERACTION_THRESHOLD ) ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onPotMoved();
        }

        if( hal.getActionButton().wasLongPressed( 1500, nowMs ) ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onActionButtonLongPress();
        } else if( hal.getActionButton().wasClicked() ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onActionButton();
        }

        if( hal.getProximity().getDistanceMm() < 500 ) {
            PowerManager::instance().resetActivity( nowMs );
            timer.onProximityWake();
        }
    }

    timer.update( nowMs );

    if( wasActive && !timer.isActive() ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::DeskCompanion, nowMs );
    }
}

void SystemOrchestrator::handleSlapGag( const uint32_t nowMs )
{
    const bool wasActive = gag.isActive();

    // Priority P3: Right pot and left pot turns ignored
    if( hal.getRightPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
        // Consume and ignore
    }
    if( hal.getLeftPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
        // Consume and ignore
    }

    if( hal.getActionButton().wasClicked() ) {
        PowerManager::instance().resetActivity( nowMs );
        gag.onActionButton( nowMs );
    }

    if( hal.getMotion().wasSlapped() ) {
        PowerManager::instance().resetActivity( nowMs );
        gag.onSlap( nowMs );
    }

    gag.update( nowMs );

    if( wasActive && !gag.isActive() ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::DeskCompanion, nowMs );
    }
}

void SystemOrchestrator::handleArbitrator( const uint32_t nowMs )
{
    const bool wasActive = arbitrator.isActive();

    // Priority P4: Turning left pot transitions to Timer
    if( hal.getLeftPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::Timer, nowMs );
        return;
    }

    if( hal.getRightPot().hasChanged( POT_INTERACTION_THRESHOLD ) ) {
        PowerManager::instance().resetActivity( nowMs );
        arbitrator.onPotMoved();
    }

    if( hal.getActionButton().wasClicked() ) {
        PowerManager::instance().resetActivity( nowMs );
        arbitrator.onActionButton();
    }

    if( hal.getMotion().wasShaken() ) {
        PowerManager::instance().resetActivity( nowMs );
        arbitrator.onShake();
    }

    arbitrator.update( nowMs );

    if( wasActive && !arbitrator.isActive() ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::DeskCompanion, nowMs );
    }
}

void SystemOrchestrator::handleDeskCompanion( const uint32_t nowMs )
{
    // Priority P5: Turning left pot transitions to Timer
    if( hal.getLeftPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::Timer, nowMs );
        return;
    }

    // Turning right pot transitions to Arbitrator
    if( hal.getRightPot().hasChanged( POT_CHANGE_THRESHOLD ) ) {
        PowerManager::instance().resetActivity( nowMs );
        requestMode( SystemMode::Arbitrator, nowMs );
        return;
    }

    if( hal.getProximity().isTargetPresent() )
        PowerManager::instance().resetActivity( nowMs );

    // Check daytime slap gag trigger
    if( companion.isAboutToSleep( nowMs ) && gag.canTrigger( nowMs ) ) {
        gag.trigger( nowMs );
        requestMode( SystemMode::SlapGag, nowMs );
        return;
    }

    const CompanionState prevState = companion.getState();
    companion.update( nowMs );
    if( prevState == CompanionState::Sleeping && companion.getState() != CompanionState::Sleeping ) {
        PowerManager::instance().resetActivity( nowMs );
    }

    if( hal.getActionButton().wasClicked() ) {
        PowerManager::instance().resetActivity( nowMs );
        if( !companion.isNightMode() ) {
            hal.getBuzzer().beep( 30 );
        }
        companion.onUserActionClick( nowMs );
    }

    if( hal.getMotion().wasSlapped() ) {
        PowerManager::instance().resetActivity( nowMs );
    }
}
