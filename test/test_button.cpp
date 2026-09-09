#include "drivers/button.h"

#include <cassert>
#include <iostream>

void testActiveLowDebounceAndClick()
{
    Button btn( 10, true, 30 ); // Pin 10, activeLow=true, debounceMs=30

    // Initial state: not pressed (rawState = true because active-low unpressed)
    btn.update( true, 0 );
    assert( !btn.isPressed() );
    assert( !btn.wasClicked() );
    assert( btn.getHoldDurationMs( 0 ) == 0 );

    // Glitch / bounce: pressed for only 10ms (rawState = false)
    btn.update( false, 5 );
    btn.update( true, 15 );
    assert( !btn.isPressed() );
    assert( !btn.wasClicked() );

    // Solid press: pressed for 50ms then released
    btn.update( false, 20 );
    btn.update( false, 60 ); // Stable after debounce
    assert( btn.isPressed() );
    assert( !btn.wasClicked() ); // Click only fires on release!
    assert( btn.getHoldDurationMs( 70 ) == 50 );

    // Release button
    btn.update( true, 100 );
    assert( !btn.isPressed() );
    assert( btn.wasClicked() );
    assert( !btn.wasClicked() ); // Single-shot consumption
    assert( btn.getHoldDurationMs( 100 ) == 0 );

    // Long press test: hold for 3000ms
    btn.update( false, 200 );
    btn.update( false, 240 ); // Debounced
    btn.update( false, 3210 ); // 3010ms hold
    assert( btn.wasLongPressed( 3000, 3210 ) );
    assert( !btn.wasLongPressed( 3000, 3220 ) ); // Single-shot consumption
    assert( !btn.wasClicked() ); // Releasing after long-press should not fire standard short click
    btn.update( true, 3300 );
    assert( !btn.wasClicked() );
}

void testActiveHighButton()
{
    Button btn( 7, false, 25 ); // Pin 7, activeLow=false, debounceMs=25

    // Initial state: low (not pressed)
    btn.update( false, 0 );
    assert( !btn.isPressed() );
    assert( !btn.wasClicked() );

    // Noise high for 10ms
    btn.update( true, 10 );
    btn.update( false, 20 );
    assert( !btn.isPressed() );
    assert( !btn.wasClicked() );

    // Valid high press for 30ms
    btn.update( true, 30 );
    btn.update( true, 60 );
    assert( btn.isPressed() );
    assert( !btn.wasClicked() );

    // Release (low)
    btn.update( false, 70 );
    assert( !btn.isPressed() );
    assert( btn.wasClicked() );
    assert( !btn.wasClicked() );
}

void testMultipleConsecutiveClicks()
{
    Button btn( 10, true, 20 );

    for( int i = 0; i < 3; ++i ) {
        uint32_t base = static_cast<uint32_t>(i * 100);
        btn.update( false, base );
        btn.update( false, base + 25 );
        assert( btn.isPressed() );
        assert( !btn.wasClicked() );

        btn.update( true, base + 50 );
        assert( !btn.isPressed() );
        assert( btn.wasClicked() );
        assert( !btn.wasClicked() );
    }
}

int main()
{
    testActiveLowDebounceAndClick();
    testActiveHighButton();
    testMultipleConsecutiveClicks();

    std::cout << "test_button: All tests passed successfully!\n";
    return 0;
}
