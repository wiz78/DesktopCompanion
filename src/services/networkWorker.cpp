//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/networkWorker.h"
#include "services/configServer.h"
#include "x509_crt_bundle.h"

#include <algorithm>
#include <string>

#ifdef ARDUINO
#include <Arduino.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#endif

NetworkWorker::NetworkWorker() = default;

void NetworkWorker::begin( TimeService *timeSvc, WeatherService *weatherSvc, SentenceEngine *sentences )
{
    timeService = timeSvc;
    weatherService = weatherSvc;
    sentenceEngine = sentences;

#ifdef ARDUINO
    Preferences prefs; prefs.begin( NVS_NAMESPACE, true ); cachedSsid = prefs.getString( "ssid", "" ).c_str();
    cachedPass = prefs.getString( "pass", "" ).c_str();
    cachedCoordsConfigured = prefs.isKey( "latitude" ) && prefs.isKey( "longitude" );
    cachedLat = prefs.getDouble( "latitude", 0.0 ); cachedLon = prefs.getDouble( "longitude", 0.0 );
    cachedApiKey = prefs.getString( "apikey", "" ).c_str();
    cachedTz = prefs.getString( "tz", TimeService::DEFAULT_TIMEZONE ).c_str();
    cachedHostname = prefs.getString( "hostname", "bot50" ).c_str();
    cachedTimerMode = prefs.getUChar( "timer_mode", 0 ); configured = !cachedSsid.empty(); prefs.end(); if(
        timeService != nullptr ) {
        timeService->init( cachedTz );
    } ConfigServer::instance().begin( sentenceEngine, this ); xTaskCreatePinnedToCore(
        NetworkWorker::taskEntry, "NetWorker", 16384, this, 1, nullptr, 0 // Core 0
    );
#else
    cachedTz = TimeService::DEFAULT_TIMEZONE;
    cachedHostname = "bot50";
    cachedTimerMode = 0;
    if( timeService != nullptr ) {
        timeService->init( cachedTz );
    }
    ConfigServer::instance().begin( sentenceEngine, this );
#endif
}

#ifdef ARDUINO
void NetworkWorker::taskEntry( void *param )
{
    auto *self = static_cast<NetworkWorker *>(param);
    self->runTask();
}

void NetworkWorker::runTask()
{
    uint32_t lastSyncMs = 0;
    uint32_t lastWifiAttemptMs = 0;
    uint32_t wifiBackoffMs = 5000;
    bool mdnsStarted = false;

    for( ;; ) {
        if( reconnectRequested.exchange( false ) ) {
            WiFi.disconnect( true );
            lastWifiAttemptMs = 0;
            wifiBackoffMs = 5000;
            mdnsStarted = false;
        }

        if( currentPowerProfile.load() == PowerProfile::Battery ) {
            if( ConfigServer::instance().isApActive() ) {
                ConfigServer::instance().stopAp();
            }

            if( batterySyncRequested.exchange( false ) && isConfigured() ) {
                performBatterySync();
                lastSyncMs = millis();
            } else {
                if( WiFi.getMode() != WIFI_OFF ) {
                    WiFi.disconnect( true );
                    WiFi.mode( WIFI_OFF );
                    mdnsStarted = false;
                }
            }

            vTaskDelay( pdMS_TO_TICKS( 100 ) );
            continue;
        }

        ConfigServer::instance().handleClient();

        if( !isConfigured() ) {
            if( WiFi.status() == WL_CONNECTED ) {
                WiFi.disconnect( true );
                mdnsStarted = false;
            }
            if( !ConfigServer::instance().isApActive() ) {
                ConfigServer::instance().startAp( "Console50-Setup" );
            }
            vTaskDelay( pdMS_TO_TICKS( 10 ) );
            continue;
        }

        const uint32_t nowMs = millis();

        if( WiFi.status() != WL_CONNECTED ) {
            mdnsStarted = false;
            if( !ConfigServer::instance().isApActive() ) {
                ConfigServer::instance().startAp( "Console50-Setup" );
            }

            if( ( lastWifiAttemptMs == 0 ) || ( nowMs - lastWifiAttemptMs >= wifiBackoffMs ) ) {
                lastWifiAttemptMs = nowMs;
                const bool connected = connectWifi();
                if( connected ) {
                    wifiBackoffMs = 5000;
                } else {
                    wifiBackoffMs = std::min<uint32_t>( wifiBackoffMs * 2, 30000 );
                }
            }
        } else {
            wifiBackoffMs = 5000;
            if( ConfigServer::instance().isApActive() ) {
                ConfigServer::instance().stopAp();
            }

            if( !mdnsStarted ) {
                std::string hostname; {
                    std::lock_guard lock( netMutex );
                    hostname = cachedHostname;
                }
                WiFi.setHostname( hostname.c_str() );
                if( MDNS.begin( hostname.c_str() ) ) {
                    MDNS.addService( "http", "tcp", 80 );
                    mdnsStarted = true;
                }
            }
        }

        const bool shouldSync = ( lastSyncMs == 0 ) || immediateFetchRequested.load() || batterySyncRequested.
                                exchange( false ) || ( nowMs - lastSyncMs >= SYNC_INTERVAL_MS );

        if( WiFi.status() == WL_CONNECTED && shouldSync ) {
            immediateFetchRequested = false;
            const bool fetchOk = fetchWeatherHttp();
            const uint32_t syncCompletedMs = millis();

            if( fetchOk ) {
                lastSyncMs = syncCompletedMs;
            } else {
                lastSyncMs = syncCompletedMs - SYNC_INTERVAL_MS + 30000;
            }

            if( timeService != nullptr && time( nullptr ) > 1700000000 ) {
                timeService->markSynced( true );
            }
        }

        if( timeService != nullptr && !timeService->isTimeSynced() && time( nullptr ) > 1700000000 ) {
            timeService->markSynced( true );
        }

        vTaskDelay( pdMS_TO_TICKS( 10 ) );
    }
}

