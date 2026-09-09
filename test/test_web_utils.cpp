#include "services/configServer.h"
#include "services/networkWorker.h"
#include "services/timeService.h"
#include "services/webUtils.h"

#include <cassert>
#include <cmath>
#include <iostream>

void testSanitizeHostname()
{
    assert( WebUtils::sanitizeHostname( "Bot-50!" ) == "bot-50" );
    assert( WebUtils::sanitizeHostname( "MY BOT" ) == "my-bot" );
    assert( WebUtils::sanitizeHostname( "" ) == "bot50" );
    assert( WebUtils::sanitizeHostname( "---invalid---" ) == "bot50" );
    assert( WebUtils::sanitizeHostname( "  leading and trailing spaces  " ) == "leading-and-trailing-spaces" );
    assert( WebUtils::sanitizeHostname( "bot--50" ) == "bot-50" );
    assert( WebUtils::sanitizeHostname( "-leading-dash" ) == "bot50" );
    assert( WebUtils::sanitizeHostname( "trailing-dash-" ) == "bot50" );
    assert( WebUtils::sanitizeHostname( "!@#$%" ) == "bot50" );
    assert( WebUtils::sanitizeHostname( "Friendly_Bot_50" ) == "friendly-bot-50" );
    std::cout << "PASS: testSanitizeHostname\n";
}

void testPlainTextToJsonSentences()
{
    const std::string text = "Joke number 1\n\nJoke number 2 with \"quotes\"\r\nJoke 3";
    std::string outJson;
    size_t count = 0;
    const bool ok = WebUtils::plainTextToJsonSentences( text, outJson, 1024, count );
    assert( ok );
    assert( count == 3 );
    assert( outJson.find( "Joke number 1" ) != std::string::npos );
    assert( outJson.find( "\\\"quotes\\\"" ) != std::string::npos );

    // Empty text
    std::string emptyJson;
    size_t emptyCount = 99;
    assert( WebUtils::plainTextToJsonSentences( "", emptyJson, 1024, emptyCount ) );
    assert( emptyJson == "[]" );
    assert( emptyCount == 0 );

    // Quota overflow test
    std::string tinyJson;
    size_t tinyCount = 0;
    const bool overflow = WebUtils::plainTextToJsonSentences( text, tinyJson, 20, tinyCount );
    assert( !overflow );

    // Escaping special characters
    const std::string complexText = "Line with \\ backslash\t\tand tab\nLine with / slash";
    std::string complexJson;
    size_t complexCount = 0;
    assert( WebUtils::plainTextToJsonSentences( complexText, complexJson, 2048, complexCount ) );
    assert( complexCount == 2 );
    assert( complexJson.find( "\\\\" ) != std::string::npos );
    assert( complexJson.find( "\\t" ) != std::string::npos );

    std::cout << "PASS: testPlainTextToJsonSentences\n";
}

