//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_RENDERERS_TIMERRENDERER_H
#define FRIENDLYBOT_CONTROLLERS_RENDERERS_TIMERRENDERER_H

#include <cstdint>

class DisplayManager;

enum class TimerVisualMode : uint8_t
{
    DigitsWithBar = 0,
    DigitalSand = 1
};

class ITimerRenderer
{
public:
    virtual ~ITimerRenderer() = default;

    virtual void renderSetting( DisplayManager& display, uint16_t targetSeconds ) = 0;
    virtual void renderRunning( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds ) = 0;
    virtual void renderPaused( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds, bool blinkPhase ) = 0;
    virtual void renderAlarming( DisplayManager& display, bool invertPhase ) = 0;
};

#endif // FRIENDLYBOT_CONTROLLERS_RENDERERS_TIMERRENDERER_H
