#include "controllers/renderers/digitalSandRenderer.h"
#include "controllers/renderers/digitsBarRenderer.h"
#include "controllers/timerController.h"
#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/motionSensor.h"
#include "drivers/potentiometer.h"
#include "drivers/proximitySensor.h"
#include "i18n.h"

#include <cassert>
#include <iostream>

void test_initial_state_idle()
{
    setMockMillis( 0 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();

    assert( timer.getState() == TimerState::Idle );
    assert( !timer.isActive() );
    assert( !timer.isAlarming() );
    std::cout << "PASS: test_initial_state_idle\n";
}

void test_activation_and_inactivity_timeout()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 2048 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();

    timer.activate();
    assert( timer.getState() == TimerState::Setting );
    assert( timer.isActive() );

    // Advance 14.9s -> still Setting
    advanceMockMillis( 14900 );
    timer.update();
    assert( timer.getState() == TimerState::Setting );

    // Advance past 15.0s -> returns to Idle
    advanceMockMillis( 200 );
    timer.update();
    assert( timer.getState() == TimerState::Idle );
    std::cout << "PASS: test_activation_and_inactivity_timeout\n";
}

void test_start_pause_resume_flow()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 0 ); // Preset 0 = 30s
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();

    // Click Action Button -> Starts countdown
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );
    assert( timer.getRemainingSeconds() == 30 );

    // Run 5 seconds
    advanceMockMillis( 5000 );
    timer.update();
    assert( timer.getRemainingSeconds() == 25 );

    // Click Action Button -> Pause
    timer.onActionButton();
    assert( timer.getState() == TimerState::Paused );
    assert( timer.getRemainingSeconds() == 25 );

    // Time passes while paused -> remaining time does not decrease
    advanceMockMillis( 10000 );
    timer.update();
    assert( timer.getRemainingSeconds() == 25 );

    // Click Action Button -> Resume
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );

    // Run 25 seconds -> Reaches 0 -> Alarming
    advanceMockMillis( 25000 );
    timer.update();
    assert( timer.getState() == TimerState::Alarming );
    assert( timer.isAlarming() );
    assert( buzzer.isSounding() );
    std::cout << "PASS: test_start_pause_resume_flow\n";
}

void test_long_press_cancel()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );

    // Long press cancels timer back to Setting
    timer.onActionButtonLongPress();
    assert( timer.getState() == TimerState::Setting );
    std::cout << "PASS: test_long_press_cancel\n";
}

void test_power_policy_dim_sleep_and_wake()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 2048 ); // Longer timer
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton(); // Running
    assert( timer.getState() == TimerState::Running );

    // Advance 20 seconds -> Dimmed
    advanceMockMillis( 20000 );
    timer.update();
    assert( timer.isDimmed() );
    assert( display.isPoweredOn() );

    // Advance another 10 seconds (total 30s) -> Sleeping
    advanceMockMillis( 10000 );
    timer.update();
    assert( !display.isPoweredOn() );

    // Wake via Proximity (<500mm)
    proximity.feedRawDistance( 400, 0 );
    timer.onProximityWake();
    assert( display.isPoweredOn() );
    assert( !timer.isDimmed() );

    // Dim and Sleep again
    advanceMockMillis( 30000 );
    timer.update();
    assert( !display.isPoweredOn() );

    // Wake via Slap/Bump
    timer.onSlap();
    assert( display.isPoweredOn() );
    assert( !timer.isDimmed() );

    std::cout << "PASS: test_power_policy_dim_sleep_and_wake\n";
}

void test_auto_wake_at_30s_mark()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 350 ); // Preset 2 = 90s
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton(); // Running 90s timer

    // Advance 35 seconds -> 55s remaining (>30s) and idle 35s (>=30s) -> Display sleeps
    advanceMockMillis( 35000 );
    timer.update();
    assert( !display.isPoweredOn() );
    assert( timer.getRemainingSeconds() == 55 );

    // Advance another 25 seconds (total 60s elapsed) -> Reaches exactly 30s remaining
    // Should auto-wake display
    advanceMockMillis( 25000 );
    timer.update();
    assert( display.isPoweredOn() );
    assert( timer.getRemainingSeconds() == 30 );
    std::cout << "PASS: test_auto_wake_at_30s_mark\n";
}

