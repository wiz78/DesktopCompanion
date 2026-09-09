//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_GAGCONTROLLER_H
#define FRIENDLYBOT_CONTROLLERS_GAGCONTROLLER_H

#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/ldrSensor.h"
#include "drivers/motionSensor.h"
#include "drivers/powerSense.h"
#include "i18n.h"
#include "stateMachine.h"

#include <cstdint>

enum class GagState : uint8_t
{
    Idle = 0,
    Glitching,
    RejectingButton,
    RepairingAnimation,
    RepairedSuccess
};

class GagController : public StateMachine<GagController, GagState>
{
public:
    using StateMachine<GagController, GagState>::getState;

    static constexpr uint32_t GLITCH_TIMEOUT_MS = 30000;
    static constexpr uint32_t REJECT_DURATION_MS = 1500;
    static constexpr uint32_t FLASH_DURATION_MS = 120;
    static constexpr uint32_t COLLAPSE_DURATION_MS = 280;
    static constexpr uint32_t SUCCESS_DURATION_MS = 2500;
    static constexpr uint32_t MIN_COOLDOWN_MS = 10800000; // 3 hours
    static constexpr uint8_t MAX_DAILY_BUDGET = 2;

    GagController(
        DisplayManager& display,
        Buzzer& buzzer,
        MotionSensor& motion,
        LdrSensor& ldr,
        PowerSense& powerSense
    );

    void init();
    void update( uint32_t nowMs = 0 );

    // External triggers & events
    bool trigger( uint32_t nowMs = 0, bool force = false );
    void onActionButton( uint32_t nowMs = 0 );
    void onSlap( uint32_t nowMs = 0 );

    [[nodiscard]] bool isActive() const
    {
        return getState() != GagState::Idle;
    }

    [[nodiscard]] bool canEnterSleep() const
    {
        return getState() == GagState::Idle;
    }

    [[nodiscard]] uint8_t getTriggerCount() const
    {
        return triggerCount;
    }

    [[nodiscard]] bool canTrigger( uint32_t nowMs = 0 ) const;

private:
    void idleHandler();
    void glitchingHandler();
    void rejectingButtonHandler();
    void repairingAnimationHandler();
    void repairedSuccessHandler();

    DisplayManager& display;
    Buzzer& buzzer;
    MotionSensor& motion;
    LdrSensor& ldr;
    PowerSense& powerSense;

    uint32_t lastTriggerTime = 0;
    uint8_t triggerCount = 0;
    uint32_t noiseSeed = 0x12345678;

    void renderGlitchFrame();
    void renderRejectFrame();
    void renderRepairFrame();
    void renderSuccessFrame();
};

#endif // FRIENDLYBOT_CONTROLLERS_GAGCONTROLLER_H
