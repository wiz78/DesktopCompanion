//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_TIMERCONTROLLER_H
#define FRIENDLYBOT_CONTROLLERS_TIMERCONTROLLER_H

#include "controllers/renderers/digitalSandRenderer.h"
#include "controllers/renderers/digitsBarRenderer.h"
#include "controllers/renderers/timerRenderer.h"
#include "controllers/timerPresets.h"
#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/motionSensor.h"
#include "drivers/potentiometer.h"
#include "drivers/proximitySensor.h"
#include "stateMachine.h"

#include <cstdint>
#include <memory>

enum class TimerState : uint8_t
{
    Idle = 0,
    Setting,
    Running,
    Paused,
    Alarming
};

class TimerController : public StateMachine<TimerController, TimerState>
{
public:
    using StateMachine<TimerController, TimerState>::getState;

    static constexpr uint32_t SETTING_INACTIVITY_TIMEOUT_MS = 15000;
    static constexpr uint32_t DIM_TIMEOUT_MS = 20000;
    static constexpr uint32_t SLEEP_TIMEOUT_MS = 30000;
    static constexpr uint32_t ALARM_AUTO_SILENCE_MS = 120000;
    static constexpr uint16_t AUTO_WAKE_SECONDS_THRESHOLD = 30;
    static constexpr uint16_t CLOSE_PROXIMITY_MM = 500;

    TimerController(
        DisplayManager& display,
        Potentiometer& leftPot,
        Buzzer& buzzer,
        MotionSensor& motion,
        ProximitySensor& proximity
    );

    void init();
    void update( uint32_t nowMs = 0 );

    // External inputs
    void activate();
    void onPotMoved();
    void onActionButton();
    void onActionButtonLongPress();
    void onSlap();
    void onProximityWake();

    void startWithDuration( uint16_t seconds );
    void triggerAlarm();

    void setVisualMode( TimerVisualMode mode );
    [[nodiscard]] TimerVisualMode getVisualMode() const
    {
        return visualMode;
    }

    [[nodiscard]] bool isActive() const
    {
        return getState() != TimerState::Idle;
    }

    [[nodiscard]] bool canEnterSleep() const
    {
        return getState() == TimerState::Idle;
    }

    [[nodiscard]] bool isAlarming() const
    {
        return getState() == TimerState::Alarming;
    }

    [[nodiscard]] uint16_t getRemainingSeconds() const
    {
        return remainingSeconds;
    }

    [[nodiscard]] uint16_t getTargetSeconds() const
    {
        return targetSeconds;
    }

    [[nodiscard]] bool isDimmed() const
    {
        return dimmed;
    }

private:
    DisplayManager& display;
    Potentiometer& leftPot;
    Buzzer& buzzer;
    MotionSensor& motion;
    ProximitySensor& proximity;

    TimerVisualMode visualMode = TimerVisualMode::DigitsWithBar;
    std::unique_ptr<DigitsBarRenderer> digitsBarRenderer;
    std::unique_ptr<DigitalSandRenderer> digitalSandRenderer;

    uint8_t selectedPresetIndex = 0;
    uint16_t targetSeconds = 30;
    uint16_t remainingSeconds = 30;
    uint32_t countdownStartTimeMs = 0;
    uint32_t pausedAccumulatedMs = 0;
    uint32_t pauseStartMs = 0;
    uint32_t lastInteractionMs = 0;

    bool dimmed = false;
    bool buzzerSilenced = false;
    uint32_t lastBlinkPhase = 255;
    uint32_t lastInvertPhase = 255;

    // State machine handlers
    void idleHandler();
    void settingHandler();
    void runningHandler();
    void pausedHandler();
    void alarmingHandler();

    void resetInteractionTimer( uint32_t nowMs = 0 );
    void wakeDisplay( uint32_t nowMs = 0 );
    [[nodiscard]] ITimerRenderer& getActiveRenderer();
};

#endif // FRIENDLYBOT_CONTROLLERS_TIMERCONTROLLER_H