bool NetworkWorker::connectWifi()
{
    std::string ssid;
    std::string pass; {
        std::lock_guard lock( netMutex );
        if( cachedSsid.empty() ) {
            return false;
        }
        ssid = cachedSsid;
        pass = cachedPass;
    }

    if( ConfigServer::instance().isApActive() ) {
        WiFi.mode( WIFI_AP_STA );
    } else {
        WiFi.mode( WIFI_STA );
    }
    WiFi.begin( ssid.c_str(), pass.c_str() );

    int retries = 0;
    while( WiFi.status() != WL_CONNECTED && retries < 20 ) {
        ConfigServer::instance().handleClient();
        vTaskDelay( pdMS_TO_TICKS( 500 ) );
        retries++;
    }

    return ( WiFi.status() == WL_CONNECTED );
}

bool NetworkWorker::fetchWeatherHttp()
{
    if( weatherService == nullptr ) {
        return false;
    }

    double lat = 0.0;
    double lon = 0.0;
    std::string apiKey; {
        std::lock_guard lock( netMutex );
        if( ( !cachedCoordsConfigured && cachedLat == 0.0 && cachedLon == 0.0 ) || cachedApiKey.empty() ) {
            return false;
        }
        lat = cachedLat;
        lon = cachedLon;
        apiKey = cachedApiKey;
    }

    char urlBuf[ 256 ];
    std::snprintf( urlBuf, sizeof( urlBuf ),
                   "https://api.openweathermap.org/data/2.5/weather?lat=%.4f&lon=%.4f&appid=%s&units=metric&lang=%s",
                   lat, lon, apiKey.c_str(), I18n::instance().getLanguageCode() );
    const String url( urlBuf );

    WiFiClientSecure tlsClient;
    HTTPClient http;

    tlsClient.setCACertBundle( x509_crt_bundle );

    http.begin( tlsClient, url );
    http.setTimeout( 5000 );

    const int httpCode = http.GET();
    bool success = false;

    if( httpCode == HTTP_CODE_OK ) {
        const String payload = http.getString();
        WeatherData w;
        if( WeatherService::parseWeatherJson( payload.c_str(), w ) ) {
            w.lastFetchTimeMs = millis();
            weatherService->updateWeather( w );
            success = true;
        }
    }
    http.end();
    return success;
}

void NetworkWorker::performBatterySync()
{
    std::string ssid;
    std::string pass; {
        std::lock_guard lock( netMutex );
        if( cachedSsid.empty() ) {
            return;
        }
        ssid = cachedSsid;
        pass = cachedPass;
    }

    WiFi.mode( WIFI_STA );
    WiFi.begin( ssid.c_str(), pass.c_str() );

    const uint32_t startMs = millis();
    while( WiFi.status() != WL_CONNECTED && ( millis() - startMs < BATTERY_CONNECT_TIMEOUT_MS ) ) {
        vTaskDelay( pdMS_TO_TICKS( 100 ) );
    }

    if( WiFi.status() == WL_CONNECTED ) {
        fetchWeatherHttp();
        if( timeService != nullptr && time( nullptr ) > 1700000000 ) {
            timeService->markSynced( true );
        }
    }

    WiFi.disconnect( true );
    WiFi.mode( WIFI_OFF );
}
#endif

void NetworkWorker::setPowerProfile( const PowerProfile profile )
{
    if( currentPowerProfile.load() == profile ) {
        return;
    }

    const PowerProfile prev = currentPowerProfile.exchange( profile );
    if( profile == PowerProfile::Battery ) {
        ConfigServer::instance().stopAp();
#ifdef ARDUINO
        WiFi.disconnect( true ); WiFi.mode( WIFI_OFF );
#endif
    } else if( profile == PowerProfile::Usb && prev == PowerProfile::Battery ) {
        reconnectRequested = true;
    }
}

