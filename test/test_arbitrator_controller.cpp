#include "controllers/arbitratorController.h"
#include "display.h"
#include "drivers/motionSensor.h"
#include "drivers/potentiometer.h"
#include "i18n.h"

#include <cassert>
#include <iostream>

struct TestHarness
{
    DisplayManager display;
    Potentiometer pot{ 2, 1.0f, 0 };
    MotionSensor motion;
    ArbitratorController controller;

    TestHarness() :
        controller( display, pot, motion )
    {
        setMockMillis( 0 );
        I18n::instance().init( Language::Italian );
        display.setup();
        pot.setup();
        controller.init();
    }
};

void testInitialState()
{
    TestHarness h;
    assert( !h.controller.isActive() );
    assert( h.controller.getState() == ArbitratorState::Idle );
    std::cout << "PASS: testInitialState\n";
}

void testCategoryBrowsingAndLockDebounce()
{
    TestHarness h;
    h.pot.updateRaw( 0 ); // Category 0
    h.controller.activate();
    h.controller.onPotMoved();

    assert( h.controller.isActive() );
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );
    assert( h.controller.getSelectedCategory() == 0 );

    // Advance mock time 500ms -> should still be browsing
    advanceMockMillis( 500 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );

    // Move pot to Category 1 at 600ms -> timer resets
    advanceMockMillis( 100 );
    h.pot.updateRaw( 1200 ); // Category 1
    h.controller.onPotMoved();
    assert( h.controller.getSelectedCategory() == 1 );
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );

    // Advance mock time 900ms -> still browsing
    advanceMockMillis( 900 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );

    // Advance another 150ms (total 1050ms without movement) -> transitions to CategoryLocked
    advanceMockMillis( 150 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );
    std::cout << "PASS: testCategoryBrowsingAndLockDebounce\n";
}

void testCategoryLockedInactivityTimeout()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    h.controller.onPotMoved();

    // Lock the category (wait 1050ms)
    advanceMockMillis( 1050 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    // Advance 14900ms (total in locked: 14900ms) -> still locked
    advanceMockMillis( 14900 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    // Advance another 150ms (total in locked: 15050ms >= 15000ms) -> returns to Idle
    advanceMockMillis( 150 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Idle );
    assert( !h.controller.isActive() );
    std::cout << "PASS: testCategoryLockedInactivityTimeout\n";
}

void testCategoryLockedPotMovedReturnsToBrowsing()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    h.controller.onPotMoved();

    // Lock category 0
    advanceMockMillis( 1050 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );
    assert( h.controller.getSelectedCategory() == 0 );

    // Move pot to Category 2
    h.pot.updateRaw( 2400 );
    h.controller.onPotMoved();
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );
    assert( h.controller.getSelectedCategory() == 2 );

    // Lock category 2
    advanceMockMillis( 1050 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );
    assert( h.controller.getSelectedCategory() == 2 );
    std::cout << "PASS: testCategoryLockedPotMovedReturnsToBrowsing\n";
}

void testAllCategoryIndices()
{
    TestHarness h;
    h.controller.activate();

    // Category 0
    h.pot.updateRaw( 0 );
    h.controller.onPotMoved();
    assert( h.controller.getSelectedCategory() == 0 );

    // Category 1
    h.pot.updateRaw( 1200 );
    h.controller.onPotMoved();
    assert( h.controller.getSelectedCategory() == 1 );

    // Category 2
    h.pot.updateRaw( 2400 );
    h.controller.onPotMoved();
    assert( h.controller.getSelectedCategory() == 2 );

    // Category 3
    h.pot.updateRaw( 3800 );
    h.controller.onPotMoved();
    assert( h.controller.getSelectedCategory() == 3 );

    std::cout << "PASS: testAllCategoryIndices\n";
}

void testEnglishLocalizationRendering()
{
    TestHarness h;
    I18n::instance().setLanguage( Language::English );
    h.pot.updateRaw( 0 );
    h.controller.activate();
    h.controller.onPotMoved();
    h.controller.update();

    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );
    advanceMockMillis( 1050 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    // Test prompt blinking (500ms on, 500ms off)
    advanceMockMillis( 250 );
    h.controller.update();
    advanceMockMillis( 500 );
    h.controller.update();

    std::cout << "PASS: testEnglishLocalizationRendering\n";
}

void testSpinAndVerdictTransitions()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    // Trigger spin via button
    h.controller.onActionButton();
    assert( h.controller.getState() == ArbitratorState::Spinning );

    // Advance 1500ms (during spin)
    advanceMockMillis( 1500 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Spinning );

    // Advance past 2500ms spin duration
    advanceMockMillis( 1100 ); // total 2600ms
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // Re-spin via shake in Verdict
    h.controller.onShake();
    assert( h.controller.getState() == ArbitratorState::Spinning );

    // Complete re-spin to Verdict
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // Pot movement in Verdict returns to CategoryBrowsing
    h.pot.updateRaw( 2000 );
    h.controller.onPotMoved();
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );
    std::cout << "PASS: testSpinAndVerdictTransitions\n";
}

void testInactivityTimeoutReturnsToIdle()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    // 15 seconds of inactivity in CategoryLocked returns to Idle
    advanceMockMillis( 15100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Idle );
    assert( !h.controller.isActive() );
    assert( !h.display.isPoweredOn() );

    // Also test 15 seconds in Verdict returns to Idle
    h.controller.activate();
    assert( h.display.isPoweredOn() );
    advanceMockMillis( 1100 );
    h.controller.update();
    h.controller.onActionButton();
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    advanceMockMillis( 15100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Idle );
    assert( !h.controller.isActive() );
    assert( !h.display.isPoweredOn() );
    std::cout << "PASS: testInactivityTimeoutReturnsToIdle\n";
}

