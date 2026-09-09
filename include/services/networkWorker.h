//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_NETWORKWORKER_H
#define FRIENDLYBOT_SERVICES_NETWORKWORKER_H

#include "services/timeService.h"
#include "services/weatherService.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class SentenceEngine;

enum class PowerProfile : uint8_t
{
    Usb = 0,
    Battery
};

class NetworkWorker
{
public:
    using PowerProfile = ::PowerProfile;
    static constexpr const char *NVS_NAMESPACE = "friendlybot";
    static constexpr uint32_t SYNC_INTERVAL_MS = 3600000; // 1 hour
    static constexpr uint32_t BATTERY_CONNECT_TIMEOUT_MS = 5000;

    static NetworkWorker& instance()
    {
        static NetworkWorker inst;
        return inst;
    }

    void begin( TimeService *timeSvc, WeatherService *weatherSvc, SentenceEngine *sentences = nullptr );

    void setPowerProfile( PowerProfile profile );
    [[nodiscard]] PowerProfile getPowerProfile() const;
    void triggerBatterySync();

    void saveWifiCredentials( const std::string& ssid, const std::string& pass );
    void clearWifiCredentials();
    void saveWeatherConfig( double lat, double lon, const std::string& apikey );
    void saveTimezone( const std::string& posixTz );
    void saveHostname( const std::string& name );
    [[nodiscard]] std::string getHostname() const;
    void saveTimerMode( uint8_t mode );
    [[nodiscard]] uint8_t getTimerMode() const;

    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] bool isConnected() const;
    void requestImmediateFetch();

    [[nodiscard]] std::string getSsid() const;
    [[nodiscard]] double getLatitude() const;
    [[nodiscard]] double getLongitude() const;
    [[nodiscard]] bool hasCoordinates() const;
    [[nodiscard]] std::string getApiKey() const;

private:
    NetworkWorker();

    SentenceEngine *sentenceEngine = nullptr;
    TimeService *timeService = nullptr;
    WeatherService *weatherService = nullptr;

    std::atomic<PowerProfile> currentPowerProfile{ PowerProfile::Usb };
    std::atomic<bool> batterySyncRequested{ false };
    std::atomic<bool> immediateFetchRequested{ false };
    std::atomic<bool> reconnectRequested{ false };

    mutable std::mutex netMutex;
    std::string cachedSsid;
    std::string cachedPass;
    double cachedLat = 0.0;
    double cachedLon = 0.0;
    bool cachedCoordsConfigured = false;
    std::string cachedApiKey;
    std::string cachedTz;
    std::string cachedHostname = "bot50";
    std::atomic<uint8_t> cachedTimerMode{ 0 };
    bool configured = false;

#ifdef ARDUINO
    static void taskEntry( void *param );
    void runTask();
    bool connectWifi();
    bool fetchWeatherHttp();
    void performBatterySync();
#endif
};

#endif // FRIENDLYBOT_SERVICES_NETWORKWORKER_H
