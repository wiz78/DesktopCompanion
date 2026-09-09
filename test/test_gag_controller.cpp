#include "controllers/gagController.h"
#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/ldrSensor.h"
#include "drivers/motionSensor.h"
#include "drivers/powerSense.h"
#include "i18n.h"

#include <cassert>
#include <iostream>

namespace
{
    struct TestHarness
    {
        DisplayManager display;
        Buzzer buzzer{ 18 };
        MotionSensor motion;
        LdrSensor ldr{ 3, 0.2f, 800, 300 };
        PowerSense powerSense{ 4 };
        GagController controller;

        TestHarness() :
            controller( display, buzzer, motion, ldr, powerSense )
        {
            setMockMillis( 0 );
            I18n::instance().init( Language::Italian );
            display.setup();
            buzzer.setup();
            ldr.setup();
            powerSense.setup();

            // Default: Daytime and USB powered
            for( int i = 0; i < 30; ++i ) {
                ldr.updateRaw( 2500 );
            }
            powerSense.updateRaw( true );
        }
    };
} // namespace

void testInitialState()
{
    TestHarness h;
    assert( !h.controller.isActive() );
    assert( h.controller.getState() == GagState::Idle );
    assert( h.controller.getTriggerCount() == 0 );
    std::cout << "PASS: testInitialState\n";
}

void testCanTriggerConditions()
{
    TestHarness h;

    // Default conditions: USB powered, daytime, no prior triggers -> should be eligible
    setMockMillis( 1000 );
    assert( h.controller.canTrigger( 1000 ) );

    // 1. USB power required (debounced after >= 100ms)
    h.powerSense.updateRaw( false );
    advanceMockMillis( 105 );
    h.powerSense.updateRaw( false );
    assert( !h.controller.canTrigger( 1105 ) );
    h.powerSense.updateRaw( true );
    advanceMockMillis( 105 );
    h.powerSense.updateRaw( true );
    assert( h.controller.canTrigger( 1210 ) );

    // 2. Daytime required (night is < 300)
    for( int i = 0; i < 30; ++i ) {
        h.ldr.updateRaw( 100 );
    }
    assert( h.ldr.isNight() );
    assert( !h.controller.canTrigger( 1000 ) );

    for( int i = 0; i < 30; ++i ) {
        h.ldr.updateRaw( 2500 );
    }
    assert( !h.ldr.isNight() );
    assert( h.controller.canTrigger( 1000 ) );

    // Trigger 1st time at t = 1000
    setMockMillis( 1000 );
    assert( h.controller.trigger( 1000 ) );
    assert( h.controller.getTriggerCount() == 1 );
    assert( h.controller.isActive() );
    // While active, canTrigger must be false
    assert( !h.controller.canTrigger( 1000 ) );

    // Fast-forward to finish 1st trigger: simulate 30s timeout
    setMockMillis( 1000 + GagController::GLITCH_TIMEOUT_MS );
    h.controller.update( 1000 + GagController::GLITCH_TIMEOUT_MS );
    assert( !h.controller.isActive() );
    assert( h.controller.getState() == GagState::Idle );

    // 3. Cooldown required (3 hours = 10,800,000 ms)
    // At t = 1000 + 1 hour, cooldown has not expired
    setMockMillis( 1000 + 3600000 );
    assert( !h.controller.canTrigger( 1000 + 3600000 ) );

    // At t = 1000 + 3 hours - 1ms, still not expired
    setMockMillis( 1000 + GagController::MIN_COOLDOWN_MS - 1 );
    assert( !h.controller.canTrigger( 1000 + GagController::MIN_COOLDOWN_MS - 1 ) );

    // At t = 1000 + 3 hours, cooldown expired -> can trigger
    const uint32_t t2 = 1000 + GagController::MIN_COOLDOWN_MS;
    setMockMillis( t2 );
    assert( h.controller.canTrigger( t2 ) );

    // Trigger 2nd time at t2
    assert( h.controller.trigger( t2 ) );
    assert( h.controller.getTriggerCount() == 2 );

    // Finish 2nd trigger
    setMockMillis( t2 + GagController::GLITCH_TIMEOUT_MS );
    h.controller.update( t2 + GagController::GLITCH_TIMEOUT_MS );
    assert( !h.controller.isActive() );

    // 4. Max 2 triggers per boot reached
    const uint32_t t3 = t2 + GagController::MIN_COOLDOWN_MS + 1000;
    setMockMillis( t3 );
    assert( !h.controller.canTrigger( t3 ) );

    // Even if canTrigger is false, force = true bypasses budget and conditions
    assert( h.controller.trigger( t3, true ) );
    assert( h.controller.isActive() );

    std::cout << "PASS: testCanTriggerConditions\n";
}

