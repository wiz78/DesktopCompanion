#include "pins.h"

#include <cassert>
#include <iostream>

int main()
{
    // Verify pin mappings match spec
    static_assert( PIN_I2C_SDA == 8 );
    static_assert( PIN_I2C_SCL == 9 );
    static_assert( PIN_MPU_INT == 6 );
    static_assert( PIN_POT_LEFT == 1 );
    static_assert( PIN_POT_RIGHT == 2 );
    static_assert( PIN_LDR == 3 );
    static_assert( PIN_USB_DETECT == 4 );
    static_assert( PIN_BUTTON_ACTION == 10 );
    static_assert( PIN_BUTTON_RESET == 12 );
    static_assert( PIN_BUZZER == 5 );

    std::cout << "test_pins: All pin assertions passed!\n";
    return 0;
}
