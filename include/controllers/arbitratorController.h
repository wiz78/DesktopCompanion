//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_ARBITRATORCONTROLLER_H
#define FRIENDLYBOT_CONTROLLERS_ARBITRATORCONTROLLER_H

#include "display.h"
#include "drivers/motionSensor.h"
#include "drivers/potentiometer.h"
#include "i18n.h"
#include "stateMachine.h"

#include <cstddef>
#include <cstdint>

enum class ArbitratorState : uint8_t
{
    Idle = 0,
    CategoryBrowsing,
    CategoryLocked,
    Spinning,
    Verdict
};

class ArbitratorController : public StateMachine<ArbitratorController, ArbitratorState>
{
public:
    using StateMachine<ArbitratorController, ArbitratorState>::getState;

    static constexpr uint8_t NUM_CATEGORIES = 4;
    static constexpr uint32_t CATEGORY_LOCK_DELAY_MS = 1000;
    static constexpr uint32_t SPIN_DURATION_MS = 2500;
    static constexpr uint32_t INACTIVITY_TIMEOUT_MS = 15000;
    static constexpr uint32_t BLINK_INTERVAL_MS = 500;

    ArbitratorController(
        DisplayManager& display,
        Potentiometer& rightPot,
        MotionSensor& motion
    );

    void init();
    void update( uint32_t nowMs = 0 );

    // External inputs
    void activate();
    void onPotMoved();
    void onActionButton();
    void onShake();

    [[nodiscard]] bool isActive() const
    {
        return getState() != ArbitratorState::Idle;
    }

    [[nodiscard]] bool canEnterSleep() const
    {
        return getState() == ArbitratorState::Idle;
    }

    [[nodiscard]] uint8_t getSelectedCategory() const
    {
        return selectedCategory;
    }

    [[nodiscard]] size_t getVerdictIndex() const
    {
        return verdictIndex;
    }

    [[nodiscard]] size_t getLastVerdictIndex() const
    {
        return lastVerdictIndex;
    }

private:
    DisplayManager& display;
    Potentiometer& rightPot;
    MotionSensor& motion;

    uint8_t selectedCategory = 0;
    uint32_t lastPotMoveTime = 0;

    // Spin animation & verdict
    size_t verdictIndex = 0;
    size_t lastVerdictIndex = 255;
    uint32_t lastReelStepTime = 0;
    uint32_t lastBlinkPhase = 255;
    bool displayInverted = false;

    // State machine handlers
    void idleHandler();
    void categoryBrowsingHandler();
    void categoryLockedHandler();
    void spinningHandler();
    void verdictHandler();

    // Input & Potentiometer helpers
    uint8_t readCategoryFromPot();

    // Rendering methods
    void renderDrumPicker();
    void renderLockedCategory();
    void renderReelFrame( float progress );
    void renderVerdictFrame();

    void pickRandomVerdict();
};

#endif // FRIENDLYBOT_CONTROLLERS_ARBITRATORCONTROLLER_H
