//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_CONTROLLERS_TIMERPRESETS_H
#define FRIENDLYBOT_CONTROLLERS_TIMERPRESETS_H

#include <algorithm>
#include <array>
#include <cstdint>

class TimerPresets
{
public:
    static constexpr uint8_t NUM_PRESETS = 26;

    static constexpr std::array<uint16_t, NUM_PRESETS> PRESETS_SECONDS = {
        // 0:30 to 5:00 in 30s steps (10 steps)
        30, 60, 90, 120, 150, 180, 210, 240, 270, 300,
        // 5:00 to 15:00 in 1m steps (10 steps)
        360, 420, 480, 540, 600, 660, 720, 780, 840, 900,
        // 15:00 to 30:00 in 2.5m steps (6 steps)
        1050, 1200, 1350, 1500, 1650, 1800
    };

    [[nodiscard]] static constexpr uint16_t getDurationSeconds( const uint8_t stepIndex )
    {
        const uint8_t idx = std::min( stepIndex, static_cast<uint8_t>( NUM_PRESETS - 1 ) );
        return PRESETS_SECONDS[ idx ];
    }

    [[nodiscard]] static constexpr uint8_t getStepIndexForNormalized( const float norm )
    {
        if( norm <= 0.0f )
        {
            return 0;
        }
        if( norm >= 1.0f )
        {
            return NUM_PRESETS - 1;
        }
        const auto step = static_cast<uint8_t>( norm * static_cast<float>( NUM_PRESETS ) );
        return std::min( step, static_cast<uint8_t>( NUM_PRESETS - 1 ) );
    }
};

#endif // FRIENDLYBOT_CONTROLLERS_TIMERPRESETS_H