void testTriggerAndDisplayActivation()
{
    TestHarness h;
    h.display.setPower( false );
    assert( !h.display.isPoweredOn() );

    setMockMillis( 500 );
    const bool ok = h.controller.trigger( 500 );
    assert( ok );
    assert( h.controller.isActive() );
    assert( h.controller.getState() == GagState::Glitching );
    assert( h.display.isPoweredOn() );

    std::cout << "PASS: testTriggerAndDisplayActivation\n";
}

void testGlitchInactivityTimeout()
{
    TestHarness h;
    setMockMillis( 1000 );
    assert( h.controller.trigger( 1000 ) );
    assert( h.controller.getState() == GagState::Glitching );

    // Advance right before timeout
    setMockMillis( 1000 + GagController::GLITCH_TIMEOUT_MS - 1 );
    h.controller.update( 1000 + GagController::GLITCH_TIMEOUT_MS - 1 );
    assert( h.controller.getState() == GagState::Glitching );
    assert( h.controller.isActive() );

    // Advance to timeout (30,000 ms)
    setMockMillis( 1000 + GagController::GLITCH_TIMEOUT_MS );
    h.controller.update( 1000 + GagController::GLITCH_TIMEOUT_MS );
    assert( h.controller.getState() == GagState::Idle );
    assert( !h.controller.isActive() );
    assert( !h.display.isPoweredOn() ); // Turns off display to sleep

    std::cout << "PASS: testGlitchInactivityTimeout\n";
}

void testActionButtonRejectionAndTimeout()
{
    TestHarness h;
    setMockMillis( 2000 );
    assert( h.controller.trigger( 2000 ) );

    // 1st button press: transitions to RejectingButton and sounds a click beep
    setMockMillis( 2100 );
    h.controller.onActionButton( 2100 );
    assert( h.controller.getState() == GagState::RejectingButton );
    assert( h.buzzer.isSounding() );

    // Stop buzzer to verify it doesn't sound again during reject
    h.buzzer.stop();

    // Advance 1499ms -> still RejectingButton
    setMockMillis( 2100 + GagController::REJECT_DURATION_MS - 1 );
    h.controller.update( 2100 + GagController::REJECT_DURATION_MS - 1 );
    assert( h.controller.getState() == GagState::RejectingButton );

    // Advance to 1500ms -> reverts to Glitching
    setMockMillis( 2100 + GagController::REJECT_DURATION_MS );
    h.controller.update( 2100 + GagController::REJECT_DURATION_MS );
    assert( h.controller.getState() == GagState::Glitching );
    assert( h.controller.isActive() );

    std::cout << "PASS: testActionButtonRejectionAndTimeout\n";
}

void testActionButtonPeacefulFallback()
{
    TestHarness h;
    setMockMillis( 3000 );
    assert( h.controller.trigger( 3000 ) );

    // 1st button press
    setMockMillis( 3100 );
    h.controller.onActionButton( 3100 );
    assert( h.controller.getState() == GagState::RejectingButton );

    // 2nd button press within rejection window -> peaceful fallback repair
    setMockMillis( 3300 );
    h.controller.onActionButton( 3300 );
    assert( h.controller.getState() == GagState::RepairingAnimation );

    std::cout << "PASS: testActionButtonPeacefulFallback\n";
}

