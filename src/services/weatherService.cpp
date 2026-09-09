//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/weatherService.h"

#include <ArduinoJson.h>

WeatherService::WeatherService() = default;

bool WeatherService::parseWeatherJson( const char *jsonPayload, WeatherData& outData )
{
    outData.valid = false;
    if( jsonPayload == nullptr || jsonPayload[ 0 ] == '\0' ) {
        return false;
    }

    JsonDocument doc;
    if( const DeserializationError err = deserializeJson( doc, jsonPayload ); err != DeserializationError::Ok ) {
        return false;
    }

    if( !doc[ "main" ].is<JsonObject>() || !doc[ "main" ][ "temp" ].is<float>() ) {
        return false;
    }

    outData.tempC = doc[ "main" ][ "temp" ].as<float>();
    outData.humidity = doc[ "main" ][ "humidity" ] | 0;

    if( doc[ "weather" ].is<JsonArray>() && doc[ "weather" ].size() > 0 ) {
        outData.condition = doc[ "weather" ][ 0 ][ "description" ] | doc[ "weather" ][ 0 ][ "main" ] | "Unknown";
    } else {
        outData.condition = "Unknown";
    }

    outData.valid = true;
    return true;
}

WeatherData WeatherService::getLatestWeather() const
{
    std::lock_guard lock( mutex );
    return currentWeather;
}

void WeatherService::updateWeather( const WeatherData& data )
{
    std::lock_guard lock( mutex );
    currentWeather = data;
}
