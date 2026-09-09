//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "hardwareManager.h"
#include "services/configServer.h"
#include "services/networkWorker.h"
#include "services/webUi.h"
#include "services/webUtils.h"

#ifdef ARDUINO
#include <Preferences.h>
#include <esp_system.h>
#include <esp_timer.h>
#if __has_include( <LittleFS.h> )
#include <LittleFS.h>
#endif
#endif

ConfigServer &ConfigServer::instance()
{
    static ConfigServer inst;
    return inst;
}

void ConfigServer::begin( SentenceEngine *sentences, NetworkWorker *network )
{
    sentenceEngine = sentences;
    networkWorker = network;

#ifdef ARDUINO
    registerRoutes();
    server.begin();
#endif
}

void ConfigServer::startAp( const char *ssid, const char *pass )
{
    std::lock_guard lock( apMutex );
    if( ssid ) {
        apSsid = ssid;
    }

#ifdef ARDUINO
    WiFi.mode( WIFI_AP_STA );
    const IPAddress ip( 192, 168, 4, 1 );
    const IPAddress netmask( 255, 255, 255, 0 );

    WiFi.softAPConfig( ip, ip, netmask );
    WiFi.softAP( apSsid.c_str(), pass );
    dnsServer.start( 53, "*", ip );
#endif

    apActive = true;
}

void ConfigServer::stopAp()
{
#ifdef ARDUINO
    dnsServer.stop();
    WiFi.softAPdisconnect( true );
#endif
    apActive = false;
}

bool ConfigServer::isApActive() const
{
    return apActive.load();
}

std::string ConfigServer::getApSsid() const
{
    std::lock_guard lock( apMutex );
    return apSsid;
}

std::string ConfigServer::getApIp() const
{
    std::lock_guard lock( apMutex );
    return apIp;
}

void ConfigServer::handleClient()
{
#ifdef ARDUINO
    if( apActive ) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
#endif
}

#ifdef ARDUINO

void ConfigServer::registerRoutes()
{
    server.on( "/generate_204", HTTP_GET, [this] { handleCaptiveRedirect(); } );
    server.on( "/gen_204", HTTP_GET, [this] { handleCaptiveRedirect(); } );
    server.on( "/hotspot-detect.html", HTTP_GET, [this] { handleCaptiveRedirect(); } );
    server.on( "/ncsi.txt", HTTP_GET, [this] { handleCaptiveRedirect(); } );
    server.on( "/connecttest.txt", HTTP_GET, [this] { handleCaptiveRedirect(); } );
    server.on( "/canonical.html", HTTP_GET, [this] { handleCaptiveRedirect(); } );

    server.on( "/", HTTP_GET, [this] { handleRoot(); } );

    server.on( "/api/status", HTTP_GET, [this] { handleApiStatus(); } );
    server.on( "/api/wifi/scan", HTTP_GET, [this] { handleApiWifiScan(); } );
    server.on( "/api/wifi", HTTP_POST, [this] { handleApiWifiSave(); } );
    server.on( "/api/wifi/clear", HTTP_POST, [this] { handleApiWifiClear(); } );
    server.on( "/api/config", HTTP_POST, [this] { handleApiConfigSave(); } );

    server.on( "/api/sentences", HTTP_GET, [this] { handleApiSentencesGet(); } );
    server.on( "/api/sentences", HTTP_POST, [this] { handleApiSentencesSave(); } );
    server.on( "/api/sentences", HTTP_DELETE, [this] { handleApiSentencesDelete(); } );

    server.on( "/api/ota", HTTP_POST, [this] {
                   if( Update.hasError() ) {
                       server.send( 500, "text/plain", "OTA Failed" );
                   } else {
                       server.send( 200, "application/json", "{\"success\":true}" );
                       handleApiRestart();
                   }
               }, [this] {
                   handleApiOtaUpload();
               } );

    server.on( "/api/restart", HTTP_POST, [this] { handleApiRestart(); } );
    server.on( "/api/orientation/default", HTTP_POST, [this] { handleApiOrientationDefault(); } );

    server.onNotFound( [this] {
        if( server.hostHeader() != apIp.c_str() && apActive ) {
            handleCaptiveRedirect();
        } else {
            server.send( 404, "text/plain", "Not Found" );
        }
    } );
}

void ConfigServer::handleRoot()
{
    server.send_P( 200, "text/html", WebUi::INDEX_HTML );
}

void ConfigServer::handleCaptiveRedirect()
{
    server.sendHeader( "Location", String( "http://" ) + apIp.c_str() + "/", true );
    server.send( 302, "text/plain", "" );
}

