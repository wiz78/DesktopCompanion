//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_BUZZER_H
#define FRIENDLYBOT_DRIVERS_BUZZER_H

#include <array>
#include <cstddef>
#include <cstdint>

class Buzzer
{
public:
    explicit Buzzer( uint8_t pin );

    void setup();
    void update( uint32_t nowMs );

    void beep( uint16_t durationMs, uint32_t nowMs = 0 );
    void doubleBeep( uint16_t durationMs = 50, uint16_t gapMs = 50, uint32_t nowMs = 0 );
    void playAlarmPattern( uint32_t nowMs = 0 );
    void stop();

    [[nodiscard]] uint8_t getPin() const
    {
        return pin;
    }

    [[nodiscard]] bool isSounding() const
    {
        return sounding;
    }

private:
    static constexpr size_t MAX_STEPS = 8;

    uint8_t pin;
    bool sounding = false;
    bool repeating = false;

    std::array<uint16_t, MAX_STEPS> stepDurations{};
    std::array<bool, MAX_STEPS> stepStates{};
    size_t totalSteps = 0;
    size_t currentStep = 0;
    uint32_t stepStartTime = 0;

    void setOutput( bool on );
};

#endif // FRIENDLYBOT_DRIVERS_BUZZER_H
