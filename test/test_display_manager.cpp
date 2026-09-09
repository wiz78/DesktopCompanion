#include "display.h"

#include <cassert>
#include <iostream>

void testStaticTextNoMarquee()
{
    DisplayManager dm;

    // Short text: 5 chars * 6px * 2 = 60px <= 128px
    dm.setScrollingText( "Short", 20, 2, 40, 1000 );
    assert( !dm.isMarqueeActive() );
    assert( dm.isDirty() );

    dm.update( 50 );
    assert( !dm.isDirty() );
}

void testMarqueeProgressionAndWrapping()
{
    DisplayManager dm;

    // Text: 26 chars * 12px = 312px > 128px
    // Max offset before wrap = 312 - 128 + 10 = 194
    const std::string text = "Aphorism that is very long";
    dm.setScrollingText( text, 20, 2, 40, 1000 );

    assert( dm.isMarqueeActive() );
    assert( dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 0 );

    // Frame 0: Marquee initializes pause timer at t = 0
    dm.update( 0 );
    assert( dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 0 );

    // Frame 1: Advance by 500ms -> still in initial 1000ms pause
    dm.update( 500 );
    assert( dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 0 );

    // Frame 2: Advance to 1000ms -> pause expires, marquee starts stepping
    dm.update( 1000 );
    assert( !dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 0 );

    // Frame 3: Advance to 1040ms (1 step of 40ms) -> offset moves by 2px
    dm.update( 1040 );
    assert( !dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 2 );

    // Frame 4: Advance to 1080ms -> offset moves to 4px
    dm.update( 1080 );
    assert( !dm.isMarqueeInPause() );
    assert( dm.getMarqueeOffset() == 4 );

    // Step until marquee reaches and passes the wrap threshold (194)
    // 194 / 2 = 97 steps * 40ms = 3880ms + 1000ms base
    uint32_t simTime = 1080;
    while( dm.getMarqueeOffset() <= 194 && !dm.isMarqueeInPause() ) {
        simTime += 40;
        dm.update( simTime );
    }

    // After stepping past 194, offset wraps to 0 and marquee re-enters pause
    assert( dm.getMarqueeOffset() == 0 );
    assert( dm.isMarqueeInPause() );
    assert( dm.isMarqueeActive() );
}

void testPowerAndSleepControls()
{
    DisplayManager dm;

    assert( dm.isPoweredOn() );

    dm.setPower( false );
    assert( !dm.isPoweredOn() );

    dm.setPower( true );
    assert( dm.isPoweredOn() );

    dm.setBrightnessLevel( 3 );
    dm.setBrightnessLevel( 0 );

    dm.invertDisplay( true );
    dm.invertDisplay( false );
}

void testHeaderAndBody()
{
    DisplayManager dm;

    dm.setHeader( "12:45", "21C" );
    assert( dm.getHeaderLeft() == "12:45" );
    assert( dm.getHeaderRight() == "21C" );
    assert( dm.isDirty() );

    dm.update( 50 );
    assert( !dm.isDirty() );

    dm.setBodyText( { "Row 1", "Row 2" }, 1 );
    assert( dm.isDirty() );

    dm.clearHeader();
    assert( dm.isDirty() );

    // Set scrolling text and ensure clearBody resets marquee
    dm.setScrollingText( "Very long text that triggers marquee mode", 20, 2, 40, 1000 );
    assert( dm.isMarqueeActive() );

    dm.clearBody();
    assert( !dm.isMarqueeActive() );
    assert( dm.isDirty() );

    // Clear everything resets marquee
    dm.setScrollingText( "Another long text string for marquee testing", 20, 2, 40, 1000 );
    assert( dm.isMarqueeActive() );
    dm.clear();
    assert( !dm.isMarqueeActive() );
    assert( dm.isDirty() );
}