void testPartialConfigParsing()
{
    // Partial JSON with subset of keys
    {
        const std::string json = "{\"lat\":45.4642,\"lon\":9.1900,\"timer_mode\":1}";
        PartialConfig cfg;
        assert( WebUtils::parsePartialConfig( json, cfg ) );
        assert( cfg.hasLat && std::abs( cfg.lat - 45.4642 ) < 0.0001 );
        assert( cfg.hasLon && std::abs( cfg.lon - 9.1900 ) < 0.0001 );
        assert( cfg.hasTimerMode && cfg.timerMode == 1 );
        assert( !cfg.hasApiKey );
        assert( !cfg.hasSsid );
        assert( !cfg.hasPass );
        assert( !cfg.hasHostname );
        assert( !cfg.hasTz );
        assert( !cfg.hasLang );
    }

    // Full JSON with all keys
    {
        const std::string json =
                "{\"ssid\":\"MyHomeWiFi\",\"pass\":\"secret123\",\"hostname\":\"bot-master\",\"latitude\":45.4642,\"longitude\":9.1900,"
                "\"api_key\":\"abc123xyz\",\"tz\":\"CET-1CEST,M3.5.0,M10.5.0/3\",\"lang\":\"it\",\"timer_mode\":2}";
        PartialConfig cfg;
        assert( WebUtils::parsePartialConfig( json, cfg ) );
        assert( cfg.hasSsid && cfg.ssid == "MyHomeWiFi" );
        assert( cfg.hasPass && cfg.pass == "secret123" );
        assert( cfg.hasHostname && cfg.hostname == "bot-master" );
        assert( cfg.hasLat && std::abs( cfg.lat - 45.4642 ) < 0.0001 );
        assert( cfg.hasLon && std::abs( cfg.lon - 9.1900 ) < 0.0001 );
        assert( cfg.hasApiKey && cfg.apiKey == "abc123xyz" );
        assert( cfg.hasTz && cfg.tz == "CET-1CEST,M3.5.0,M10.5.0/3" );
        assert( cfg.hasLang && cfg.lang == "it" );
        assert( cfg.hasTimerMode && cfg.timerMode == 2 );
    }

    // Alternative alias keys (password, apiKey, timezone, language, timerMode)
    {
        const std::string json =
                "{\"password\":\"secret456\",\"apiKey\":\"key456\",\"timezone\":\"UTC\",\"language\":\"en\",\"timerMode\":0}";
        PartialConfig cfg;
        assert( WebUtils::parsePartialConfig( json, cfg ) );
        assert( cfg.hasPass && cfg.pass == "secret456" );
        assert( cfg.hasApiKey && cfg.apiKey == "key456" );
        assert( cfg.hasTz && cfg.tz == "UTC" );
        assert( cfg.hasLang && cfg.lang == "en" );
        assert( cfg.hasTimerMode && cfg.timerMode == 0 );
        assert( !cfg.hasSsid );
        assert( !cfg.hasLat );
        assert( !cfg.hasLon );
    }

    // Invalid / Empty JSON
    {
        PartialConfig cfg;
        assert( !WebUtils::parsePartialConfig( "", cfg ) );
        assert( !WebUtils::parsePartialConfig( "invalid-json", cfg ) );
        assert( !WebUtils::parsePartialConfig( "{unclosed", cfg ) );
    }

    std::cout << "PASS: testPartialConfigParsing\n";
}

void testHardwareConfigParsing()
{
    const std::string json = R"({
        "invert_pot_l": true,
        "invert_pot_r": false,
        "invert_pwr": true,
        "temp_offset": -2.5,
        "night_threshold": 450,
        "slap_threshold": 1.4
    })";

    PartialConfig cfg;
    assert( WebUtils::parsePartialConfig( json, cfg ) );
    assert( cfg.hasInvertPotL && cfg.invertPotL == true );
    assert( cfg.hasInvertPotR && cfg.invertPotR == false );
    assert( cfg.hasInvertPowerSense && cfg.invertPowerSense == true );
    assert( cfg.hasTempOffset && std::abs( cfg.tempOffset - ( -2.5f ) ) < 0.01f );
    assert( cfg.hasNightThreshold && cfg.nightThreshold == 450 );
    assert( cfg.hasSlapThreshold && std::abs( cfg.slapThreshold - 1.4f ) < 0.01f );

    // Aliases test (night_thresh, slap_thresh)
    const std::string jsonAliases = R"({
        "night_thresh": 500,
        "slap_thresh": 2.2
    })";
    PartialConfig cfgAliases;
    assert( WebUtils::parsePartialConfig( jsonAliases, cfgAliases ) );
    assert( cfgAliases.hasNightThreshold && cfgAliases.nightThreshold == 500 );
    assert( cfgAliases.hasSlapThreshold && std::abs( cfgAliases.slapThreshold - 2.2f ) < 0.01f );
    assert( !cfgAliases.hasInvertPotL );
    assert( !cfgAliases.hasInvertPotR );
    assert( !cfgAliases.hasInvertPowerSense );
    // Clamping test (out-of-range thresholds)
    const std::string jsonOutOfRange = R"({
        "night_threshold": 9999,
        "slap_threshold": 0.1
    })";
    PartialConfig cfgClampedLow;
    assert( WebUtils::parsePartialConfig( jsonOutOfRange, cfgClampedLow ) );
    assert( cfgClampedLow.hasNightThreshold && cfgClampedLow.nightThreshold == 4095 );
    assert( cfgClampedLow.hasSlapThreshold && std::abs( cfgClampedLow.slapThreshold - 0.5f ) < 0.01f );

    const std::string jsonOutOfRangeHigh = R"({
        "slap_thresh": 99.0
    })";
    PartialConfig cfgClampedHigh;
    assert( WebUtils::parsePartialConfig( jsonOutOfRangeHigh, cfgClampedHigh ) );
    assert( cfgClampedHigh.hasSlapThreshold && std::abs( cfgClampedHigh.slapThreshold - 5.0f ) < 0.01f );

    std::cout << "PASS: testHardwareConfigParsing\n";
}

