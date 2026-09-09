//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/serialCli.h"
#include "i18n.h"
#include "services/configServer.h"

#include <cstdlib>
#include <cstdint>
#include <string>

#include <Arduino.h>
#include <WiFi.h>

namespace
{
    [[nodiscard]] const char *gagStateToString( const GagState state )
    {
        switch( state ) {
            case GagState::Idle:
                return "Idle";
            case GagState::Glitching:
                return "Glitching";
            case GagState::RejectingButton:
                return "RejectingButton";
            case GagState::RepairingAnimation:
                return "RepairingAnimation";
            case GagState::RepairedSuccess:
                return "RepairedSuccess";
            default:
                return "Unknown";
        }
    }

    [[nodiscard]] const char *arbitratorStateToString( const ArbitratorState state )
    {
        switch( state ) {
            case ArbitratorState::Idle:
                return "Idle";
            case ArbitratorState::CategoryBrowsing:
                return "CategoryBrowsing";
            case ArbitratorState::CategoryLocked:
                return "CategoryLocked";
            case ArbitratorState::Spinning:
                return "Spinning";
            case ArbitratorState::Verdict:
                return "Verdict";
            default:
                return "Unknown";
        }
    }

    [[nodiscard]] const char *timerStateToString( const TimerState state )
    {
        switch( state ) {
            case TimerState::Idle:
                return "Idle";
            case TimerState::Setting:
                return "Setting";
            case TimerState::Running:
                return "Running";
            case TimerState::Paused:
                return "Paused";
            case TimerState::Alarming:
                return "Alarming";
            default:
                return "Unknown";
        }
    }
}

void SerialCli::init( SystemOrchestrator *orchestratorPtr, HardwareManager *halPtr, NetworkWorker *netPtr,
                      TimeService *timePtr, WeatherService *weatherPtr, SentenceEngine *sentencesPtr,
                      PowerManager *powerPtr, DeskCompanionController *companionPtr, GagController *gagPtr,
                      ArbitratorController *arbitratorPtr, TimerController *timerPtr )
{
    orchestrator = orchestratorPtr;
    hal = halPtr;
    net = netPtr;
    time = timePtr;
    weather = weatherPtr;
    sentences = sentencesPtr;
    power = powerPtr;
    companion = companionPtr;
    gag = gagPtr;
    arbitrator = arbitratorPtr;
    timer = timerPtr;

    hwTestStreaming = false;
    streamStartMs = 0;
    lastTelemetryMs = 0;
}

void SerialCli::update( const uint32_t nowMs )
{
    if( Serial.available() > 0 ) {
        if( hwTestStreaming ) {
            stopHwTest();
        }
        String cmd = Serial.readStringUntil( '\n' );
        cmd.trim();
        if( cmd.length() > 0 ) {
            processCommand( cmd, nowMs );
        }
    }

    if( hwTestStreaming ) {
        if( nowMs - streamStartMs >= HW_TEST_DURATION_MS ) {
            stopHwTest();
        } else if( nowMs - lastTelemetryMs >= TELEMETRY_INTERVAL_MS ) {
            lastTelemetryMs = nowMs;
            emitTelemetryLine();
        }
    }
}

bool SerialCli::isHwTestStreaming() const
{
    return hwTestStreaming;
}

void SerialCli::stopHwTest()
{
    if( hwTestStreaming ) {
        hwTestStreaming = false;
        Serial.println( "--- Hardware Test Complete ---" );
    }
}

void SerialCli::emitTelemetryLine()
{
    if( !hal ) {
        return;
    }

    Serial.printf(
        "[HW_TEST] PotL: %.2f (%.1fm) | PotR: %u | LDR: %u (Brightness=%d, Night=%d) | Power: %s | ToF: %umm | Temp: %.1fC | Acc: %.2f,%.2f,%.2f (%.2fG) | BtnA: %d, BtnR: %d\n",
        hal->getLeftPot().getNormalized(), hal->getLeftPot().getMappedNonLinear( 0.5f, 30.0f ),
        hal->getRightPot().getStep( 4 ), hal->getLdr().getRawValue(), hal->getLdr().getIdealDisplayBrightnessLevel(),
        hal->getLdr().isNight() ? 1 : 0, hal->getPowerSense().isUsbPowered() ? "USB" : "BAT",
        hal->getProximity().getDistanceMm(), hal->getMotion().getTemperatureC(), hal->getMotion().getAccelX(),
        hal->getMotion().getAccelY(), hal->getMotion().getAccelZ(), hal->getMotion().getTotalG(),
        hal->getActionButton().isPressed() ? 1 : 0, hal->getResetButton().isPressed() ? 1 : 0 );
}

