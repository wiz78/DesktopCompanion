#include <cstdlib>
#include <iostream>

int main()
{
    std::cout << "Running all native unit test suites...\n";
    const int r1 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks test/test_pins.cpp -o test/test_pins && ./test/test_pins" );
    const int r2 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/button.cpp test/test_button.cpp -o test/test_button && "
            "./test/test_button"
            );
    const int r3 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/potentiometer.cpp test/test_potentiometer.cpp -o "
            "test/test_potentiometer && ./test/test_potentiometer"
            );
    const int r4 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/ldrSensor.cpp src/drivers/powerSense.cpp test/test_ldr_powersense.cpp "
            "-o test/test_ldr && ./test/test_ldr"
            );
    const int r5 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/buzzer.cpp test/test_buzzer.cpp -o test/test_buzzer && "
            "./test/test_buzzer"
            );
    const int r6 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/motionSensor.cpp test/test_motion_sensor.cpp -o test/test_motion && "
            "./test/test_motion"
            );
    const int r7 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/drivers/proximitySensor.cpp test/test_desk_occupant.cpp -o "
            "test/test_occupant && ./test/test_occupant"
            );
    const int r8 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/display.cpp "
            "src/textFormatter.cpp src/services/webUtils.cpp test/test_display_manager.cpp -o "
            "test/test_display && ./test/test_display"
            );
    const int r9 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/i18n.cpp test/test_i18n.cpp -o test/test_i18n && ./test/test_i18n"
            );
    const int r10 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/textFormatter.cpp test/test_text_formatter.cpp -o test/test_text_formatter && "
            "./test/test_text_formatter"
            );
    const int r11 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/i18n.cpp src/sentenceEngine.cpp test/test_sentence_engine.cpp -o "
            "test/test_sentence_engine && ./test/test_sentence_engine"
            );
    const int r12 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks src/i18n.cpp src/services/timeService.cpp test/test_time_service.cpp -o "
            "test/test_time_service && ./test/test_time_service"
            );
    const int r13 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/services/weatherService.cpp "
            "test/test_weather_parser.cpp -o test/test_weather_parser && ./test/test_weather_parser"
            );
    const int r14 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/i18n.cpp src/display.cpp "
            "src/textFormatter.cpp src/sentenceEngine.cpp src/drivers/proximitySensor.cpp src/drivers/ldrSensor.cpp "
            "src/drivers/motionSensor.cpp src/services/timeService.cpp src/services/weatherService.cpp src/services/webUtils.cpp "
            "src/services/configServer.cpp src/services/networkWorker.cpp "
            "src/controllers/deskCompanionController.cpp test/test_desk_companion_controller.cpp -o test/test_companion && ./test/test_companion"
            );
    const int r15 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/display.cpp src/textFormatter.cpp "
            "src/i18n.cpp src/drivers/buzzer.cpp src/drivers/motionSensor.cpp src/drivers/ldrSensor.cpp src/drivers/powerSense.cpp "
            "src/services/webUtils.cpp src/controllers/gagController.cpp "
            "test/test_gag_controller.cpp -o test/test_gag && ./test/test_gag"
            );
    const int r16 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/display.cpp src/textFormatter.cpp "
            "src/i18n.cpp src/drivers/potentiometer.cpp src/drivers/motionSensor.cpp src/services/webUtils.cpp "
            "src/controllers/arbitratorController.cpp "
            "test/test_arbitrator_controller.cpp -o test/test_arbitrator && ./test/test_arbitrator"
            );
    const int r17 = std::system(
            "clang++ -std=c++20 -Iinclude -Itest/mocks test/test_timer_presets.cpp -o test/test_presets && ./test/test_presets"
            );
    const int r18 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/display.cpp src/textFormatter.cpp "
            "src/i18n.cpp src/services/webUtils.cpp "
            "src/controllers/renderers/digitsBarRenderer.cpp src/controllers/renderers/digitalSandRenderer.cpp "
            "test/test_timer_renderers.cpp -o test/test_renderers && ./test/test_renderers"
            );
    const int r19 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/display.cpp src/textFormatter.cpp "
            "src/i18n.cpp src/drivers/potentiometer.cpp src/drivers/buzzer.cpp src/drivers/motionSensor.cpp src/drivers/proximitySensor.cpp "
            "src/services/webUtils.cpp src/controllers/renderers/digitsBarRenderer.cpp src/controllers/renderers/digitalSandRenderer.cpp "
            "src/controllers/timerController.cpp test/test_timer_controller.cpp -o test/test_timer_controller && ./test/test_timer_controller"
            );
    const int r20 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/i18n.cpp src/sentenceEngine.cpp "
            "src/services/timeService.cpp src/services/weatherService.cpp src/services/configServer.cpp src/services/networkWorker.cpp "
            "src/services/webUtils.cpp test/test_web_utils.cpp -o test/test_web_utils && ./test/test_web_utils"
            );
    const int r21 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks src/i18n.cpp src/sentenceEngine.cpp "
            "src/services/timeService.cpp src/services/weatherService.cpp src/services/configServer.cpp src/services/networkWorker.cpp "
            "src/display.cpp src/textFormatter.cpp src/services/webUtils.cpp src/drivers/buzzer.cpp src/drivers/powerSense.cpp "
            "src/drivers/motionSensor.cpp src/services/powerManager.cpp test/test_power_management.cpp -o test/test_power && ./test/test_power"
            );
    const int r22 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks "
            "src/display.cpp src/textFormatter.cpp src/i18n.cpp src/sentenceEngine.cpp "
            "src/drivers/button.cpp src/drivers/potentiometer.cpp src/drivers/buzzer.cpp "
            "src/drivers/motionSensor.cpp src/drivers/proximitySensor.cpp src/drivers/ldrSensor.cpp "
            "src/drivers/powerSense.cpp src/services/timeService.cpp src/services/weatherService.cpp "
            "src/services/configServer.cpp src/services/networkWorker.cpp src/services/powerManager.cpp "
            "src/services/webUtils.cpp src/hardwareManager.cpp src/controllers/renderers/digitsBarRenderer.cpp "
            "src/controllers/renderers/digitalSandRenderer.cpp src/controllers/deskCompanionController.cpp "
            "src/controllers/gagController.cpp src/controllers/arbitratorController.cpp "
            "src/controllers/timerController.cpp src/controllers/systemOrchestrator.cpp "
            "test/test_system_orchestrator.cpp -o test/test_system_orchestrator && ./test/test_system_orchestrator"
            );
    const int r23 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks "
            "src/display.cpp src/textFormatter.cpp src/i18n.cpp src/sentenceEngine.cpp "
            "src/drivers/button.cpp src/drivers/potentiometer.cpp src/drivers/buzzer.cpp "
            "src/drivers/motionSensor.cpp src/drivers/proximitySensor.cpp src/drivers/ldrSensor.cpp "
            "src/drivers/powerSense.cpp src/services/timeService.cpp src/services/weatherService.cpp "
            "src/services/configServer.cpp src/services/networkWorker.cpp src/services/powerManager.cpp "
            "src/services/webUtils.cpp src/hardwareManager.cpp src/controllers/renderers/digitsBarRenderer.cpp "
            "src/controllers/renderers/digitalSandRenderer.cpp src/controllers/deskCompanionController.cpp "
            "src/controllers/gagController.cpp src/controllers/arbitratorController.cpp "
            "src/controllers/timerController.cpp src/controllers/systemOrchestrator.cpp "
            "src/services/serialCli.cpp "
            "test/test_serial_cli.cpp -o test/test_serial_cli && ./test/test_serial_cli"
            );
    const int r24 = std::system(
            "clang++ -std=c++20 -Iinclude -I.pio/libdeps/esp32-s3-mini/ArduinoJson/src -Itest/mocks "
            "src/display.cpp src/textFormatter.cpp src/i18n.cpp src/sentenceEngine.cpp "
            "src/drivers/button.cpp src/drivers/potentiometer.cpp src/drivers/buzzer.cpp "
            "src/drivers/motionSensor.cpp src/drivers/proximitySensor.cpp src/drivers/ldrSensor.cpp "
            "src/drivers/powerSense.cpp src/services/timeService.cpp src/services/weatherService.cpp "
            "src/services/configServer.cpp src/services/networkWorker.cpp src/services/powerManager.cpp "
            "src/services/webUtils.cpp src/hardwareManager.cpp "
            "test/test_hardware_manager.cpp -o test/test_hardware_manager && ./test/test_hardware_manager"
            );

    std::system(
            "rm -f test/test_pins test/test_button test/test_potentiometer test/test_ldr test/test_buzzer test/test_motion "
            "test/test_occupant test/test_display test/test_i18n test/test_text_formatter test/test_sentence_engine test/test_time_service "
            "test/test_weather_parser test/test_companion test/test_gag test/test_arbitrator test/test_presets test/test_renderers test/test_timer_controller "
            "test/test_web_utils test/test_power test/test_system_orchestrator test/test_serial_cli test/test_hardware_manager"
            );

    if( r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0 && r5 == 0 && r6 == 0 && r7 == 0 && r8 == 0 && r9 == 0 && r10 == 0 &&
        r11 == 0 && r12 == 0 && r13 == 0 && r14 == 0 && r15 == 0 && r16 == 0 && r17 == 0 && r18 == 0 && r19 == 0 && r20
        == 0 && r21 == 0 && r22 == 0 && r23 == 0 && r24 == 0 ) {
        std::cout << "\n✅ ALL 24 TEST SUITES PASSED SUCCESSFULLY!\n";
        return 0;
    }

    std::cerr << "\n❌ SOME TESTS FAILED!\n";
    return 1;
}
