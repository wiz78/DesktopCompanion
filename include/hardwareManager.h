//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_HARDWAREMANAGER_H
#define FRIENDLYBOT_HARDWAREMANAGER_H

#include "display.h"
#include "drivers/button.h"
#include "drivers/buzzer.h"
#include "drivers/ldrSensor.h"
#include "drivers/motionSensor.h"
#include "drivers/potentiometer.h"
#include "drivers/powerSense.h"
#include "drivers/proximitySensor.h"
#include "pins.h"

class HardwareManager
{
public:
    static HardwareManager& instance();

    void begin();
    void update();

    [[nodiscard]] bool saveDefaultOrientation();
    bool clearDefaultOrientation();

    [[nodiscard]] DisplayManager& getDisplay()
    {
        return display;
    }
    [[nodiscard]] const DisplayManager& getDisplay() const
    {
        return display;
    }

    [[nodiscard]] Button& getActionButton()
    {
        return actionButton;
    }
    [[nodiscard]] const Button& getActionButton() const
    {
        return actionButton;
    }

    [[nodiscard]] Button& getResetButton()
    {
        return resetButton;
    }
    [[nodiscard]] const Button& getResetButton() const
    {
        return resetButton;
    }

    [[nodiscard]] Potentiometer& getLeftPot()
    {
        return leftPot;
    }
    [[nodiscard]] const Potentiometer& getLeftPot() const
    {
        return leftPot;
    }

    [[nodiscard]] Potentiometer& getRightPot()
    {
        return rightPot;
    }
    [[nodiscard]] const Potentiometer& getRightPot() const
    {
        return rightPot;
    }

    [[nodiscard]] LdrSensor& getLdr()
    {
        return ldr;
    }
    [[nodiscard]] const LdrSensor& getLdr() const
    {
        return ldr;
    }

    [[nodiscard]] PowerSense& getPowerSense()
    {
        return powerSense;
    }
    [[nodiscard]] const PowerSense& getPowerSense() const
    {
        return powerSense;
    }

    [[nodiscard]] Buzzer& getBuzzer()
    {
        return buzzer;
    }
    [[nodiscard]] const Buzzer& getBuzzer() const
    {
        return buzzer;
    }

    [[nodiscard]] MotionSensor& getMotion()
    {
        return motion;
    }
    [[nodiscard]] const MotionSensor& getMotion() const
    {
        return motion;
    }

    [[nodiscard]] ProximitySensor& getProximity()
    {
        return proximity;
    }
    [[nodiscard]] const ProximitySensor& getProximity() const
    {
        return proximity;
    }

    void setInvertLeftPot( const bool invert )
    {
        leftPot.setInverted( invert );
    }
    [[nodiscard]] bool isInvertLeftPot() const
    {
        return leftPot.isInverted();
    }

    void setInvertRightPot( const bool invert )
    {
        rightPot.setInverted( invert );
    }
    [[nodiscard]] bool isInvertRightPot() const
    {
        return rightPot.isInverted();
    }

    void setInvertPowerSense( const bool invert )
    {
        powerSense.setInverted( invert );
    }
    [[nodiscard]] bool isInvertPowerSense() const
    {
        return powerSense.isInverted();
    }

    void setTemperatureOffsetC( const float offset )
    {
        motion.setTemperatureOffsetC( offset );
    }
    [[nodiscard]] float getTemperatureOffsetC() const
    {
        return motion.getTemperatureOffsetC();
    }

    void setNightThreshold( const uint16_t thresh )
    {
        ldr.setNightThreshold( thresh );
    }
    [[nodiscard]] uint16_t getNightThreshold() const
    {
        return ldr.getNightThreshold();
    }

    void setSlapThresholdG( const float threshG )
    {
        motion.setSlapThresholdG( threshG );
    }
    [[nodiscard]] float getSlapThresholdG() const
    {
        return motion.getSlapThresholdG();
    }

private:
    HardwareManager();

    DisplayManager display;
    Button actionButton;
    Button resetButton;
    Potentiometer leftPot;
    Potentiometer rightPot;
    LdrSensor ldr;
    PowerSense powerSense;
    Buzzer buzzer;
    MotionSensor motion;
    ProximitySensor proximity;
};

#endif // FRIENDLYBOT_HARDWAREMANAGER_H