void SerialCli::printHelp()
{
    Serial.println( "--- Available Commands ---" );
    Serial.println( "  diag            : Display sensor readings & system diagnostics" );
    Serial.println( "  hw_test         : Interactive hardware checkout & 10Hz telemetry stream" );
    Serial.println( "  beep            : Play acoustic chirp on buzzer" );
    Serial.println( "  wom             : Enable MPU6050 Wake-on-Motion" );
    Serial.println( "  jokes_info      : Sentence engine status and quotas" );
    Serial.println( "  joke            : Display next sentence" );
    Serial.println( "  lang <en|it>    : Set system language" );
    Serial.println( "  wifi <ssid> <pw>: Configure WiFi credentials" );
    Serial.println( "  wifi_clear      : Erase stored WiFi credentials" );
    Serial.println( "  weather <lat> <lon> <k> : Configure OpenWeather coordinates & API key" );
    Serial.println( "  tz <posix_tz>   : Configure POSIX timezone string" );
    Serial.println( "  net_status      : Network, NTP, and weather status" );
    Serial.println( "  portal_status   : Web portal & captive AP status" );
    Serial.println( "  portal_ap       : Manually start config captive AP" );
    Serial.println( "  portal_sta      : Trigger immediate WiFi station sync" );
    Serial.println( "  glitch          : Force trigger slap gag" );
    Serial.println( "  gag_info        : Slap gag controller status" );
    Serial.println( "  oracle          : Activate arbitrator mode" );
    Serial.println( "  spin            : Trigger arbitrator spin" );
    Serial.println( "  arbitrator_info : Arbitrator controller status" );
    Serial.println( "  timer [sec]     : Activate or start kitchen timer" );
    Serial.println( "  alarm           : Force trigger timer alarm" );
    Serial.println( "  timer_info      : Kitchen timer controller status" );
    Serial.println( "  power_info      : Power manager & sleep status" );
    Serial.println( "  sleep           : Force enter deep sleep" );
    Serial.println( "  help            : Show this help message" );
}

