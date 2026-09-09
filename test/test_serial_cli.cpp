#include "controllers/systemOrchestrator.h"
#include "hardwareManager.h"
#include "i18n.h"
#include "sentenceEngine.h"
#include "services/configServer.h"
#include "services/networkWorker.h"
#include "services/powerManager.h"
#include "services/serialCli.h"
#include "services/timeService.h"
#include "services/weatherService.h"

#include <Arduino.h>
#include <cassert>
#include <iostream>
#include <string>

struct SerialCliTestHarness
{
    HardwareManager &hal = HardwareManager::instance();
    TimeService timeService;
    WeatherService weatherService;
    SentenceEngine sentenceEngine;
    NetworkWorker &network = NetworkWorker::instance();
    PowerManager &power = PowerManager::instance();

    DeskCompanionController companion;
    GagController gag;
    ArbitratorController arbitrator;
    TimerController timer;
    SystemOrchestrator orchestrator;

    SerialCli cli;

    SerialCliTestHarness() :
        companion(
                hal.getDisplay(),
                timeService,
                weatherService,
                sentenceEngine,
                hal.getProximity(),
                hal.getLdr(),
                hal.getMotion()
                ),
        gag(
                hal.getDisplay(),
                hal.getBuzzer(),
                hal.getMotion(),
                hal.getLdr(),
                hal.getPowerSense()
                ),
        arbitrator(
                hal.getDisplay(),
                hal.getRightPot(),
                hal.getMotion()
                ),
        timer(
                hal.getDisplay(),
                hal.getLeftPot(),
                hal.getBuzzer(),
                hal.getMotion(),
                hal.getProximity()
                ),
        orchestrator( hal, companion, gag, arbitrator, timer, sentenceEngine )
    {
        setMockMillis( 0 );
        Serial.clearCapturedOutput();
        Serial.clearInputBuffer();
        I18n::instance().init( Language::Italian );
        hal.begin();
        sentenceEngine.begin();

        companion.init( false );
        gag.init();
        arbitrator.init();
        timer.init();
        orchestrator.init( false );

        power.init( hal.getPowerSense(), hal.getDisplay(), hal.getBuzzer(), network );
        power.resetActivity( 0 );

        cli.init(
                &orchestrator,
                &hal,
                &network,
                &timeService,
                &weatherService,
                &sentenceEngine,
                &power,
                &companion,
                &gag,
                &arbitrator,
                &timer
                );
    }
};

void testDiagCommand()
{
    std::cout << "Testing diag command...\n";
    SerialCliTestHarness h;
    Serial.clearCapturedOutput();

    h.cli.processCommand( "diag", 1000 );
    const std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Diagnostics ---" ) != std::string::npos );
    assert( out.find( "Left Pot:" ) != std::string::npos );
    assert( out.find( "Right Pot:" ) != std::string::npos );
    assert( out.find( "LDR Light:" ) != std::string::npos );
    assert( out.find( "Power:" ) != std::string::npos );
    assert( out.find( "Temp:" ) != std::string::npos );
    assert( out.find( "Distance:" ) != std::string::npos );
    assert( h.power.getLastActivityMs() == 1000 );
    std::cout << "PASS: testDiagCommand\n";
}

void testJokesInfoAndJoke()
{
    std::cout << "Testing jokes_info and joke commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "jokes_info", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Sentence Engine Info ---" ) != std::string::npos );
    assert( out.find( "Custom Sentences Active:" ) != std::string::npos );
    assert( out.find( "Total Sentences:" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "joke", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[SENTENCE]" ) != std::string::npos );
    std::cout << "PASS: testJokesInfoAndJoke\n";
}

void testLanguageSwitching()
{
    std::cout << "Testing lang command...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "lang en", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[I18N] Language changed to: en" ) != std::string::npos );
    assert( I18n::instance().getLanguage() == Language::English );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "lang it", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[I18N] Language changed to: it" ) != std::string::npos );
    assert( I18n::instance().getLanguage() == Language::Italian );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "lang zz", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[I18N] Unknown language: 'zz'" ) != std::string::npos );
    std::cout << "PASS: testLanguageSwitching\n";
}