void test_alarm_dismissal_slap_and_button()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    // Test dismissal via button
    {
        TimerController timer( display, leftPot, buzzer, motion, proximity );
        timer.init();
        timer.activate();
        timer.onActionButton(); // Running
        advanceMockMillis( 30000 );
        timer.update();
        assert( timer.getState() == TimerState::Alarming );

        timer.onActionButton(); // Dismiss
        assert( timer.getState() == TimerState::Idle );
        assert( !buzzer.isSounding() );
    }

    // Test dismissal via slap
    {
        TimerController timer( display, leftPot, buzzer, motion, proximity );
        timer.init();
        timer.activate();
        timer.onActionButton(); // Running
        advanceMockMillis( 30000 );
        timer.update();
        assert( timer.getState() == TimerState::Alarming );

        timer.onSlap(); // Dismiss via slap
        assert( timer.getState() == TimerState::Idle );
        assert( !buzzer.isSounding() );
    }

    std::cout << "PASS: test_alarm_dismissal_slap_and_button\n";
}

void test_alarm_auto_silence_after_2_minutes()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton();
    advanceMockMillis( 30000 );
    timer.update();
    assert( timer.getState() == TimerState::Alarming );
    assert( buzzer.isSounding() );

    // Advance 120 seconds in Alarming
    advanceMockMillis( 120000 );
    timer.update();
    assert( timer.getState() == TimerState::Alarming );
    assert( !buzzer.isSounding() ); // Silenced automatically

    // Can still be dismissed to Idle
    timer.onActionButton();
    assert( timer.getState() == TimerState::Idle );
    std::cout << "PASS: test_alarm_auto_silence_after_2_minutes\n";
}

void test_visual_modes_and_physics()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 0 ); // 30s
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();

    assert( timer.getVisualMode() == TimerVisualMode::DigitsWithBar );
    timer.setVisualMode( TimerVisualMode::DigitalSand );
    assert( timer.getVisualMode() == TimerVisualMode::DigitalSand );

    timer.activate();
    timer.onActionButton(); // Running

    // Update with accelerometer physics
    motion.feedRawData( 0.2f, 0.9f, 0.0f, 20.0f, 0 );
    timer.update();
    assert( timer.getState() == TimerState::Running );

    std::cout << "PASS: test_visual_modes_and_physics\n";
}

void test_pot_movement_behavior()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 0 ); // 30s
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    assert( timer.getTargetSeconds() == 30 );

    // Move pot to preset 2 (90s) while in Setting
    leftPot.updateRaw( 350 );
    timer.onPotMoved();
    assert( timer.getTargetSeconds() == 90 );
    assert( timer.getRemainingSeconds() == 90 );

    // Start timer
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );

    // Move pot while running -> ignored
    leftPot.updateRaw( 2048 );
    timer.onPotMoved();
    assert( timer.getTargetSeconds() == 90 );
    assert( timer.getState() == TimerState::Running );

    std::cout << "PASS: test_pot_movement_behavior\n";
}

void test_paused_long_press_and_multi_pause()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 0 ); // 30s
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton(); // Running (target 30s)

    // Run 5s
    advanceMockMillis( 5000 );
    timer.update();
    assert( timer.getRemainingSeconds() == 25 );

    // Pause 1
    timer.onActionButton();
    assert( timer.getState() == TimerState::Paused );
    advanceMockMillis( 3000 );
    timer.update();

    // Resume 1
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );

    // Run 5s
    advanceMockMillis( 5000 );
    timer.update();
    assert( timer.getRemainingSeconds() == 20 );

    // Pause 2
    timer.onActionButton();
    assert( timer.getState() == TimerState::Paused );

    // Long press while paused -> cancels back to Setting with full 30s
    timer.onActionButtonLongPress();
    assert( timer.getState() == TimerState::Setting );
    assert( timer.getRemainingSeconds() == 30 );

    std::cout << "PASS: test_paused_long_press_and_multi_pause\n";
}

