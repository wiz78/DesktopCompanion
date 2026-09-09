#include "services/weatherService.h"

#include <cassert>
#include <cmath>
#include <iostream>

void testValidWeatherJson()
{
    const char *sampleJson = R"({
        "coord": {"lon": 9.1895, "lat": 45.4643},
        "weather": [{"id": 800, "main": "Clear", "description": "clear sky", "icon": "01d"}],
        "main": {"temp": 21.4, "feels_like": 21.1, "temp_min": 19.8, "temp_max": 22.5, "pressure": 1015, "humidity": 52},
        "name": "Milan"
    })";

    WeatherData data;
    const bool ok = WeatherService::parseWeatherJson( sampleJson, data );
    assert( ok );
    assert( data.valid );
    assert( std::fabs( data.tempC - 21.4f ) < 0.01f );
    assert( data.humidity == 52 );
    assert( data.condition == "Clear" );
    std::cout << "[PASS] testValidWeatherJson\n";
}

void testSubzeroTemperature()
{
    const char *subzeroJson = R"({
        "weather": [{"id": 600, "main": "Snow", "description": "light snow"}],
        "main": {"temp": -4.8, "humidity": 88}
    })";

    WeatherData data;
    const bool ok = WeatherService::parseWeatherJson( subzeroJson, data );
    assert( ok );
    assert( data.valid );
    assert( std::fabs( data.tempC - (-4.8f) ) < 0.01f );
    assert( data.humidity == 88 );
    assert( data.condition == "Snow" );
    std::cout << "[PASS] testSubzeroTemperature\n";
}

void testMalformedJson()
{
    WeatherData data;
    assert( !WeatherService::parseWeatherJson( "{ truncated json...", data ) );
    assert( !data.valid );

    assert( !WeatherService::parseWeatherJson( "", data ) );
    assert( !data.valid );

    // Missing main object
    assert( !WeatherService::parseWeatherJson( "{\"weather\": []}", data ) );
    assert( !data.valid );
    std::cout << "[PASS] testMalformedJson\n";
}

void testWeatherServiceStore()
{
    WeatherService service;
    assert( !service.getLatestWeather().valid );

    WeatherData sample;
    sample.tempC = 18.0f;
    sample.humidity = 60;
    sample.condition = "Clouds";
    sample.valid = true;
    sample.lastFetchTimeMs = 12345;

    service.updateWeather( sample );
    const auto latest = service.getLatestWeather();
    assert( latest.valid );
    assert( std::fabs( latest.tempC - 18.0f ) < 0.01f );
    assert( latest.condition == "Clouds" );
    std::cout << "[PASS] testWeatherServiceStore\n";
}

int main()
{
    std::cout << "Running WeatherParser unit tests...\n";
    testValidWeatherJson();
    testSubzeroTemperature();
    testMalformedJson();
    testWeatherServiceStore();
    std::cout << "All WeatherParser tests passed!\n";
    return 0;
}