void testWifiAndCredentials()
{
    std::cout << "Testing wifi commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "wifi MySSID MyPass123", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Saved WiFi SSID: MySSID" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "wifi OpenWiFi", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Saved open WiFi SSID: OpenWiFi" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "wifi", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Usage: wifi <SSID> <PASSWORD>" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "wifi_clear", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] WiFi credentials cleared." ) != std::string::npos );
    std::cout << "PASS: testWifiAndCredentials\n";
}

void testWeatherAndTz()
{
    std::cout << "Testing weather and tz commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "weather 45.4642 9.1900 Key789", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Saved Weather Config for Coords: 45.4642, 9.1900" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "weather", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Usage: weather <LAT> <LON> <API_KEY>" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "tz CET-1CEST,M3.5.0,M10.5.0/3", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Saved Timezone:" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "tz", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Usage: tz <POSIX_TZ_STRING>" ) != std::string::npos );
    std::cout << "PASS: testWeatherAndTz\n";
}

void testNetStatusAndPortal()
{
    std::cout << "Testing net_status and portal commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "net_status", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Network & Services Status ---" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "portal_status", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "--- Portal Status ---" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "portal_ap", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Access Point started manually." ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "portal_sta", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Forced immediate station retry / fetch." ) != std::string::npos );
    std::cout << "PASS: testNetStatusAndPortal\n";
}

void testGlitchAndGagInfo()
{
    std::cout << "Testing glitch and gag_info commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "glitch", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Gag triggered (forced)." ) != std::string::npos );
    assert( h.gag.isActive() );
    assert( h.orchestrator.getTargetMode() == SystemMode::SlapGag );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "gag_info", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "--- Gag Controller Info ---" ) != std::string::npos );
    assert( out.find( "Active: YES" ) != std::string::npos );
    std::cout << "PASS: testGlitchAndGagInfo\n";
}

void testArbitratorCommands()
{
    std::cout << "Testing arbitrator commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "oracle", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Arbitrator (Oracle) mode activated." ) != std::string::npos );
    assert( h.arbitrator.isActive() );
    assert( h.orchestrator.getTargetMode() == SystemMode::Arbitrator );

    // Complete CRT transition to Arbitrator mode
    h.orchestrator.update( 1090 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );

    // Return to DeskCompanion to test spin during mode transition
    h.orchestrator.requestMode( SystemMode::DeskCompanion, 2000 );
    h.orchestrator.update( 2090 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    Serial.clearCapturedOutput();
    setMockMillis( 3000 );
    h.cli.processCommand( "spin", 3000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Arbitrator spin triggered." ) != std::string::npos );
    assert( h.arbitrator.getState() == ArbitratorState::Spinning );
    assert( h.orchestrator.getTargetMode() == SystemMode::Arbitrator );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Advancing time through the 85 ms CRT transition maintains spinning state
    setMockMillis( 3090 );
    h.orchestrator.update( 3090 );
    assert( h.orchestrator.getMode() == SystemMode::Arbitrator );
    assert( h.arbitrator.getState() == ArbitratorState::Spinning );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "arbitrator_info", 3090 );
    out = Serial.getCapturedOutput();
    assert( out.find( "--- Arbitrator Controller Info ---" ) != std::string::npos );
    assert( out.find( "Active: YES" ) != std::string::npos );
    std::cout << "PASS: testArbitratorCommands\n";
}

