//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITSBARRENDERER_H
#define FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITSBARRENDERER_H

#include "controllers/renderers/timerRenderer.h"

class DigitsBarRenderer : public ITimerRenderer
{
public:
    void renderSetting( DisplayManager& display, uint16_t targetSeconds ) override;
    void renderRunning( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds ) override;
    void renderPaused( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds, bool blinkPhase ) override;
    void renderAlarming( DisplayManager& display, bool invertPhase ) override;

private:
    static void drawTimeDigits( DisplayManager& display, uint16_t seconds );
    static void drawProgressBar( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds );
    static void drawHeader( DisplayManager& display, const char *headerText );
};

#endif // FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITSBARRENDERER_H