void testMarqueeFormatting()
{
    // Label + value <= 21 chars: fits static
    const std::string shortField = WebUtils::formatMarqueeField( "AP: ", "Console50-Setup", 21, 0 );
    assert( shortField == "AP: Console50-Setup" );

    // Exact 21 chars
    const std::string exactField = WebUtils::formatMarqueeField( "IP: ", "192.168.123.456.7", 21, 0 );
    assert( exactField == "IP: 192.168.123.456.7" );
    assert( exactField.length() == 21 );

    // Label + value > 21 chars: scrolls value
    const std::string longVal = "ThisIsAVeryLongAccessPointName";
    const std::string scrolled0 = WebUtils::formatMarqueeField( "AP: ", longVal, 21, 0 );
    assert( scrolled0.length() <= 21 );
    assert( scrolled0.rfind( "AP: ", 0 ) == 0 );

    const std::string scrolled1 = WebUtils::formatMarqueeField( "AP: ", longVal, 21, 1 );
    assert( scrolled1.length() <= 21 );
    assert( scrolled1.rfind( "AP: ", 0 ) == 0 );
    assert( scrolled0 != scrolled1 );

    // Edge cases: maxChars = 0 or maxChars < label.length()
    assert( WebUtils::formatMarqueeField( "VeryLongLabel: ", "Value", 0, 0 ) == "" );
    assert( WebUtils::formatMarqueeField( "VeryLongLabel: ", "Value", 5, 0 ) == "VeryL" );

    std::cout << "PASS: testMarqueeFormatting\n";
}

void testNetworkWorkerHostnameAndTimerMode()
{
    auto &nw = NetworkWorker::instance();
    // Default hostname and timer mode
    assert( nw.getHostname() == "bot50" );
    assert( nw.getTimerMode() == 0 );

    nw.saveHostname( "my-friendly-bot" );
    assert( nw.getHostname() == "my-friendly-bot" );

    nw.saveTimerMode( 1 );
    assert( nw.getTimerMode() == 1 );

    // Reset back to default
    nw.saveHostname( "bot50" );
    nw.saveTimerMode( 0 );
    assert( nw.getHostname() == "bot50" );
    assert( nw.getTimerMode() == 0 );

    std::cout << "PASS: testNetworkWorkerHostnameAndTimerMode\n";
}

void testNetworkWorkerPowerProfile()
{
    auto &nw = NetworkWorker::instance();
    TimeService ts;
    nw.begin( &ts, nullptr );

    // Default power profile is Usb
    assert( nw.getPowerProfile() == PowerProfile::Usb );
    assert( !ts.isTimeSynced() );

    // Setting same profile (Usb) should early return and be a no-op
    nw.setPowerProfile( PowerProfile::Usb );
    assert( nw.getPowerProfile() == PowerProfile::Usb );

    // Start AP to verify AP is shut down when switching to Battery
    ConfigServer::instance().startAp( "FriendlyBot-Test" );
    assert( ConfigServer::instance().isApActive() );

    // Switch to Battery profile
    nw.setPowerProfile( PowerProfile::Battery );
    assert( nw.getPowerProfile() == PowerProfile::Battery );
    // AP should be deactivated
    assert( !ConfigServer::instance().isApActive() );

    // Redundant switch to Battery profile should early return and be a no-op
    nw.setPowerProfile( PowerProfile::Battery );
    assert( nw.getPowerProfile() == PowerProfile::Battery );

    // Trigger battery sync while on Battery
    nw.triggerBatterySync();
    assert( ts.isTimeSynced() );

    // Reset sync flag before next test
    ts.markSynced( false );
    assert( !ts.isTimeSynced() );

    // Switch back to Usb profile
    nw.setPowerProfile( PowerProfile::Usb );
    assert( nw.getPowerProfile() == PowerProfile::Usb );

    // Trigger sync while on Usb profile (should not mark synced)
    nw.triggerBatterySync();
    assert( !ts.isTimeSynced() );

    std::cout << "PASS: testNetworkWorkerPowerProfile\n";
}

int main()
{
    std::cout << "Running WebUtils native tests...\n";
    testSanitizeHostname();
    testPlainTextToJsonSentences();
    testPartialConfigParsing();
    testHardwareConfigParsing();
    testMarqueeFormatting();
    testNetworkWorkerHostnameAndTimerMode();
    testNetworkWorkerPowerProfile();
    std::cout << "✅ ALL WEBUTILS TESTS PASSED!\n";
    return 0;
}
