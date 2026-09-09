//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DRIVERS_POWERSENSE_H
#define FRIENDLYBOT_DRIVERS_POWERSENSE_H

#include <cstdint>

class PowerSense
{
public:
    static constexpr uint32_t DEBOUNCE_MS = 100;

    explicit PowerSense( uint8_t pin );

    void setup();
    void update( uint32_t nowMs = 0 );
    void updateRaw( bool rawPin, uint32_t nowMs = 0 );

    void setInverted( const bool invert )
    {
        inverted = invert;
    }
    [[nodiscard]] bool isInverted() const
    {
        return inverted;
    }
    [[nodiscard]] uint8_t getPin() const
    {
        return pin;
    }
    [[nodiscard]] bool isUsbPowered() const
    {
        return usbPowered;
    }
    [[nodiscard]] bool wasPowerSourceChanged();

private:
    uint8_t pin;
    bool usbPowered = true;
    bool candidateState = true;
    uint32_t candidateChangeTimeMs = 0;
    bool stateChanged = false;
    bool initialized = false;
    bool inverted = false;
};

#endif // FRIENDLYBOT_DRIVERS_POWERSENSE_H