void testTimerCommands()
{
    std::cout << "Testing timer commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "timer", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Timer mode activated (Setting)." ) != std::string::npos );
    assert( h.timer.isActive() );
    assert( h.orchestrator.getTargetMode() == SystemMode::Timer );

    // Complete CRT transition to Timer mode
    h.orchestrator.update( 1090 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );

    // Return to DeskCompanion to test timer 90 starting while in another mode
    h.orchestrator.requestMode( SystemMode::DeskCompanion, 2000 );
    h.orchestrator.update( 2090 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    Serial.clearCapturedOutput();
    setMockMillis( 3000 );
    h.cli.processCommand( "timer 90", 3000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Timer started with 90 seconds." ) != std::string::npos );
    assert( h.timer.getState() == TimerState::Running );
    assert( h.timer.getRemainingSeconds() == 90 );
    assert( h.orchestrator.getTargetMode() == SystemMode::Timer );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Advancing time through the 85 ms CRT transition maintains running timer state and remaining seconds
    setMockMillis( 3090 );
    h.orchestrator.update( 3090 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.getState() == TimerState::Running );
    assert( h.timer.getRemainingSeconds() == 90 );

    // Return to DeskCompanion to test alarm starting while in another mode
    h.orchestrator.requestMode( SystemMode::DeskCompanion, 4000 );
    h.orchestrator.update( 4090 );
    assert( h.orchestrator.getMode() == SystemMode::DeskCompanion );

    Serial.clearCapturedOutput();
    setMockMillis( 5000 );
    h.cli.processCommand( "alarm", 5000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Alarm triggered (forced)." ) != std::string::npos );
    assert( h.timer.getState() == TimerState::Alarming );
    assert( h.orchestrator.getTargetMode() == SystemMode::Timer );
    assert( h.orchestrator.getMode() == SystemMode::Transitioning );

    // Advancing time through the 85 ms CRT transition maintains alarming state
    setMockMillis( 5090 );
    h.orchestrator.update( 5090 );
    assert( h.orchestrator.getMode() == SystemMode::Timer );
    assert( h.timer.getState() == TimerState::Alarming );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "timer_info", 5090 );
    out = Serial.getCapturedOutput();
    assert( out.find( "--- Timer Controller Info ---" ) != std::string::npos );
    assert( out.find( "Active: YES" ) != std::string::npos );
    std::cout << "PASS: testTimerCommands\n";
}

void testPowerAndSleepCommands()
{
    std::cout << "Testing power_info and sleep commands...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "power_info", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Power Manager Info ---" ) != std::string::npos );
    assert( out.find( "Last Activity:" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "sleep", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Forcing deep sleep..." ) != std::string::npos );
    std::cout << "PASS: testPowerAndSleepCommands\n";
}

void testHwTestActivationAndChirp()
{
    std::cout << "Testing hw_test activation and chirp...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "hw_test", 1000 );
    const std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Hardware Checkout & Telemetry Test ---" ) != std::string::npos );
    assert( h.hal.getBuzzer().isSounding() );
    assert( h.cli.isHwTestStreaming() );
    std::cout << "PASS: testHwTestActivationAndChirp\n";
}

void testHwTestTelemetryStreaming10Hz()
{
    std::cout << "Testing hw_test 10Hz telemetry streaming...\n";
    SerialCliTestHarness h;

    h.hal.getLeftPot().updateRaw( 2048 );
    h.hal.getRightPot().updateRaw( 1000 );
    h.hal.getMotion().feedRawData( 0.05f, -0.1f, 9.78f, 23.4f, 1000 );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "hw_test", 1000 );
    assert( h.cli.isHwTestStreaming() );

    // At 1050ms (only 50ms elapsed), no telemetry line emitted yet
    Serial.clearCapturedOutput();
    h.cli.update( 1050 );
    assert( Serial.getCapturedOutput().empty() );

    // At 1100ms (100ms elapsed), telemetry line emitted
    h.cli.update( 1100 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "[HW_TEST] PotL:" ) != std::string::npos );
    assert( out.find( "PotR:" ) != std::string::npos );
    assert( out.find( "LDR:" ) != std::string::npos );
    assert( out.find( "Power:" ) != std::string::npos );
    assert( out.find( "ToF:" ) != std::string::npos );
    assert( out.find( "Temp:" ) != std::string::npos );
    assert( out.find( "Acc:" ) != std::string::npos );
    assert( out.find( "BtnA:" ) != std::string::npos );
    assert( out.find( "BtnR:" ) != std::string::npos );

    // At 1200ms, next line emitted
    Serial.clearCapturedOutput();
    h.cli.update( 1200 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[HW_TEST]" ) != std::string::npos );
    std::cout << "PASS: testHwTestTelemetryStreaming10Hz\n";
}