void testSlapResolutionAndRepairSequence()
{
    TestHarness h;
    setMockMillis( 4000 );
    assert( h.controller.trigger( 4000 ) );

    // Slap occurs at t = 4200
    setMockMillis( 4200 );
    h.controller.onSlap( 4200 );
    assert( h.controller.getState() == GagState::RepairingAnimation );

    // Phase 1 (0 to 120ms): White flash (filled with SSD1306_WHITE, normal polarity)
    setMockMillis( 4200 + 50 );
    h.controller.update( 4200 + 50 );
    assert( h.controller.getState() == GagState::RepairingAnimation );
    assert( !h.display.getDriver().isInverted() );

    // Phase 2 (120 to 400ms): CRT collapse (display normal polarity)
    setMockMillis( 4200 + 200 );
    h.controller.update( 4200 + 200 );
    assert( h.controller.getState() == GagState::RepairingAnimation );
    assert( !h.display.getDriver().isInverted() );

    // Phase 3 (>= 400ms): Transitions to RepairedSuccess with double beep
    h.buzzer.stop();
    setMockMillis( 4200 + GagController::FLASH_DURATION_MS + GagController::COLLAPSE_DURATION_MS );
    h.controller.update( 4200 + GagController::FLASH_DURATION_MS + GagController::COLLAPSE_DURATION_MS );
    assert( h.controller.getState() == GagState::RepairedSuccess );
    assert( h.buzzer.isSounding() );
    assert( !h.display.getDriver().isInverted() );

    // Diagnosis duration (2500ms)
    const uint32_t successStart = 4200 + GagController::FLASH_DURATION_MS + GagController::COLLAPSE_DURATION_MS;
    setMockMillis( successStart + GagController::SUCCESS_DURATION_MS - 1 );
    h.controller.update( successStart + GagController::SUCCESS_DURATION_MS - 1 );
    assert( h.controller.getState() == GagState::RepairedSuccess );
    assert( h.controller.isActive() );

    // At 2500ms, transitions back to Idle
    setMockMillis( successStart + GagController::SUCCESS_DURATION_MS );
    h.controller.update( successStart + GagController::SUCCESS_DURATION_MS );
    assert( h.controller.getState() == GagState::Idle );
    assert( !h.controller.isActive() );

    std::cout << "PASS: testSlapResolutionAndRepairSequence\n";
}

void testSlapResolutionFromRejectingButton()
{
    TestHarness h;
    setMockMillis( 5000 );
    assert( h.controller.trigger( 5000 ) );

    setMockMillis( 5100 );
    h.controller.onActionButton( 5100 );
    assert( h.controller.getState() == GagState::RejectingButton );

    // Slap during rejection window transitions to RepairingAnimation
    setMockMillis( 5300 );
    h.controller.onSlap( 5300 );
    assert( h.controller.getState() == GagState::RepairingAnimation );

    std::cout << "PASS: testSlapResolutionFromRejectingButton\n";
}

void testMotionSensorSlapIntegration()
{
    TestHarness h;
    setMockMillis( 6000 );
    assert( h.controller.trigger( 6000 ) );

    // Sudden slap spike (> 2.4g) fed to MotionSensor
    setMockMillis( 6100 );
    h.motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, 6100 );

    // Dispatcher polls MotionSensor and forwards to onSlap
    if( h.motion.wasSlapped() ) {
        h.controller.onSlap( 6100 );
    }

    // update() runs animation
    h.controller.update( 6100 );
    assert( h.controller.getState() == GagState::RepairingAnimation );

    std::cout << "PASS: testMotionSensorSlapIntegration\n";
}

