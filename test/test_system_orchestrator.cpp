#include "controllers/systemOrchestrator.h"
#include "hardwareManager.h"
#include "i18n.h"
#include "sentenceEngine.h"
#include "services/powerManager.h"
#include "services/timeService.h"
#include "services/weatherService.h"

#include <Preferences.h>
#include <cassert>
#include <iostream>

struct OrchestratorTestHarness
{
    HardwareManager &hal = HardwareManager::instance();
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;

    DeskCompanionController companion;
    GagController gag;
    ArbitratorController arbitrator;
    TimerController timer;

    SystemOrchestrator orchestrator;

    OrchestratorTestHarness() :
        companion(
                hal.getDisplay(),
                timeService,
                weatherService,
                sentenceEngine,
                hal.getProximity(),
                hal.getLdr(),
                hal.getMotion()
                ),
        gag(
                hal.getDisplay(),
                hal.getBuzzer(),
                hal.getMotion(),
                hal.getLdr(),
                hal.getPowerSense()
                ),
        arbitrator(
                hal.getDisplay(),
                hal.getRightPot(),
                hal.getMotion()
                ),
        timer(
                hal.getDisplay(),
                hal.getLeftPot(),
                hal.getBuzzer(),
                hal.getMotion(),
                hal.getProximity()
                ),
        orchestrator( hal, companion, gag, arbitrator, timer, sentenceEngine )
    {
        setMockMillis( 0 );
        I18n::instance().init( Language::Italian );
        hal.begin();
        sentenceEngine.begin();

        companion.init( false );
        gag.init();
        arbitrator.init();
        timer.init();

        // Reset inputs to neutral
        hal.getActionButton().update( true, 0 );
        hal.getResetButton().update( true, 0 );
        hal.getLeftPot().updateRaw( 0 );
        (void)hal.getLeftPot().hasChanged();
        hal.getRightPot().updateRaw( 0 );
        (void)hal.getRightPot().hasChanged();
        (void)hal.getMotion().wasSlapped();
        (void)hal.getMotion().wasShaken();
        hal.getBuzzer().stop();
        hal.getDisplay().setPower( true );
    }

    void update( const uint32_t nowMs )
    {
        setMockMillis( nowMs );
        hal.getBuzzer().update( nowMs );
        orchestrator.update( nowMs );
    }

    void resetInputs( uint32_t nowMs )
    {
        hal.getActionButton().update( true, nowMs );
        hal.getResetButton().update( true, nowMs );
        (void)hal.getLeftPot().hasChanged();
        (void)hal.getRightPot().hasChanged();
        (void)hal.getMotion().wasSlapped();
        (void)hal.getMotion().wasShaken();
    }
};

void testBootDefaultDeskCompanion()
{
    std::cout << "Testing Boot Default DeskCompanion...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    h.orchestrator.update( 100 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testBootDefaultDeskCompanion\n";
}

void testBootDiagnosticsAndExit()
{
    std::cout << "Testing Boot Diagnostics and Action Button Exit...\n";
    OrchestratorTestHarness h;

    // Boot holding Action button
    h.hal.getActionButton().update( false, 0 );
    h.orchestrator.init( true );

    assert( h.orchestrator.getMode() == SystemMode::Diagnostics );

    // User releases the button after boot
    h.hal.getActionButton().update( true, 100 );
    h.update( 100 );

    // Mode must STILL be Diagnostics (not exited on initial release)
    assert( h.orchestrator.getMode() == SystemMode::Diagnostics );

    // Subsequent click to exit Diagnostics
    h.hal.getActionButton().update( false, 200 );
    h.hal.getActionButton().update( false, 240 );
    h.hal.getActionButton().update( true, 280 );
    h.update( 280 );

    // Should transition to DeskCompanion
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Complete CRT transition (>= 85ms)
    h.update( 280 + 90 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testBootDiagnosticsAndExit\n";
}

void testLeftPotSwitchesToTimerWithCrt()
{
    std::cout << "Testing Left Pot Turn Transitions to Timer...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    // Turn Left Pot significantly
    h.hal.getLeftPot().updateRaw( 2048 );

    h.orchestrator.update( 1000 );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Check intermediate CRT frames
    h.orchestrator.update( 1020 ); // dt = 20 < 30 (Frame 0)
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1040 ); // dt = 40 (Frame 1)
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1070 ); // dt = 70 (Frame 2)
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1090 ); // dt = 90 >= 85 (Frame 3 -> Timer)
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.isActive() );

    std::cout << "PASS: testLeftPotSwitchesToTimerWithCrt\n";
}