void testHwTestTimeoutAt5000ms()
{
    std::cout << "Testing hw_test 5000ms timeout...\n";
    SerialCliTestHarness h;

    h.cli.processCommand( "hw_test", 1000 );
    assert( h.cli.isHwTestStreaming() );

    // At 5999ms (4999ms elapsed), still streaming
    h.cli.update( 5999 );
    assert( h.cli.isHwTestStreaming() );

    // At 6000ms (5000ms elapsed), test completes
    Serial.clearCapturedOutput();
    h.cli.update( 6000 );
    assert( !h.cli.isHwTestStreaming() );
    const std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Hardware Test Complete ---" ) != std::string::npos );
    std::cout << "PASS: testHwTestTimeoutAt5000ms\n";
}

void testHwTestManualStop()
{
    std::cout << "Testing hw_test manual stop...\n";
    SerialCliTestHarness h;

    h.cli.processCommand( "hw_test", 1000 );
    assert( h.cli.isHwTestStreaming() );

    Serial.clearCapturedOutput();
    h.cli.stopHwTest();
    assert( !h.cli.isHwTestStreaming() );
    const std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Hardware Test Complete ---" ) != std::string::npos );
    std::cout << "PASS: testHwTestManualStop\n";
}

void testHwTestSerialInterruption()
{
    std::cout << "Testing hw_test serial input interruption...\n";
    SerialCliTestHarness h;

    h.cli.processCommand( "hw_test", 1000 );
    assert( h.cli.isHwTestStreaming() );

    // Incoming serial keystroke stops streaming
    Serial.queueInput( "q\n" );
    Serial.clearCapturedOutput();
    h.cli.update( 1050 );
    assert( !h.cli.isHwTestStreaming() );
    const std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Hardware Test Complete ---" ) != std::string::npos );
    std::cout << "PASS: testHwTestSerialInterruption\n";
}

void testUnknownCommandAndHelp()
{
    std::cout << "Testing help and unknown command...\n";
    SerialCliTestHarness h;

    Serial.clearCapturedOutput();
    h.cli.processCommand( "help", 1000 );
    std::string out = Serial.getCapturedOutput();
    assert( out.find( "--- Available Commands ---" ) != std::string::npos );
    assert( out.find( "hw_test" ) != std::string::npos );
    assert( out.find( "diag" ) != std::string::npos );

    Serial.clearCapturedOutput();
    h.cli.processCommand( "foobar999", 1000 );
    out = Serial.getCapturedOutput();
    assert( out.find( "[CLI] Unknown command: 'foobar999'" ) != std::string::npos );
    std::cout << "PASS: testUnknownCommandAndHelp\n";
}

void testActivityResetOnlyOnValidCommands()
{
    std::cout << "Testing activity reset behavior...\n";
    SerialCliTestHarness h;

    h.power.resetActivity( 100 );
    assert( h.power.getLastActivityMs() == 100 );

    // Invalid command does not reset activity
    h.cli.processCommand( "nonsense_command", 500 );
    assert( h.power.getLastActivityMs() == 100 );

    // Valid command resets activity
    h.cli.processCommand( "diag", 800 );
    assert( h.power.getLastActivityMs() == 800 );
    std::cout << "PASS: testActivityResetOnlyOnValidCommands\n";
}

int main()
{
    std::cout << "Running SerialCli Native Tests...\n";
    testDiagCommand();
    testJokesInfoAndJoke();
    testLanguageSwitching();
    testWifiAndCredentials();
    testWeatherAndTz();
    testNetStatusAndPortal();
    testGlitchAndGagInfo();
    testArbitratorCommands();
    testTimerCommands();
    testPowerAndSleepCommands();
    testHwTestActivationAndChirp();
    testHwTestTelemetryStreaming10Hz();
    testHwTestTimeoutAt5000ms();
    testHwTestManualStop();
    testHwTestSerialInterruption();
    testUnknownCommandAndHelp();
    testActivityResetOnlyOnValidCommands();

    std::cout << "\n✅ ALL SERIAL CLI TESTS PASSED!\n";
    return 0;
}
