//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_WEATHERSERVICE_H
#define FRIENDLYBOT_SERVICES_WEATHERSERVICE_H

#include <cstdint>
#include <mutex>
#include <string>

struct WeatherData
{
    float tempC = 0.0f;
    uint8_t humidity = 0;
    std::string condition;
    bool valid = false;
    uint32_t lastFetchTimeMs = 0;
};

class WeatherService
{
public:
    WeatherService();

    [[nodiscard]] static bool parseWeatherJson( const char *jsonPayload, WeatherData& outData );

    [[nodiscard]] WeatherData getLatestWeather() const;
    void updateWeather( const WeatherData& data );

private:
    WeatherData currentWeather;
    mutable std::mutex mutex;
};

#endif // FRIENDLYBOT_SERVICES_WEATHERSERVICE_H