void testSpinIgnoresPotMovement()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );
    assert( h.controller.getSelectedCategory() == 0 );

    h.controller.onActionButton();
    assert( h.controller.getState() == ArbitratorState::Spinning );

    // Moving pot during spin should be ignored
    h.pot.updateRaw( 3000 );
    h.controller.onPotMoved();
    assert( h.controller.getState() == ArbitratorState::Spinning );
    assert( h.controller.getSelectedCategory() == 0 );

    std::cout << "PASS: testSpinIgnoresPotMovement\n";
}

void testVerdictNonRepeating()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );

    size_t lastVerdict = 255;
    for( int i = 0; i < 20; ++i ) {
        h.controller.onActionButton();
        assert( h.controller.getState() == ArbitratorState::Spinning );
        advanceMockMillis( 2600 );
        h.controller.update();
        assert( h.controller.getState() == ArbitratorState::Verdict );

        const size_t currentVerdict = h.controller.getVerdictIndex();
        assert( currentVerdict != lastVerdict );
        lastVerdict = currentVerdict;
    }

    std::cout << "PASS: testVerdictNonRepeating\n";
}

void testVerdictDisplayInversion()
{
    TestHarness h;
    h.pot.updateRaw( 0 );
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    h.controller.onActionButton();
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // At start of verdict (0..149ms), invert is false
    assert( !h.display.getDriver().isInverted() );

    // At 160ms (150..299ms), invert pulses to true
    advanceMockMillis( 160 );
    h.controller.update();
    assert( h.display.getDriver().isInverted() );

    // At 350ms (>=300ms), invert is back to false
    advanceMockMillis( 190 ); // total 350ms in Verdict
    h.controller.update();
    assert( !h.display.getDriver().isInverted() );

    std::cout << "PASS: testVerdictDisplayInversion\n";
}

void testCategoryChangeResetsLastVerdictIndex()
{
    TestHarness h;
    h.pot.updateRaw( 0 ); // Category 0
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    h.controller.onActionButton();
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // lastVerdictIndex should match current verdictIndex after spin
    assert( h.controller.getLastVerdictIndex() == h.controller.getVerdictIndex() );
    assert( h.controller.getLastVerdictIndex() != 255 );

    // Move potentiometer to Category 1
    h.pot.updateRaw( 1200 ); // Category 1
    h.controller.onPotMoved();
    assert( h.controller.getState() == ArbitratorState::CategoryBrowsing );
    assert( h.controller.getSelectedCategory() == 1 );
    assert( h.controller.getLastVerdictIndex() == 255 );

    std::cout << "PASS: testCategoryChangeResetsLastVerdictIndex\n";
}

void testActivateStopsSentenceScroll()
{
    TestHarness h;
    // Simulate active sentence scroll on display
    h.display.displaySentence( "A very long sentence scrolling on screen before entering oracle mode.", 30, 1000 );
    assert( h.display.isVerticalScrollActive() );
    assert( h.display.isSentenceActive( 1000 ) );

    // Activating arbitrator must stop sentence scroll immediately
    h.controller.activate();
    assert( !h.display.isVerticalScrollActive() );
    assert( !h.display.isSentenceActive( 1000 ) );

    // Complete spin to verdict
    advanceMockMillis( 1100 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::CategoryLocked );
    h.controller.onActionButton();
    assert( h.controller.getState() == ArbitratorState::Spinning );
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // Re-spin via action button or shake works in Verdict state
    h.controller.onActionButton();
    assert( h.controller.getState() == ArbitratorState::Spinning );

    std::cout << "PASS: testActivateStopsSentenceScroll\n";
}

void testVerdictFontSizeAndScrolling()
{
    TestHarness h;
    h.pot.updateRaw( 0 ); // Category 0 (Food: "Pizza", "Pasta", etc. - <= 3 lines)
    h.controller.activate();
    advanceMockMillis( 1100 );
    h.controller.update();
    h.controller.onActionButton();
    advanceMockMillis( 2600 );
    h.controller.update();
    assert( h.controller.getState() == ArbitratorState::Verdict );

    // Standard verdict fits within 3 lines: static font size 2 (not scrolling)
    assert( !h.display.isVerticalScrollActive() );
    assert( !h.display.getBodyLines().empty() );
    assert( h.display.getBodyLines().size() <= 3 );

    // Test long verdict triggers vertical scrolling just like aphorisms
    h.display.displaySentence(
            "A very long custom oracle verdict that requires multiple lines and scrolls upward smoothly.", 30, 1000 );
    assert( h.display.isVerticalScrollActive() );
    assert( h.display.isSentenceActive( 1000 ) );

    std::cout << "PASS: testVerdictFontSizeAndScrolling\n";
}

int main()
{
    testInitialState();
    testCategoryBrowsingAndLockDebounce();
    testCategoryLockedInactivityTimeout();
    testCategoryLockedPotMovedReturnsToBrowsing();
    testAllCategoryIndices();
    testEnglishLocalizationRendering();
    testSpinAndVerdictTransitions();
    testInactivityTimeoutReturnsToIdle();
    testSpinIgnoresPotMovement();
    testVerdictNonRepeating();
    testVerdictDisplayInversion();
    testCategoryChangeResetsLastVerdictIndex();
    testActivateStopsSentenceScroll();
    testVerdictFontSizeAndScrolling();
    std::cout << "All ArbitratorController tests passed!\n";
    return 0;
}