void SerialCli::processCommand( const String& rawCmd, const uint32_t nowMs )
{
    String cmd = rawCmd;
    cmd.trim();
    if( cmd.length() == 0 ) {
        return;
    }

    if( hwTestStreaming && !cmd.startsWith( "hw_test" ) ) {
        stopHwTest();
    }

    bool validCommand = true;

    if( cmd.startsWith( "diag" ) ) {
        Serial.println( "--- Diagnostics ---" );
        if( hal ) {
            Serial.printf( "Left Pot: %.2f (%.1f min)\n", hal->getLeftPot().getNormalized(),
                           hal->getLeftPot().getMappedNonLinear( 0.5f, 30.0f ) );
            Serial.printf( "Right Pot: %u (Category)\n", hal->getRightPot().getStep( 4 ) );
            Serial.printf( "LDR Light: %u (Brightness=%d, Night=%d)\n", hal->getLdr().getRawValue(),
                           hal->getLdr().getIdealDisplayBrightnessLevel(), hal->getLdr().isNight() );
            Serial.printf( "Power: %s\n", hal->getPowerSense().isUsbPowered() ? "USB" : "Battery" );
            Serial.printf( "Temp: %.1f C\n", hal->getMotion().getTemperatureC() );
            Serial.printf( "Distance: %u mm (Wake=%d)\n", hal->getProximity().getDistanceMm(),
                           hal->getProximity().shouldWakeUp() );
        }
    } else if( cmd.startsWith( "hw_test" ) ) {
        if( hal ) {
            hal->getBuzzer().beep( 100, nowMs );
        }
        Serial.println( "--- Hardware Checkout & Telemetry Test ---" );
        hwTestStreaming = true;
        streamStartMs = nowMs;
        lastTelemetryMs = nowMs;
    } else if( cmd.startsWith( "beep" ) ) {
        if( hal ) {
            hal->getBuzzer().beep( 100, nowMs );
        }
    } else if( cmd.startsWith( "wom" ) ) {
        if( hal ) {
            hal->getMotion().enableWakeOnMotion();
            Serial.println( "[HAL] MPU6050 Wake-on-Motion enabled." );
        }
    } else if( cmd.startsWith( "jokes_info" ) ) {
        Serial.println( "--- Sentence Engine Info ---" );
        if( sentences ) {
            Serial.printf( "Custom Sentences Active: %s\n", sentences->hasCustomSentences() ? "YES" : "NO" );
            Serial.printf( "Total Sentences: %u\n", static_cast<unsigned int>(sentences->getSentenceCount()) );
            Serial.printf( "Active Language: %s\n", I18n::instance().getLanguageCode() );
            Serial.printf( "Max File Quota: %u bytes\n",
                           static_cast<unsigned int>(SentenceEngine::MAX_FILE_SIZE_BYTES) );
        }
    } else if( cmd.startsWith( "joke" ) ) {
        if( sentences ) {
            std::string sentence;
            if( sentences->getNextSentence( sentence ) ) {
                Serial.printf( "[SENTENCE] %s\n", sentence.c_str() );
                if( hal ) {
                    hal->getDisplay().displaySentence( sentence );
                }
            } else {
                Serial.println( "[SENTENCE] No sentence available." );
            }
        }
    } else if( cmd.startsWith( "lang" ) ) {
        String code = cmd.substring( 4 );
        code.trim();
        if( I18n::instance().setLanguageCode( code.c_str() ) ) {
            I18n::instance().saveToNvs();
            Serial.printf( "[I18N] Language changed to: %s\n", code.c_str() );
            if( companion ) {
                companion->onUserActionClick( nowMs );
            }
        } else {
            Serial.printf( "[I18N] Unknown language: '%s' (supported: en, it)\n", code.c_str() );
        }
    } else if( cmd.startsWith( "wifi_clear" ) ) {
        if( net ) {
            net->clearWifiCredentials();
        }
        if( companion ) {
            companion->setConfiguredOnline( false );
        }
        Serial.println( "[CLI] WiFi credentials cleared." );
    } else if( cmd.startsWith( "wifi" ) ) {
        String args = cmd.substring( 4 );
        args.trim();
        const int spaceIdx = args.indexOf( ' ' );
        if( spaceIdx > 0 ) {
            String ssid = args.substring( 0, spaceIdx );
            String pass = args.substring( spaceIdx + 1 );
            ssid.trim();
            pass.trim();
            if( net ) {
                net->saveWifiCredentials( ssid.c_str(), pass.c_str() );
            }
            if( companion ) {
                companion->setConfiguredOnline( true );
            }
            Serial.printf( "[CLI] Saved WiFi SSID: %s\n", ssid.c_str() );
        } else if( args.length() > 0 ) {
            String ssid = args;
            ssid.trim();
            if( net ) {
                net->saveWifiCredentials( ssid.c_str(), "" );
            }
            if( companion ) {
                companion->setConfiguredOnline( true );
            }
            Serial.printf( "[CLI] Saved open WiFi SSID: %s\n", ssid.c_str() );
        } else {
            Serial.println( "[CLI] Usage: wifi <SSID> <PASSWORD>" );
        }
    } else if( cmd.startsWith( "weather" ) ) {
        String args = cmd.substring( 7 );
        args.trim();
        const int spaceIdx1 = args.indexOf( ' ' );
        const int spaceIdx2 = ( spaceIdx1 > 0 ) ? args.indexOf( ' ', spaceIdx1 + 1 ) : -1;
        if( spaceIdx1 > 0 && spaceIdx2 > spaceIdx1 ) {
            String latStr = args.substring( 0, spaceIdx1 );
            String lonStr = args.substring( spaceIdx1 + 1, spaceIdx2 );
            String apikey = args.substring( spaceIdx2 + 1 );
            latStr.trim();
            lonStr.trim();
            apikey.trim();
            const double lat = std::strtod( latStr.c_str(), nullptr );
            const double lon = std::strtod( lonStr.c_str(), nullptr );
            if( net ) {
                net->saveWeatherConfig( lat, lon, apikey.c_str() );
            }
            Serial.printf( "[CLI] Saved Weather Config for Coords: %.4f, %.4f\n", lat, lon );
        } else {
            Serial.println( "[CLI] Usage: weather <LAT> <LON> <API_KEY>" );
        }
    } else if( cmd.startsWith( "tz" ) ) {
        String tz = cmd.substring( 2 );
        tz.trim();
        if( tz.length() > 0 ) {
            if( net ) {
                net->saveTimezone( tz.c_str() );
            }
            Serial.printf( "[CLI] Saved Timezone: %s\n", tz.c_str() );
        } else {
            Serial.println( "[CLI] Usage: tz <POSIX_TZ_STRING>" );
        }
    } else if( cmd.startsWith( "net_status" ) ) {
        Serial.println( "--- Network & Services Status ---" );
        if( net ) {
            Serial.printf( "Configured: %s\n", net->isConfigured() ? "YES" : "NO" );
            Serial.printf( "SSID: %s\n", net->getSsid().c_str() );
        }
        Serial.printf( "WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED" );
        if( WiFi.status() == WL_CONNECTED ) {
            Serial.printf( "IP Address: %s\n", WiFi.localIP().toString().c_str() );
        }
        if( time ) {
            Serial.printf( "Timezone: %s\n", time->getTimezone().c_str() );
            Serial.printf( "NTP Synced: %s\n", time->isTimeSynced() ? "YES" : "NO" );

            tm timeInfo{};
            if( time->getLocalTimeInfo( timeInfo ) ) {
                Serial.printf( "Local Time: %s %s\n",
                               TimeService::formatDate( timeInfo, I18n::instance().getLanguage() ).c_str(),
                               TimeService::formatTime( timeInfo ).c_str() );
            } else {
                Serial.println( "Local Time: Not Available" );
            }
        }
        if( weather && net ) {
            const WeatherData w = weather->getLatestWeather();
            Serial.printf( "Weather Coords: %.4f, %.4f\n", net->getLatitude(), net->getLongitude() );
            Serial.printf( "Weather Valid: %s\n", w.valid ? "YES" : "NO" );
            if( w.valid ) {
                Serial.printf( "Weather: %.1f C, Humidity: %u%%, Condition: %s\n", w.tempC, w.humidity,
                               w.condition.c_str() );
            }
        }
    } else if( cmd.startsWith( "portal_status" ) ) {
        Serial.println( "--- Portal Status ---" );
        Serial.printf( "AP Active: %s\n", ConfigServer::instance().isApActive() ? "YES" : "NO" );
        Serial.printf( "AP SSID: %s\n", ConfigServer::instance().getApSsid().c_str() );
        Serial.printf( "AP IP: %s\n", ConfigServer::instance().getApIp().c_str() );
        if( net ) {
            Serial.printf( "Hostname: %s\n", net->getHostname().c_str() );
        }
        Serial.printf( "Clients Connected: %u\n", WiFi.softAPgetStationNum() );
    } else if( cmd.startsWith( "portal_ap" ) ) {
        ConfigServer::instance().startAp();
        Serial.println( "[CLI] Access Point started manually." );
    } else if( cmd.startsWith( "portal_sta" ) ) {
        if( net ) {
            net->requestImmediateFetch();
        }
        Serial.println( "[CLI] Forced immediate station retry / fetch." );
    } else if( cmd.startsWith( "glitch" ) ) {
        if( gag ) {
            gag->trigger( nowMs, true /* force */ );
            Serial.println( "[CLI] Gag triggered (forced)." );
        }
        if( orchestrator ) {
            orchestrator->requestMode( SystemMode::SlapGag, nowMs );
        }
    } else if( cmd.startsWith( "gag_info" ) ) {
        Serial.println( "--- Gag Controller Info ---" );
        if( gag ) {
            Serial.printf( "Active: %s\n", gag->isActive() ? "YES" : "NO" );
            Serial.printf( "State: %s\n", gagStateToString( gag->getState() ) );
            Serial.printf( "Trigger Count: %u/%u\n", gag->getTriggerCount(), GagController::MAX_DAILY_BUDGET );
            Serial.printf( "Can Trigger: %s\n", gag->canTrigger( nowMs ) ? "YES" : "NO" );
        } else {
            Serial.println( "GagController not initialized." );
        }
    } else if( cmd.startsWith( "oracle" ) ) {
        if( arbitrator ) {
            arbitrator->activate();
            Serial.println( "[CLI] Arbitrator (Oracle) mode activated." );
        } else {
            Serial.println( "[CLI] ArbitratorController not initialized." );
        }
        if( orchestrator ) {
            orchestrator->requestMode( SystemMode::Arbitrator, nowMs );
        }
    } else if( cmd.startsWith( "spin" ) ) {
        if( arbitrator ) {
            if( !arbitrator->isActive() ) {
                arbitrator->activate();
            }
            arbitrator->onActionButton();
            Serial.println( "[CLI] Arbitrator spin triggered." );
        } else {
            Serial.println( "[CLI] ArbitratorController not initialized." );
        }
        if( orchestrator ) {
            orchestrator->requestMode( SystemMode::Arbitrator, nowMs );
        }
    } else if( cmd.startsWith( "arbitrator_info" ) ) {
        Serial.println( "--- Arbitrator Controller Info ---" );
        if( arbitrator ) {
            Serial.printf( "Active: %s\n", arbitrator->isActive() ? "YES" : "NO" );
            Serial.printf( "State: %s\n", arbitratorStateToString( arbitrator->getState() ) );
            Serial.printf( "Selected Category: %u\n", arbitrator->getSelectedCategory() );
            Serial.printf( "Verdict Index: %u\n", static_cast<unsigned int>(arbitrator->getVerdictIndex()) );
        } else {
            Serial.println( "ArbitratorController not initialized." );
        }
    } else if( cmd.startsWith( "timer_info" ) ) {
        Serial.println( "--- Timer Controller Info ---" );
        if( timer ) {
            Serial.printf( "Active: %s\n", timer->isActive() ? "YES" : "NO" );
            Serial.printf( "State: %s\n", timerStateToString( timer->getState() ) );
            Serial.printf( "Remaining: %u s\n", timer->getRemainingSeconds() );
            Serial.printf( "Target: %u s\n", timer->getTargetSeconds() );
            Serial.printf( "Dimmed: %s\n", timer->isDimmed() ? "YES" : "NO" );
            Serial.printf( "Display Powered: %s\n", ( hal && hal->getDisplay().isPoweredOn() ) ? "YES" : "NO" );
        } else {
            Serial.println( "TimerController not initialized." );
        }
    } else if( cmd.startsWith( "timer" ) ) {
        String arg = cmd.substring( 5 );
        arg.trim();
        if( timer ) {
            if( arg.length() > 0 ) {
                const int sec = arg.toInt();
                if( sec > 0 ) {
                    timer->startWithDuration( static_cast<uint16_t>(sec) );
                    Serial.printf( "[CLI] Timer started with %d seconds.\n", sec );
                } else {
                    Serial.println( "[CLI] Invalid seconds. Usage: timer <sec>" );
                }
            } else {
                timer->activate();
                Serial.println( "[CLI] Timer mode activated (Setting)." );
            }
        } else {
            Serial.println( "[CLI] TimerController not initialized." );
        }
        if( orchestrator ) {
            orchestrator->requestMode( SystemMode::Timer, nowMs );
        }
    } else if( cmd.startsWith( "alarm" ) ) {
        if( timer ) {
            timer->triggerAlarm();
            Serial.println( "[CLI] Alarm triggered (forced)." );
        } else {
            Serial.println( "[CLI] TimerController not initialized." );
        }
        if( orchestrator ) {
            orchestrator->requestMode( SystemMode::Timer, nowMs );
        }
    } else if( cmd.startsWith( "power_info" ) ) {
        Serial.println( "--- Power Manager Info ---" );
        if( hal ) {
            Serial.printf( "Source: %s\n", hal->getPowerSense().isUsbPowered() ? "USB" : "Battery" );
        }
        if( net ) {
            Serial.printf( "Profile: %s\n", net->getPowerProfile() == PowerProfile::Usb ? "USB" : "Battery" );
        }
        if( power ) {
            Serial.printf( "Last Activity: %u ms (now=%u ms, idle=%u ms)\n", power->getLastActivityMs(), nowMs,
                           nowMs - power->getLastActivityMs() );
            Serial.printf( "Can Sleep: %s\n", power->canEnterSleep() ? "YES" : "NO" );
            Serial.printf( "Should Sleep: %s\n", power->shouldEnterSleep( nowMs ) ? "YES" : "NO" );
        }
        Serial.printf( "RTC Wake Count: %u\n", rtcWakeCountPtr ? *rtcWakeCountPtr : 0 );
        Serial.printf( "RTC Last WiFi Attempt Epoch: %ld\n",
                       rtcLastWifiAttemptEpochPtr ? *rtcLastWifiAttemptEpochPtr : 0L );
    } else if( cmd.startsWith( "sleep" ) ) {
        Serial.println( "[CLI] Forcing deep sleep..." );
        if( power && hal ) {
            power->enterDeepSleep( hal->getMotion() );
        }
    } else if( cmd.startsWith( "help" ) ) {
        printHelp();
    } else {
        validCommand = false;
        Serial.printf( "[CLI] Unknown command: '%s'. Type 'help' for available commands.\n", cmd.c_str() );
    }

    if( validCommand ) {
        if( power ) {
            power->resetActivity( nowMs );
        } else {
            PowerManager::instance().resetActivity( nowMs );
        }
    }
}