void testProgressBarAndRateLimiter()
{
    DisplayManager dm;

    dm.drawProgressBar( 0, 50, 128, 10, 0.5f );
    assert( dm.isDirty() );

    // Negative and overflow clamp coverage
    dm.drawProgressBar( 0, 50, 128, 10, -0.2f );
    dm.drawProgressBar( 0, 50, 128, 10, 1.8f );

    // Update with elapsed < 33ms does not clear dirty
    dm.update( 10 );
    assert( dm.isDirty() );

    // Update with elapsed >= 33ms flushes dirty buffer
    dm.update( 40 );
    assert( !dm.isDirty() );
}

void test_display_sentence_short_wraps()
{
    DisplayManager dm;
    dm.setup();

    dm.displaySentence( "Schiena: Croccante" );
    assert( !dm.isMarqueeActive() );
    assert( dm.getBodyLines().size() == 2 );
    std::cout << "PASS: test_display_sentence_short_wraps\n";
}

void test_display_sentence_long_marquees()
{
    DisplayManager dm;
    dm.setup();

    dm.displaySentence( "There is still no cure for the common birthday. - John Glenn", 30, 1000 );
    assert( dm.isVerticalScrollActive() );
    assert( dm.isSentenceActive( 1000 ) );
    assert( dm.getVerticalScrollOffset() == 0 );

    // During initial pause (4000ms): at 4000ms (elapsed 3000ms), offset is still 0
    dm.update( 4000 );
    assert( dm.getVerticalScrollOffset() == 0 );

    // After initial pause: at 1000 + 4000 + 400 = 5400ms, scrolled 400ms / 40ms/px = 10px
    dm.update( 5400 );
    assert( dm.getVerticalScrollOffset() == 10 );

    // After reaching max offset and during end pause (5000ms):
    const uint32_t duration = dm.getSentenceDurationMs();
    assert( duration > 5000 );
    dm.update( 1000 + duration - 100 );
    assert( dm.isSentenceActive( 1000 + duration - 100 ) );

    // After duration completes:
    dm.update( 1000 + duration + 10 );
    assert( !dm.isSentenceActive( 1000 + duration + 10 ) );

    // Test stopSentenceScroll()
    dm.displaySentence( "Another long sentence that definitely requires more than three lines to display properly.", 30,
                        50000 );
    assert( dm.isVerticalScrollActive() );
    assert( dm.isSentenceActive( 50000 ) );
    dm.stopSentenceScroll();
    assert( !dm.isVerticalScrollActive() );
    assert( !dm.isSentenceActive( 50000 ) );

    std::cout << "PASS: test_display_sentence_long_marquees\n";
}

void testSetupScreenRendering()
{
    DisplayManager dm;
    dm.setup();

    // 1. Default fallback parameters (nullptr)
    dm.displaySetupScreen( nullptr, nullptr, nullptr, 0 );
    assert( !dm.isMarqueeActive() );

    // 2. Normal parameters
    dm.displaySetupScreen( "Console50-Setup", "192.168.4.1", "bot50.local", 0 );
    assert( !dm.isMarqueeActive() );

    // 3. Long parameters that exercise marquee scrolling
    const char *longSsid = "VeryLongAccessPointSSIDNameExceeding21Characters";
    const char *longUrl = "http://verylongdomainnameexceedinglimit.local";
    dm.displaySetupScreen( longSsid, "192.168.4.1", longUrl, 0 );
    dm.displaySetupScreen( longSsid, "192.168.4.1", longUrl, 250 );
    dm.displaySetupScreen( longSsid, "192.168.4.1", longUrl, 500 );
    dm.displaySetupScreen( longSsid, "192.168.4.1", longUrl, 1000 );

    std::cout << "PASS: testSetupScreenRendering\n";
}

void testResetCountdownRendering()
{
    DisplayManager dm;
    dm.setup();

    for( uint8_t s = 3; s >= 1; --s ) {
        dm.displayResetCountdown( s );
        assert( !dm.isMarqueeActive() );
        assert( !dm.isDirty() );
    }

    std::cout << "PASS: testResetCountdownRendering\n";
}

