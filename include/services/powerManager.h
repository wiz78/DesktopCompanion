//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_POWERMANAGER_H
#define FRIENDLYBOT_SERVICES_POWERMANAGER_H

#include "display.h"
#include "drivers/buzzer.h"
#include "drivers/motionSensor.h"
#include "drivers/powerSense.h"
#include "services/networkWorker.h"

#include <cstdint>
#include <functional>
#include <vector>

class PowerManager
{
public:
    static constexpr uint32_t SLEEP_INACTIVITY_MS = 30000;
    static constexpr uint32_t WIFI_SYNC_INTERVAL_SEC = 1800; // 30 minutes

    using CanSleepCallback = std::function<bool()>;

    static PowerManager& instance();

    PowerManager();

    void init(
        PowerSense& powerSense,
        DisplayManager& display,
        Buzzer& buzzer,
        NetworkWorker& netWorker
    );

    void registerSleepGate( CanSleepCallback gate );
    void clearSleepGates();

    void update( uint32_t nowMs = 0 );
    void resetActivity( uint32_t nowMs = 0 );

    [[nodiscard]] bool canEnterSleep() const;
    [[nodiscard]] bool shouldEnterSleep( uint32_t nowMs = 0 ) const;
    [[nodiscard]] bool shouldSyncWifiOnWake( uint32_t nowEpoch, uint32_t lastSyncEpoch ) const;

    void enterDeepSleep( MotionSensor& motion );

    [[nodiscard]] uint32_t getLastActivityMs() const
    {
        return lastActivityMs;
    }

private:
    PowerSense *powerSense = nullptr;
    DisplayManager *display = nullptr;
    Buzzer *buzzer = nullptr;
    NetworkWorker *netWorker = nullptr;

    uint32_t lastActivityMs = 0;
    std::vector<CanSleepCallback> sleepGates;
};

#endif // FRIENDLYBOT_SERVICES_POWERMANAGER_H