void ConfigServer::handleApiStatus()
{
    std::string json = "{";
    json += "\"configured\":" + std::string( networkWorker && networkWorker->isConfigured() ? "true" : "false" ) + ",";
    json += "\"connected\":" + std::string( networkWorker && networkWorker->isConnected() ? "true" : "false" ) + ",";
    json += "\"ip\":\"" + std::string( WiFi.localIP().toString().c_str() ) + "\",";
    json += "\"rssi\":" + std::to_string( WiFi.RSSI() ) + ",";

    if( networkWorker ) {
        json += "\"ssid\":\"" + networkWorker->getSsid() + "\",";
        json += "\"lat\":" + std::to_string( networkWorker->getLatitude() ) + ",";
        json += "\"lon\":" + std::to_string( networkWorker->getLongitude() ) + ",";
    }

    json += "\"heap\":" + std::to_string( esp_get_free_heap_size() ) + ",";
    json += "\"uptime\":" + std::to_string( esp_timer_get_time() / 1000000 ) + ",";
    json += "\"chip\":\"ESP32-S3\",";

    if( sentenceEngine ) {
        json += "\"sentence_count\":" + std::to_string( sentenceEngine->getSentenceCount() ) + ",";
        json += "\"custom_sentences\":" + std::string( sentenceEngine->hasCustomSentences() ? "true" : "false" ) + ",";
    }

    json += "\"mode\":\"" + std::string( apActive ? "AP" : "STA" ) + "\",";

    if( networkWorker ) {
        json += "\"hostname\":\"" + networkWorker->getHostname() + "\",";
        json += "\"timer_mode\":" + std::to_string( networkWorker->getTimerMode() ) + ",";
    } else {
        json += "\"hostname\":\"bot50\",";
        json += "\"timer_mode\":0,";
    }

    Preferences prefs;
    if( prefs.begin( NetworkWorker::NVS_NAMESPACE, true ) ) {
        String tz = prefs.getString( "tz", "CET-1CEST,M3.5.0,M10.5.0/3" );
        json += "\"tz\":\"" + std::string( tz.c_str() ) + "\",";

        prefs.end();
    }

    auto &hal = HardwareManager::instance();
    json += "\"invert_pot_l\":" + std::string( hal.isInvertLeftPot() ? "true" : "false" ) + ",";
    json += "\"invert_pot_r\":" + std::string( hal.isInvertRightPot() ? "true" : "false" ) + ",";
    json += "\"invert_pwr\":" + std::string( hal.isInvertPowerSense() ? "true" : "false" ) + ",";
    json += "\"temp_offset\":" + std::to_string( hal.getTemperatureOffsetC() ) + ",";
    json += "\"night_threshold\":" + std::to_string( hal.getNightThreshold() ) + ",";
    json += "\"slap_threshold\":" + std::to_string( hal.getSlapThresholdG() ) + ",";
    json += "\"orientation_calibrated\":" + std::string(
            hal.getMotion().hasOrientationCalibration() ? "true" : "false" ) + ",";

    json += "\"lang\":\"" + std::string( I18n::instance().getLanguageCode() ) + "\"";
    json += "}";

    server.send( 200, "application/json", json.c_str() );
}

void ConfigServer::handleApiWifiScan()
{
    int16_t n = WiFi.scanNetworks();
    std::string json = "{\"networks\":[";
    for( int16_t i = 0; i < n; ++i ) {
        if( i > 0 )
            json += ",";
        json += "{\"ssid\":\"" + std::string( WiFi.SSID( i ).c_str() ) + "\",";
        json += "\"rssi\":" + std::to_string( WiFi.RSSI( i ) ) + "}";
    }
    json += "]}";
    WiFi.scanDelete();
    server.send( 200, "application/json", json.c_str() );
}

void ConfigServer::handleApiWifiSave()
{
    if( !networkWorker ) {
        server.send( 500, "text/plain", "Internal Error" );
        return;
    }

    PartialConfig cfg;
    if( WebUtils::parsePartialConfig( server.arg( "plain" ).c_str(), cfg ) ) {
        if( cfg.hasSsid ) {
            networkWorker->saveWifiCredentials( cfg.ssid, cfg.pass );
        }
        server.send( 200, "application/json", "{\"success\":true}" );
    } else {
        server.send( 400, "text/plain", "Invalid JSON" );
    }
}

void ConfigServer::handleApiWifiClear()
{
    if( networkWorker ) {
        networkWorker->clearWifiCredentials();
        server.send( 200, "application/json", "{\"success\":true}" );
    } else {
        server.send( 500, "text/plain", "Internal Error" );
    }
}

