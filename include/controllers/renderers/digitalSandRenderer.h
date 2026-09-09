//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITALSANDRENDERER_H
#define FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITALSANDRENDERER_H

#include "controllers/renderers/timerRenderer.h"

#include <array>
#include <cstddef>
#include <cstdint>

class DigitalSandRenderer : public ITimerRenderer
{
public:
    static constexpr size_t NUM_GRAINS = 64;

    DigitalSandRenderer();

    void reset( uint16_t totalSeconds );
    void updatePhysics( float accelX, float accelY );

    void renderSetting( DisplayManager& display, uint16_t targetSeconds ) override;
    void renderRunning( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds ) override;
    void renderPaused( DisplayManager& display, uint16_t remainingSeconds, uint16_t totalSeconds, bool blinkPhase ) override;
    void renderAlarming( DisplayManager& display, bool invertPhase ) override;

private:
    struct Grain
    {
        int8_t x;
        int8_t y;
        bool dropped;
    };

    std::array<Grain, NUM_GRAINS> grains{};
    size_t droppedCount = 0;
    float gravityX = 0.0f;
    float gravityY = 1.0f;

    void drawHourglassFrame( DisplayManager& display );
    void drawGrains( DisplayManager& display );
};

#endif // FRIENDLYBOT_CONTROLLERS_RENDERERS_DIGITALSANDRENDERER_H