void testSecondarySlapDuringRepairAnimationDoesNotSkipTick()
{
    TestHarness h;
    setMockMillis( 7000 );
    assert( h.controller.trigger( 7000 ) );

    // Enter RepairingAnimation via slap
    setMockMillis( 7100 );
    h.controller.onSlap( 7100 );
    assert( h.controller.getState() == GagState::RepairingAnimation );

    // Advance time to 7100 + 400ms (end of collapse), and simulate a secondary slap occurring simultaneously
    const uint32_t tCollapseEnd = 7100 + GagController::FLASH_DURATION_MS + GagController::COLLAPSE_DURATION_MS;
    setMockMillis( tCollapseEnd );
    h.motion.feedRawData( 15.0f, 20.0f, 20.0f, 22.5f, tCollapseEnd );

    if( h.motion.wasSlapped() ) {
        h.controller.onSlap( tCollapseEnd );
    }

    // update() should process animation without aborting tick early, thus transitioning to RepairedSuccess
    h.controller.update( tCollapseEnd );
    assert( h.controller.getState() == GagState::RepairedSuccess );

    std::cout << "PASS: testSecondarySlapDuringRepairAnimationDoesNotSkipTick\n";
}

void test_gag_can_enter_sleep()
{
    TestHarness h;
    setMockMillis( 1000 );

    // Initial Idle -> canEnterSleep should be true
    assert( h.controller.getState() == GagState::Idle );
    assert( h.controller.canEnterSleep() );

    // Trigger gag -> Glitching -> canEnterSleep should be false
    assert( h.controller.trigger( 1000 ) );
    assert( h.controller.getState() == GagState::Glitching );
    assert( !h.controller.canEnterSleep() );

    // Button press -> RejectingButton -> canEnterSleep should be false
    setMockMillis( 1100 );
    h.controller.onActionButton( 1100 );
    assert( h.controller.getState() == GagState::RejectingButton );
    assert( !h.controller.canEnterSleep() );

    // Slap -> RepairingAnimation -> canEnterSleep should be false
    setMockMillis( 1200 );
    h.controller.onSlap( 1200 );
    assert( h.controller.getState() == GagState::RepairingAnimation );
    assert( !h.controller.canEnterSleep() );

    // Advance through repair animation to RepairedSuccess -> canEnterSleep should be false
    const uint32_t tSuccess = 1200 + GagController::FLASH_DURATION_MS + GagController::COLLAPSE_DURATION_MS;
    setMockMillis( tSuccess );
    h.controller.update( tSuccess );
    assert( h.controller.getState() == GagState::RepairedSuccess );
    assert( !h.controller.canEnterSleep() );

    // Advance through success duration -> Idle -> canEnterSleep should be true
    const uint32_t tIdle = tSuccess + GagController::SUCCESS_DURATION_MS;
    setMockMillis( tIdle );
    h.controller.update( tIdle );
    assert( h.controller.getState() == GagState::Idle );
    assert( h.controller.canEnterSleep() );

    // Also test glitch timeout: trigger again (forced since cooldown not expired)
    assert( h.controller.trigger( tIdle + 100, true ) );
    assert( h.controller.getState() == GagState::Glitching );
    assert( !h.controller.canEnterSleep() );

    // Timeout (30s) -> Idle -> canEnterSleep should be true
    setMockMillis( tIdle + 100 + GagController::GLITCH_TIMEOUT_MS );
    h.controller.update( tIdle + 100 + GagController::GLITCH_TIMEOUT_MS );
    assert( h.controller.getState() == GagState::Idle );
    assert( h.controller.canEnterSleep() );

    std::cout << "PASS: test_gag_can_enter_sleep\n";
}

int main()
{
    testInitialState();
    testCanTriggerConditions();
    testTriggerAndDisplayActivation();
    testGlitchInactivityTimeout();
    testActionButtonRejectionAndTimeout();
    testActionButtonPeacefulFallback();
    testSlapResolutionAndRepairSequence();
    testSlapResolutionFromRejectingButton();
    testMotionSensorSlapIntegration();
    testSecondarySlapDuringRepairAnimationDoesNotSkipTick();
    test_gag_can_enter_sleep();

    std::cout << "\nAll GagController tests passed!\n";
    return 0;
}