void test_proximity_threshold_wake()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1 );
    leftPot.updateRaw( 2048 ); // Long timer
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    timer.onActionButton(); // Running

    // Advance 30s -> display sleeps
    advanceMockMillis( 30000 );
    timer.update();
    assert( !display.isPoweredOn() );

    // Proximity >= 500mm (e.g. 600mm) -> should NOT wake
    proximity.feedRawDistance( 600, 0 );
    timer.onProximityWake();
    assert( !display.isPoweredOn() );

    // Proximity < 500mm (e.g. 450mm) -> SHOULD wake
    proximity.feedRawDistance( 450, 0 );
    timer.onProximityWake();
    assert( display.isPoweredOn() );

    std::cout << "PASS: test_proximity_threshold_wake\n";
}

void test_start_with_duration_and_trigger_alarm()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();

    // Start with custom 45 seconds
    timer.startWithDuration( 45 );
    assert( timer.getState() == TimerState::Running );
    assert( timer.getTargetSeconds() == 45 );
    assert( timer.getRemainingSeconds() == 45 );

    // Force trigger alarm
    timer.triggerAlarm();
    assert( timer.getState() == TimerState::Alarming );
    assert( timer.isAlarming() );
    assert( buzzer.isSounding() );

    std::cout << "PASS: test_start_with_duration_and_trigger_alarm\n";
}

void test_timer_can_enter_sleep()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 0 ); // 30s preset
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();

    // Idle -> true
    assert( timer.getState() == TimerState::Idle );
    assert( timer.canEnterSleep() );

    // Setting -> false
    timer.activate();
    assert( timer.getState() == TimerState::Setting );
    assert( !timer.canEnterSleep() );

    // Running -> false
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );
    assert( !timer.canEnterSleep() );

    // Paused -> false
    timer.onActionButton();
    assert( timer.getState() == TimerState::Paused );
    assert( !timer.canEnterSleep() );

    // Resume -> Running
    timer.onActionButton();
    assert( timer.getState() == TimerState::Running );

    // Reaching 0 -> Alarming -> false
    advanceMockMillis( 30000 );
    timer.update();
    assert( timer.getState() == TimerState::Alarming );
    assert( !timer.canEnterSleep() );

    // Dismiss alarm -> Idle -> true
    timer.onActionButton();
    assert( timer.getState() == TimerState::Idle );
    assert( timer.canEnterSleep() );

    std::cout << "PASS: test_timer_can_enter_sleep\n";
}

void test_pot_movement_resets_inactivity_timeout()
{
    setMockMillis( 1000 );
    DisplayManager display;
    display.setup();
    Potentiometer leftPot( 1, 1.0f, 0 );
    leftPot.updateRaw( 2048 );
    Buzzer buzzer( 5 );
    MotionSensor motion;
    ProximitySensor proximity;

    TimerController timer( display, leftPot, buzzer, motion, proximity );
    timer.init();
    timer.activate();
    assert( timer.getState() == TimerState::Setting );

    // Advance 14s (close to the 15s timeout)
    advanceMockMillis( 14000 );
    timer.update();
    assert( timer.getState() == TimerState::Setting );

    // Move pot -> resets state timer!
    timer.onPotMoved();

    // Advance 10s after the pot movement (24s total since activate)
    advanceMockMillis( 10000 );
    timer.update();
    // Must STILL be in Setting state (guaranteed at least 10 seconds of grace period)
    assert( timer.getState() == TimerState::Setting );

    // Advance remaining 5.1s (15.1s after pot movement) -> now times out to Idle
    advanceMockMillis( 5100 );
    timer.update();
    assert( timer.getState() == TimerState::Idle );

    std::cout << "PASS: test_pot_movement_resets_inactivity_timeout\n";
}

int main()
{
    std::cout << "Running TimerController Native Tests...\n";
    test_initial_state_idle();
    test_activation_and_inactivity_timeout();
    test_start_pause_resume_flow();
    test_long_press_cancel();
    test_power_policy_dim_sleep_and_wake();
    test_auto_wake_at_30s_mark();
    test_alarm_dismissal_slap_and_button();
    test_alarm_auto_silence_after_2_minutes();
    test_visual_modes_and_physics();
    test_pot_movement_behavior();
    test_paused_long_press_and_multi_pause();
    test_proximity_threshold_wake();
    test_start_with_duration_and_trigger_alarm();
    test_timer_can_enter_sleep();
    test_pot_movement_resets_inactivity_timeout();
    std::cout << "✅ ALL TIMER CONTROLLER TESTS PASSED!\n";
    return 0;
}
