#include "drivers/buzzer.h"

#include <cassert>
#include <iostream>

int main()
{
    Buzzer buzzer( 5 );
    assert( buzzer.getPin() == 5 );
    assert( !buzzer.isSounding() );

    // Single beep 50ms
    buzzer.beep( 50, 0 );
    buzzer.update( 0 );
    assert( buzzer.isSounding() );

    buzzer.update( 40 );
    assert( buzzer.isSounding() );

    buzzer.update( 55 );
    assert( !buzzer.isSounding() );

    // Double beep: 30ms on, 20ms off, 30ms on starting at 100ms
    buzzer.doubleBeep( 30, 20, 100 );
    buzzer.update( 100 );
    assert( buzzer.isSounding() );

    buzzer.update( 125 ); // Still first beep
    assert( buzzer.isSounding() );

    buzzer.update( 135 ); // Gap
    assert( !buzzer.isSounding() );

    buzzer.update( 155 ); // Second beep
    assert( buzzer.isSounding() );

    buzzer.update( 185 ); // Finished
    assert( !buzzer.isSounding() );

    // Alarm pattern (repeating: 150ms on, 100ms off)
    buzzer.playAlarmPattern( 200 );
    buzzer.update( 200 );
    assert( buzzer.isSounding() );

    buzzer.update( 300 ); // Still first beep (100ms into 150ms)
    assert( buzzer.isSounding() );

    buzzer.update( 360 ); // Gap (160ms since 200ms -> in 100ms off period)
    assert( !buzzer.isSounding() );

    buzzer.update( 460 ); // Repeats -> second cycle beep on
    assert( buzzer.isSounding() );

    buzzer.stop();
    assert( !buzzer.isSounding() );
    buzzer.update( 500 );
    assert( !buzzer.isSounding() );

    std::cout << "test_buzzer: All tests passed successfully!\n";
    return 0;
}
