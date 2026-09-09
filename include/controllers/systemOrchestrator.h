//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_SYSTEMORCHESTRATOR_H
#define FRIENDLYBOT_CONTROLLERS_SYSTEMORCHESTRATOR_H

#include "controllers/arbitratorController.h"
#include "controllers/deskCompanionController.h"
#include "controllers/gagController.h"
#include "controllers/timerController.h"
#include "hardwareManager.h"
#include "sentenceEngine.h"

#include <cstdint>

enum class SystemMode : uint8_t
{
    DeskCompanion,
    SlapGag,
    Arbitrator,
    Timer,
    FactoryReset,
    Diagnostics,
    Transitioning
};

class SystemOrchestrator
{
public:
    static constexpr uint32_t FACTORY_RESET_HOLD_MS = 3000;
    static constexpr uint32_t TRANSITION_FRAME0_MS = 30;
    static constexpr uint32_t TRANSITION_FRAME1_MS = 60;
    static constexpr uint32_t TRANSITION_FRAME2_MS = 85;
    static constexpr float POT_CHANGE_THRESHOLD = 0.02f;
    static constexpr float POT_INTERACTION_THRESHOLD = 0.01f;

    SystemOrchestrator(
        HardwareManager& hal,
        DeskCompanionController& companion,
        GagController& gag,
        ArbitratorController& arbitrator,
        TimerController& timer,
        SentenceEngine& sentenceEngine
    );

    void init( bool enterDiagnosticsAtBoot = false );
    void update( uint32_t nowMs );
    [[nodiscard]] SystemMode getMode() const;
    [[nodiscard]] bool canEnterSleep() const;
    void requestMode( SystemMode targetMode, uint32_t nowMs );

    [[nodiscard]] SystemMode getTargetMode() const
    {
        return targetMode;
    }

    [[nodiscard]] SystemMode getPreviousMode() const
    {
        return previousMode;
    }

private:
    HardwareManager& hal;
    DeskCompanionController& companion;
    GagController& gag;
    ArbitratorController& arbitrator;
    TimerController& timer;
    SentenceEngine& sentenceEngine;

    SystemMode currentMode = SystemMode::DeskCompanion;
    SystemMode targetMode = SystemMode::DeskCompanion;
    SystemMode previousMode = SystemMode::DeskCompanion;

    uint32_t transitionStartMs = 0;
    uint32_t resetPressStartMs = 0;
    uint8_t lastDisplayedRemaining = 0;
    bool resetButtonPressed = false;
    bool diagnosticsAwaitingRelease = false;
    uint32_t diagLastShockMs = 0;
    uint32_t diagLastShakeMs = 0;

    void activateMode( SystemMode mode, uint32_t nowMs );
    void handleFactoryReset( uint32_t nowMs );
    void handleDiagnostics( uint32_t nowMs );
    void handleTransitioning( uint32_t nowMs );
    void handleTimer( uint32_t nowMs );
    void handleSlapGag( uint32_t nowMs );
    void handleArbitrator( uint32_t nowMs );
    void handleDeskCompanion( uint32_t nowMs );
};

#endif // FRIENDLYBOT_CONTROLLERS_SYSTEMORCHESTRATOR_H