void ConfigServer::handleApiConfigSave()
{
    if( !networkWorker ) {
        server.send( 500, "text/plain", "Internal Error" );
        return;
    }

    PartialConfig cfg;
    if( WebUtils::parsePartialConfig( server.arg( "plain" ).c_str(), cfg ) ) {
        if( cfg.hasLat || cfg.hasLon || cfg.hasApiKey ) {
            const double lat = cfg.hasLat ? cfg.lat : networkWorker->getLatitude();
            const double lon = cfg.hasLon ? cfg.lon : networkWorker->getLongitude();
            const std::string apiKey = cfg.hasApiKey ? cfg.apiKey : networkWorker->getApiKey();
            networkWorker->saveWeatherConfig( lat, lon, apiKey );
        }
        if( cfg.hasTz ) {
            networkWorker->saveTimezone( cfg.tz );
        }
        if( cfg.hasLang ) {
            I18n::instance().setLanguageCode( cfg.lang.c_str() );
            I18n::instance().saveToNvs();
        }
        if( cfg.hasHostname ) {
            networkWorker->saveHostname( cfg.hostname );
        }
        if( cfg.hasTimerMode ) {
            networkWorker->saveTimerMode( cfg.timerMode );
        }

        if( cfg.hasInvertPotL || cfg.hasInvertPotR || cfg.hasInvertPowerSense || cfg.hasTempOffset || cfg.
            hasNightThreshold || cfg.hasSlapThreshold ) {
            auto &hal = HardwareManager::instance();
            Preferences hwPrefs;
            const bool opened = hwPrefs.begin( "friendlybot", false );

            if( cfg.hasInvertPotL ) {
                if( opened ) {
                    hwPrefs.putUChar( "inv_pot_l", cfg.invertPotL ? 1 : 0 );
                }
                hal.setInvertLeftPot( cfg.invertPotL );
            }
            if( cfg.hasInvertPotR ) {
                if( opened ) {
                    hwPrefs.putUChar( "inv_pot_r", cfg.invertPotR ? 1 : 0 );
                }
                hal.setInvertRightPot( cfg.invertPotR );
            }
            if( cfg.hasInvertPowerSense ) {
                if( opened ) {
                    hwPrefs.putUChar( "inv_pwr", cfg.invertPowerSense ? 1 : 0 );
                }
                hal.setInvertPowerSense( cfg.invertPowerSense );
            }
            if( cfg.hasTempOffset ) {
                if( opened ) {
                    hwPrefs.putFloat( "temp_offset", cfg.tempOffset );
                }
                hal.setTemperatureOffsetC( cfg.tempOffset );
            }
            if( cfg.hasNightThreshold ) {
                if( opened ) {
                    hwPrefs.putUShort( "night_thresh", cfg.nightThreshold );
                }
                hal.setNightThreshold( cfg.nightThreshold );
            }
            if( cfg.hasSlapThreshold ) {
                if( opened ) {
                    hwPrefs.putFloat( "slap_thresh", cfg.slapThreshold );
                }
                hal.setSlapThresholdG( cfg.slapThreshold );
            }

            if( opened ) {
                hwPrefs.end();
            }
        }

        server.send( 200, "application/json", "{\"success\":true}" );
    } else {
        server.send( 400, "text/plain", "Invalid JSON" );
    }
}

