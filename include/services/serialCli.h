//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_SERIALCLI_H
#define FRIENDLYBOT_SERVICES_SERIALCLI_H

#include "controllers/arbitratorController.h"
#include "controllers/deskCompanionController.h"
#include "controllers/gagController.h"
#include "controllers/systemOrchestrator.h"
#include "controllers/timerController.h"
#include "hardwareManager.h"
#include "sentenceEngine.h"
#include "services/networkWorker.h"
#include "services/powerManager.h"
#include "services/timeService.h"
#include "services/weatherService.h"

#include <cstdint>

#include <Arduino.h>

class SerialCli
{
public:
    static constexpr uint32_t HW_TEST_DURATION_MS = 5000;
    static constexpr uint32_t TELEMETRY_INTERVAL_MS = 100;

    void init(
        SystemOrchestrator *orchestrator,
        HardwareManager *hal,
        NetworkWorker *net,
        TimeService *time,
        WeatherService *weather,
        SentenceEngine *sentences,
        PowerManager *power,
        DeskCompanionController *companion = nullptr,
        GagController *gag = nullptr,
        ArbitratorController *arbitrator = nullptr,
        TimerController *timer = nullptr
    );

    void update( uint32_t nowMs );
    void processCommand( const String& rawCmd, uint32_t nowMs );
    [[nodiscard]] bool isHwTestStreaming() const;
    void stopHwTest();

    void setRtcStats( const uint32_t *wakeCount, const time_t *lastWifiAttemptEpoch )
    {
        rtcWakeCountPtr = wakeCount;
        rtcLastWifiAttemptEpochPtr = lastWifiAttemptEpoch;
    }

private:
    SystemOrchestrator *orchestrator = nullptr;
    HardwareManager *hal = nullptr;
    NetworkWorker *net = nullptr;
    TimeService *time = nullptr;
    WeatherService *weather = nullptr;
    SentenceEngine *sentences = nullptr;
    PowerManager *power = nullptr;
    DeskCompanionController *companion = nullptr;
    GagController *gag = nullptr;
    ArbitratorController *arbitrator = nullptr;
    TimerController *timer = nullptr;

    const uint32_t *rtcWakeCountPtr = nullptr;
    const time_t *rtcLastWifiAttemptEpochPtr = nullptr;

    bool hwTestStreaming = false;
    uint32_t streamStartMs = 0;
    uint32_t lastTelemetryMs = 0;

    void printHelp();
    void emitTelemetryLine();
};

#endif // FRIENDLYBOT_SERVICES_SERIALCLI_H