void testRightPotSwitchesToArbitratorWithCrt()
{
    std::cout << "Testing Right Pot Turn Transitions to Arbitrator...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    // Turn Right Pot significantly
    h.hal.getRightPot().updateRaw( 2500 );

    h.orchestrator.update( 1000 );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Complete transition
    h.orchestrator.update( 1090 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );
    assert( h.arbitrator.isActive() );

    std::cout << "PASS: testRightPotSwitchesToArbitratorWithCrt\n";
}

void testStrictPriorityLockTimerRunningAndAlarming()
{
    std::cout << "Testing Strict Priority Lock: Timer Running/Alarming Ignores Right Pot...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    // Switch directly to Timer and start countdown
    h.orchestrator.requestMode( SystemMode::Timer, 1000 );
    h.orchestrator.update( 1090 ); // Complete transition
    assert( h.orchestrator.getMode() == SystemMode::Timer );

    h.timer.startWithDuration( 60 );
    assert( h.timer.getState() == TimerState::Running );

    // Turn right pot: must be strictly ignored
    h.hal.getRightPot().updateRaw( 3000 );

    h.orchestrator.update( 1100 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.getState() == TimerState::Running );

    // Trigger alarm state
    h.timer.triggerAlarm();
    assert( h.timer.getState() == TimerState::Alarming );

    // Turn right pot again: must still be ignored
    h.hal.getRightPot().updateRaw( 1000 );

    h.orchestrator.update( 1200 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.getState() == TimerState::Alarming );

    // Slap bot to dismiss alarm
    h.hal.getMotion().feedRawData( 0, 0, 4.0f * 9.81f, 25.0f, 1300 );
    h.orchestrator.update( 1300 );

    // Alarm is dismissed, timer becomes idle, transitions back to DeskCompanion
    assert( !h.timer.isActive() );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1300 + 90 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testStrictPriorityLockTimerRunningAndAlarming\n";
}

void testPermittedHandoffTimerSettingToArbitrator()
{
    std::cout << "Testing Permitted Handoff: Timer Setting <-> Arbitrator...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    // Enter Timer (Setting state)
    h.orchestrator.requestMode( SystemMode::Timer, 1000 );
    h.orchestrator.update( 1090 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.getState() == TimerState::Setting );

    // Turn Right Pot: permitted handoff to Arbitrator
    h.hal.getRightPot().updateRaw( 3000 );

    h.orchestrator.update( 1200 );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1290 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );
    assert( h.arbitrator.isActive() );

    // Turn Left Pot: permitted handoff back to Timer
    h.hal.getLeftPot().updateRaw( 1500 );

    h.orchestrator.update( 1400 );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    h.orchestrator.update( 1490 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );

    std::cout << "PASS: testPermittedHandoffTimerSettingToArbitrator\n";
}

void testFactoryResetHoldCountdownAndAbort()
{
    std::cout << "Testing Factory Reset Hold Countdown and Abort...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    // Press and hold reset button at 1000ms
    h.hal.getResetButton().update( false, 1000 );
    h.hal.getResetButton().update( false, 1040 ); // debounced
    assert( h.hal.getResetButton().isPressed() );

    h.orchestrator.update( 1050 );
    assert( h.orchestrator.getMode() == SystemMode::FactoryReset );

    // Check countdown progression at 2000ms (1000ms elapsed)
    h.hal.getResetButton().update( false, 2000 );
    h.orchestrator.update( 2000 );
    assert( h.orchestrator.getMode() == SystemMode::FactoryReset );

    // Release button at 2500ms (< 3000ms hold)
    h.hal.getResetButton().update( true, 2500 );
    assert( !h.hal.getResetButton().isPressed() );

    h.orchestrator.update( 2500 );
    // Mode should revert to previous mode (DeskCompanion)
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testFactoryResetHoldCountdownAndAbort\n";
}