PowerProfile NetworkWorker::getPowerProfile() const
{
    return currentPowerProfile.load();
}

void NetworkWorker::triggerBatterySync()
{
    batterySyncRequested = true;
#ifndef ARDUINO
    if( currentPowerProfile.load() == PowerProfile::Battery ) {
        batterySyncRequested = false;
        if( timeService != nullptr ) {
            timeService->markSynced( true );
        }
    } else {
        immediateFetchRequested = true;
    }
#endif
}

bool NetworkWorker::isConfigured() const
{
    return configured;
}

bool NetworkWorker::isConnected() const
{
#ifdef ARDUINO
    return WiFi.status() == WL_CONNECTED;
#else
    return false;
#endif
}

void NetworkWorker::requestImmediateFetch()
{
    immediateFetchRequested = true;
}

void NetworkWorker::saveWifiCredentials( const std::string& ssid, const std::string& pass )
{
    std::lock_guard lock( netMutex );
    cachedSsid = ssid;
    cachedPass = pass;
    configured = !cachedSsid.empty();
    reconnectRequested = true;

#ifdef ARDUINO
    Preferences prefs;

    prefs.begin( NVS_NAMESPACE, false );
    prefs.putString( "ssid", ssid.c_str() );
    prefs.putString( "pass", pass.c_str() );
    prefs.end();

    requestImmediateFetch();
#endif
}

void NetworkWorker::clearWifiCredentials()
{
    std::lock_guard lock( netMutex );
    cachedSsid.clear();
    cachedPass.clear();
    configured = false;
    reconnectRequested = true;
#ifdef ARDUINO
    Preferences prefs;

    prefs.begin( NVS_NAMESPACE, false );
    prefs.remove( "ssid" );
    prefs.remove( "pass" );
    prefs.end();

    if( WiFi.status() == WL_CONNECTED ) {
        WiFi.disconnect( true );
    }
#endif
}

void NetworkWorker::saveWeatherConfig( const double lat, const double lon, const std::string& apikey )
{
    std::lock_guard lock( netMutex );

    cachedLat = lat;
    cachedLon = lon;
    cachedCoordsConfigured = true;

    if( !apikey.empty() ) {
        cachedApiKey = apikey;
    }

#ifdef ARDUINO
    Preferences prefs;

    prefs.begin( NVS_NAMESPACE, false );
    prefs.putDouble( "latitude", lat );
    prefs.putDouble( "longitude", lon );
    if( !apikey.empty() ) {
        prefs.putString( "apikey", apikey.c_str() );
    }
    prefs.end();
    requestImmediateFetch();
#endif
}

void NetworkWorker::saveTimezone( const std::string& posixTz )
{
    std::lock_guard lock( netMutex );
    cachedTz = posixTz;
#ifdef ARDUINO
    Preferences prefs;

    prefs.begin( NVS_NAMESPACE, false );
    prefs.putString( "tz", posixTz.c_str() );
    prefs.end();

    if(
        timeService != nullptr ) {
        timeService->setTimezone( posixTz );
    }
#endif
}

void NetworkWorker::saveHostname( const std::string& name )
{
    std::lock_guard lock( netMutex );
    cachedHostname = name;
#ifdef ARDUINO
    Preferences prefs; prefs.begin( NVS_NAMESPACE, false ); prefs.putString( "hostname", name.c_str() ); prefs.end();
#endif
}

std::string NetworkWorker::getHostname() const
{
    std::lock_guard lock( netMutex );
    return cachedHostname;
}

void NetworkWorker::saveTimerMode( const uint8_t mode )
{
    cachedTimerMode = mode;
#ifdef ARDUINO
    Preferences prefs; prefs.begin( NVS_NAMESPACE, false ); prefs.putUChar( "timer_mode", mode ); prefs.end();
#endif
}

uint8_t NetworkWorker::getTimerMode() const
{
    return cachedTimerMode.load();
}

std::string NetworkWorker::getSsid() const
{
    std::lock_guard lock( netMutex );
    return cachedSsid;
}

double NetworkWorker::getLatitude() const
{
    std::lock_guard lock( netMutex );
    return cachedLat;
}

double NetworkWorker::getLongitude() const
{
    std::lock_guard lock( netMutex );
    return cachedLon;
}

bool NetworkWorker::hasCoordinates() const
{
    std::lock_guard lock( netMutex );
    return cachedCoordsConfigured || ( cachedLat != 0.0 || cachedLon != 0.0 );
}

std::string NetworkWorker::getApiKey() const
{
    std::lock_guard lock( netMutex );
    return cachedApiKey;
}