void test_display_toast()
{
    DisplayManager dm;
    dm.init();
    assert( !dm.isToastActive( 1000 ) );

    dm.showToast( "BATTERY MODE", 2000, 1000 );
    assert( dm.isToastActive( 1500 ) );
    assert( dm.isToastActive( 2999 ) );
    assert( !dm.isToastActive( 3001 ) );

    std::cout << "PASS: test_display_toast\n";
}

void test_display_toast_guards_set_header()
{
    DisplayManager dm;
    dm.init();

    dm.setHeader( "12:00", "100%", 500 );
    assert( dm.getHeaderLeft() == "12:00" );
    assert( dm.getHeaderRight() == "100%" );

    dm.showToast( "USB POWER", 2000, 1000 );
    assert( dm.isToastActive( 1000 ) );

    dm.setHeader( "12:01", "99%", 1500 );
    assert( dm.getHeaderLeft() == "12:01" );
    assert( dm.getHeaderRight() == "99%" );
    assert( dm.isToastActive( 1500 ) );

    dm.update( 3000 );
    assert( !dm.isToastActive( 3000 ) );

    std::cout << "PASS: test_display_toast_guards_set_header\n";
}

void testCrtShutterFrames()
{
    std::cout << "Testing CRT Shutter frames...\n";
    Adafruit_SSD1306 rawDisplay( 128, 64, nullptr );
    DisplayManager dm( rawDisplay );
    dm.begin();

    // Frame 0: collapsing curtains
    dm.displayCrtShutter( 0 );
    assert( !dm.isDirty() );
    assert( !dm.isMarqueeActive() );

    // Frame 1: center horizontal line
    dm.displayCrtShutter( 1 );
    assert( !dm.isDirty() );

    // Frame 2: center dot
    dm.displayCrtShutter( 2 );
    assert( !dm.isDirty() );

    // Frame 3: blank/cleared
    dm.displayCrtShutter( 3 );
    assert( !dm.isDirty() );

    std::cout << "PASS: testCrtShutterFrames\n";
}

void testDiagnosticsRendering()
{
    std::cout << "Testing Diagnostics screen rendering...\n";
    Adafruit_SSD1306 rawDisplay( 128, 64, nullptr );
    DisplayManager dm( rawDisplay );
    dm.begin();

    dm.displayDiagnostics( 0.5f, 5.0f, 2, 1200, false, true, 450, 24.5f, false, false );
    assert( !dm.isDirty() );
    assert( !dm.isMarqueeActive() );

    std::cout << "PASS: testDiagnosticsRendering\n";
}

void testHardwareFlipping()
{
    DisplayManager dm;
    assert( !dm.isFlipped() );

    dm.setFlipped( true );
    assert( dm.isFlipped() );
    assert( dm.getDriver().commandsSent.size() >= 2 );
    assert( dm.getDriver().commandsSent[ dm.getDriver().commandsSent.size() - 2 ] == 0xA0 );
    assert( dm.getDriver().commandsSent[ dm.getDriver().commandsSent.size() - 1 ] == 0xC0 );

    dm.setFlipped( false );
    assert( !dm.isFlipped() );
    assert( dm.getDriver().commandsSent[ dm.getDriver().commandsSent.size() - 2 ] == 0xA1 );
    assert( dm.getDriver().commandsSent[ dm.getDriver().commandsSent.size() - 1 ] == 0xC8 );

    const size_t count = dm.getDriver().commandsSent.size();
    dm.setFlipped( false );
    assert( !dm.isFlipped() );
    assert( dm.getDriver().commandsSent.size() == count );

    std::cout << "PASS: testHardwareFlipping\n";
}

int main()
{
    testStaticTextNoMarquee();
    testMarqueeProgressionAndWrapping();
    testPowerAndSleepControls();
    testHeaderAndBody();
    testProgressBarAndRateLimiter();
    test_display_sentence_short_wraps();
    test_display_sentence_long_marquees();
    testSetupScreenRendering();
    testResetCountdownRendering();
    test_display_toast();
    test_display_toast_guards_set_header();
    testCrtShutterFrames();
    testDiagnosticsRendering();
    testHardwareFlipping();

    std::cout << "test_display_manager: All tests passed successfully!\n";
    return 0;
}