void testFactoryResetFullExecution()
{
    std::cout << "Testing Factory Reset Full 3-second Execution...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    ESP.restarted = false;
    Preferences::wasCleared = false;

    // Press and hold reset button at 1000ms
    h.hal.getResetButton().update( false, 1000 );
    h.hal.getResetButton().update( false, 1040 );
    h.orchestrator.update( 1050 );
    assert( h.orchestrator.getMode() == SystemMode::FactoryReset );

    // Hold until >= 3000ms (t = 4100ms)
    h.hal.getResetButton().update( false, 4100 );
    h.orchestrator.update( 4100 );

    assert( ESP.restarted );
    assert( Preferences::wasCleared );

    std::cout << "PASS: testFactoryResetFullExecution\n";
}

void testCanEnterSleepLogic()
{
    std::cout << "Testing canEnterSleep() across all modes...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    // DeskCompanion awake: cannot sleep
    assert( !h.orchestrator.canEnterSleep() );

    // FactoryReset: cannot sleep
    h.hal.getResetButton().update( false, 1000 );
    h.hal.getResetButton().update( false, 1040 );
    h.orchestrator.update( 1050 );
    assert( h.orchestrator.getMode() == SystemMode::FactoryReset );
    assert( !h.orchestrator.canEnterSleep() );

    // Abort reset
    h.hal.getResetButton().update( true, 1100 );
    h.orchestrator.update( 1100 );

    // Diagnostics: cannot sleep
    h.orchestrator.requestMode( SystemMode::Diagnostics, 1200 );
    h.orchestrator.update( 1290 );
    assert( h.orchestrator.getMode() == SystemMode::Diagnostics );
    assert( !h.orchestrator.canEnterSleep() );

    // Transitioning: cannot sleep
    h.orchestrator.requestMode( SystemMode::Timer, 1300 );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );
    assert( !h.orchestrator.canEnterSleep() );

    // Timer Setting/Running: cannot sleep
    h.orchestrator.update( 1390 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( !h.orchestrator.canEnterSleep() );

    // SlapGag active: cannot sleep
    h.orchestrator.requestMode( SystemMode::SlapGag, 1400 );
    h.orchestrator.update( 1490 );
    assert( h.orchestrator.getMode() == SystemMode::SlapGag );
    assert( !h.orchestrator.canEnterSleep() );

    // Arbitrator active: cannot sleep
    h.orchestrator.requestMode( SystemMode::Arbitrator, 1500 );
    h.orchestrator.update( 1590 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );
    assert( !h.orchestrator.canEnterSleep() );

    // Return to DeskCompanion and simulate sleeping
    h.orchestrator.requestMode( SystemMode::DeskCompanion, 1600 );
    h.orchestrator.update( 1690 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    // Fast-forward companion inactivity (30s) to enter sleeping
    h.orchestrator.update( 1690 + 31000 );
    assert( h.companion.canEnterSleep() );
    assert( h.orchestrator.canEnterSleep() );

    std::cout << "PASS: testCanEnterSleepLogic\n";
}

void testTransitionBypassedWhenDisplaySleeping()
{
    std::cout << "Testing CRT Transition Bypassed when Display is Sleeping...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    // Turn display off
    h.hal.getDisplay().setPower( false );
    assert( !h.hal.getDisplay().isPoweredOn() );

    // Requesting Timer mode should immediately switch without Transitioning mode
    h.orchestrator.requestMode( SystemMode::Timer, 1000 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.hal.getDisplay().isPoweredOn() );

    std::cout << "PASS: testTransitionBypassedWhenDisplaySleeping\n";
}

void testSlapGagInteractionAndRepair()
{
    std::cout << "Testing SlapGag Interaction and Slap Repair...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    h.orchestrator.requestMode( SystemMode::SlapGag, 20000 );
    h.update( 20090 );
    assert( h.orchestrator.getMode() == SystemMode::SlapGag );
    assert( h.gag.getState() == GagState::Glitching );

    // Right pot movements should be ignored
    h.hal.getRightPot().updateRaw( 2000 );
    h.update( 20150 );
    assert( h.orchestrator.getMode() == SystemMode::SlapGag );

    // Action button click triggers rejection state
    h.hal.getActionButton().update( false, 20200 );
    h.hal.getActionButton().update( false, 20240 );
    h.hal.getActionButton().update( true, 20280 );
    h.update( 20280 );
    assert( h.orchestrator.getMode() == SystemMode::SlapGag );
    assert( h.gag.getState() == GagState::RejectingButton );

    // Physical slap initiates repair sequence (21000ms is well past the 500ms debounce)
    h.hal.getMotion().feedRawData( 0, 0, 4.0f * 9.81f, 25.0f, 21000 );
    h.update( 21000 );
    assert( h.gag.getState() == GagState::RepairingAnimation );

    // Step 1: RepairingAnimation completes after 400ms -> RepairedSuccess
    h.update( 21000 + 400 );
    assert( h.gag.getState() == GagState::RepairedSuccess );

    // Step 2: RepairedSuccess completes after 2500ms -> Idle
    h.update( 21000 + 400 + 2500 );
    assert( !h.gag.isActive() );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Complete transition to DeskCompanion
    h.update( 21000 + 400 + 2500 + 90 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testSlapGagInteractionAndRepair\n";
}

void testArbitratorInactivityTimeout()
{
    std::cout << "Testing Arbitrator 15s Inactivity Timeout...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( false );

    h.orchestrator.requestMode( SystemMode::Arbitrator, 1000 );
    h.update( 1090 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );

    // Lock category after CATEGORY_LOCK_DELAY_MS (1000ms)
    h.update( 2100 );
    assert( h.arbitrator.getState() == ArbitratorState::CategoryLocked );

    // Advance 15.1 seconds in locked state -> inactivity timeout
    // Arbitrator powers off display on timeout, so CRT transition is bypassed directly to DeskCompanion
    h.update( 2100 + 15100 );
    assert( !h.arbitrator.isActive() );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    std::cout << "PASS: testArbitratorInactivityTimeout\n";
}

void testDiagnosticsShockAndShakeLatching()
{
    std::cout << "Testing Diagnostics Shock and Shake Latching...\n";
    OrchestratorTestHarness h;
    h.orchestrator.init( true );
    assert( h.orchestrator.getMode() == SystemMode::Diagnostics );

    h.update( 100 );
    assert( !h.hal.getBuzzer().isSounding() );

    // Trigger a shock spike (> 2.0g)
    h.hal.getMotion().feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 200 );
    h.update( 200 );

    // Buzzer should chirp on shock detection
    assert( h.hal.getBuzzer().isSounding() );

    // Advance beyond beep duration (25ms)
    h.update( 230 );
    assert( !h.hal.getBuzzer().isSounding() );

    // Trigger a lateral shake
    const float shakeSamples[ ] = { 15.0f, -15.0f, 15.0f, -15.0f };
    uint32_t t = 300;
    for( const float ax : shakeSamples ) {
        h.hal.getMotion().feedRawData( ax, 0.0f, 9.81f, 22.5f, t );
        t += 40;
    }
    h.update( t );

    // Buzzer should chirp on shake detection
    assert( h.hal.getBuzzer().isSounding() );

    std::cout << "PASS: testDiagnosticsShockAndShakeLatching\n";
}

int main()
{
    std::cout << "Running SystemOrchestrator Unit Tests...\n";
    testBootDefaultDeskCompanion();
    testBootDiagnosticsAndExit();
    testDiagnosticsShockAndShakeLatching();
    testLeftPotSwitchesToTimerWithCrt();
    testRightPotSwitchesToArbitratorWithCrt();
    testStrictPriorityLockTimerRunningAndAlarming();
    testPermittedHandoffTimerSettingToArbitrator();
    testFactoryResetHoldCountdownAndAbort();
    testFactoryResetFullExecution();
    testCanEnterSleepLogic();
    testTransitionBypassedWhenDisplaySleeping();
    testSlapGagInteractionAndRepair();
    testArbitratorInactivityTimeout();
    std::cout << "\n✅ ALL SYSTEM ORCHESTRATOR TESTS PASSED!\n";
    return 0;
}