void ConfigServer::handleApiSentencesGet()
{
    if( server.hasArg( "source" ) && server.arg( "source" ) == "builtin" ) {
        std::string out;
        size_t n = I18n::instance().getBuiltinAphorismCount();
        for( size_t i = 0; i < n; ++i ) {
            out += I18n::instance().getBuiltinAphorism( i );
            if( i < n - 1 )
                out += "\n";
        }
        server.send( 200, "text/plain", out.c_str() );
        return;
    }

    bool custom = false;
    size_t fileSize = 0;
    if( sentenceEngine && sentenceEngine->hasCustomSentences() ) {
        custom = true;
    }

#if __has_include( <LittleFS.h> )
    if( LittleFS.exists( SentenceEngine::SENTENCES_FILE_PATH ) ) {
        if( File f = LittleFS.open( SentenceEngine::SENTENCES_FILE_PATH, "r" ) )
            fileSize = f.size();
    }
#endif

    server.setContentLength( CONTENT_LENGTH_UNKNOWN );
    server.send( 200, "application/json", "" );

    std::string startJson = "{\"custom\":" + std::string( custom ? "true" : "false" );

    const size_t pct = ( fileSize * 100 ) / SentenceEngine::MAX_FILE_SIZE_BYTES;

    startJson += R"(,"quota":")" + std::to_string( fileSize / 1024 ) + " / " + std::to_string(
            SentenceEngine::MAX_FILE_SIZE_BYTES / 1024 ) + " KB (" + std::to_string( pct ) + "%)\"";
    startJson += R"(,"percent":)" + std::to_string( pct );
    startJson += R"(,"text":")";

    server.sendContent( startJson.c_str() );

    if( custom ) {
#if __has_include( <LittleFS.h> )
        if( File f = LittleFS.open( SentenceEngine::SENTENCES_FILE_PATH, "r" ) ) {
            bool inString = false;
            bool escaped = false;
            bool firstString = true;
            char outBuf[ 128 ];
            size_t outPos = 0;

            auto flushBuf = [&] {
                if( outPos > 0 ) {
                    outBuf[ outPos ] = '\0';
                    server.sendContent( outBuf );
                    outPos = 0;
                }
            };

            while( f.available() ) {
                char c = f.read();
                if( !inString ) {
                    if( c == '"' ) {
                        inString = true;
                        if( !firstString ) {
                            outBuf[ outPos++ ] = '\\';
                            outBuf[ outPos++ ] = 'n';
                            if( outPos >= sizeof( outBuf ) - 2 )
                                flushBuf();
                        }
                        firstString = false;
                    }
                } else {
                    if( escaped ) {
                        escaped = false;
                        outBuf[ outPos++ ] = c;
                    } else if( c == '\\' ) {
                        escaped = true;
                        outBuf[ outPos++ ] = c;
                    } else if( c == '"' ) {
                        inString = false;
                    } else {
                        outBuf[ outPos++ ] = c;
                    }
                    if( outPos >= sizeof( outBuf ) - 2 )
                        flushBuf();
                }
            }
            flushBuf();
        }
#endif
    }
    server.sendContent( "\"}" );
    server.sendContent( "" );
}

void ConfigServer::handleApiSentencesSave()
{
    if( !sentenceEngine ) {
        server.send( 500, "text/plain", "Internal Error" );
        return;
    }

    const std::string text = server.arg( "plain" ).c_str();
    std::string jsonOut;

    if( size_t count = 0; WebUtils::plainTextToJsonSentences( text, jsonOut, SentenceEngine::MAX_FILE_SIZE_BYTES, count ) ) {
        if( sentenceEngine->saveCustomJsonAtomic( jsonOut.c_str(), jsonOut.length() ) ) {
            server.send( 200, "application/json", "{\"success\":true}" );
        } else {
            server.send( 500, "text/plain", "Save Failed" );
        }
    } else {
        server.send( 400, "text/plain", "Quota Exceeded" );
    }
}

void ConfigServer::handleApiSentencesDelete()
{
    if( sentenceEngine ) {
        sentenceEngine->clearCustomSentences();
        server.send( 200, "application/json", "{\"success\":true}" );
    } else {
        server.send( 500, "text/plain", "Internal Error" );
    }
}

void ConfigServer::handleApiOtaUpload()
{
    HTTPUpload &upload = server.upload();
    if( upload.status == UPLOAD_FILE_START ) {
        std::string filename = upload.filename.c_str();
        int cmd = U_FLASH;
        if( filename.find( ".littlefs.bin" ) != std::string::npos || filename.find( "spiffs" ) != std::string::npos ||
            filename.find( "littlefs" ) != std::string::npos ) {
            cmd = U_SPIFFS;
        }

        if( !Update.begin( UPDATE_SIZE_UNKNOWN, cmd ) ) {
            Update.printError( Serial );
        }
    } else if( upload.status == UPLOAD_FILE_WRITE ) {
        if( Update.write( upload.buf, upload.currentSize ) != upload.currentSize ) {
            Update.printError( Serial );
        }
    } else if( upload.status == UPLOAD_FILE_END ) {
        if( Update.end( true ) ) {
            // Success
        } else {
            Update.printError( Serial );
        }
    } else if( upload.status == UPLOAD_FILE_ABORTED ) {
        Update.end();
    }
}

void ConfigServer::handleApiRestart()
{
    server.send( 200, "application/json", "{\"success\":true}" );

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = []( void *arg ) {
        esp_restart();
    };
    timer_args.name = "restart_timer";
    esp_timer_handle_t timer;
    if( esp_timer_create( &timer_args, &timer ) == ESP_OK ) {
        esp_timer_start_once( timer, 500000 );
    }
}

void ConfigServer::handleApiOrientationDefault()
{
    auto &hal = HardwareManager::instance();
    const bool ok = hal.saveDefaultOrientation();
    if( ok ) {
        server.send( 200, "application/json", "{\"success\":true,\"calibrated\":true}" );
    } else {
        server.send( 500, "application/json", "{\"success\":false}" );
    }
}

#endif // ARDUINO
