#include "controllers/renderers/digitsBarRenderer.h"
#include "controllers/renderers/digitalSandRenderer.h"
#include "display.h"
#include "i18n.h"

#include <cassert>
#include <iostream>

void test_digits_bar_renderer()
{
    DisplayManager display;
    display.setup();
    I18n::instance().init();

    DigitsBarRenderer renderer;

    // Setting mode
    renderer.renderSetting( display, 300 );
    // Running mode
    renderer.renderRunning( display, 240, 300 );
    // Paused mode
    renderer.renderPaused( display, 240, 300, true );
    renderer.renderPaused( display, 240, 300, false );
    // Alarming mode
    renderer.renderAlarming( display, true );
    renderer.renderAlarming( display, false );

    std::cout << "PASS: test_digits_bar_renderer\n";
}

void test_digital_sand_renderer()
{
    DisplayManager display;
    display.setup();
    I18n::instance().init();

    DigitalSandRenderer renderer;

    // Setting mode
    renderer.renderSetting( display, 120 );
    // Running mode with tilt update
    renderer.updatePhysics( 0.0f, 1.0f ); // gravity pointing straight down
    renderer.renderRunning( display, 60, 120 );
    // Paused mode
    renderer.renderPaused( display, 60, 120, true );
    // Alarming mode
    renderer.renderAlarming( display, true );

    std::cout << "PASS: test_digital_sand_renderer\n";
}

int main()
{
    std::cout << "Running Timer Renderers Native Tests...\n";
    test_digits_bar_renderer();
    test_digital_sand_renderer();
    std::cout << "✅ ALL TIMER RENDERER TESTS PASSED!\n";
    return 0;
}
