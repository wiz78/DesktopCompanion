#include "drivers/proximitySensor.h"

#include <cassert>
#include <iostream>

void testDeskOccupantFilterBasicFlow()
{
    DeskOccupantFilter filter( 80, 100, 30000 ); // tolerance=80mm, deltaWake=100mm, lockTime=30s

    // 1. Initial far state (no target)
    filter.update( 2000, 0 );
    assert( !filter.shouldWakeUp() );
    assert( !filter.isBaselineLocked() );

    // 2. Arrival: person walks up to 700mm (debounced for 1000ms)
    filter.update( 700, 1000 );
    assert( !filter.shouldWakeUp() );
    filter.update( 700, 2000 );
    assert( filter.shouldWakeUp() ); // Wakes up on sustained arrival!
    assert( !filter.shouldWakeUp() ); // Single-shot consumption

    // 3. User sits at desk for 35 seconds within 700mm +/- 50mm
    for( uint32_t t = 3000; t <= 38000; t += 1000 ) {
        filter.update( 710, t );
    }
    assert( filter.isBaselineLocked() );
    assert( filter.isSleepPermitted() ); // Screen is now allowed to sleep
    assert( filter.getBaselineMm() == 700 || filter.getBaselineMm() == 710 );

    // 4. User leans in closer (710mm -> 560mm: delta 150mm > 100mm)
    filter.update( 560, 39000 );
    assert( filter.shouldWakeUp() ); // Wakes up on intentional lean-in!
    assert( !filter.shouldWakeUp() ); // Single-shot consumption

    // 5. User leaves desk (distance jumps to 2000mm, debounced for >= 1500ms)
    filter.update( 2000, 40000 );
    assert( filter.isOccupied() ); // Debouncing departure (< 1500ms)
    filter.update( 2000, 41500 );
    assert( !filter.isOccupied() );
    assert( !filter.isBaselineLocked() );
    assert( !filter.isSleepPermitted() );
}

void testProximitySensorDriver()
{
    ProximitySensor sensor;

    assert( sensor.getDistanceMm() == 2000 );
    assert( !sensor.isTargetPresent( 1000 ) );

    // Feed distance (sustained arrival)
    sensor.feedRawDistance( 600, 100 );
    assert( sensor.getDistanceMm() == 600 );
    assert( sensor.isTargetPresent( 1000 ) );
    assert( !sensor.shouldWakeUp() ); // Debouncing arrival

    sensor.feedRawDistance( 600, 1100 );
    assert( sensor.shouldWakeUp() );
    assert( !sensor.shouldWakeUp() );

    // Direct access to filter
    DeskOccupantFilter &filter = sensor.getFilter();
    assert( !filter.isBaselineLocked() );
}

void testGradualArrivalAndHysteresis()
{
    DeskOccupantFilter filter( 80, 100, 30000 );

    // 1. Initial far state (out of range)
    filter.update( 2000, 0 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // 2. Gradual arrival: user approaches into 1100mm (hysteresis band > 1000mm and <= 1200mm)
    filter.update( 1100, 500 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // 3. User enters primary detection zone (<= 1000mm, e.g. 700mm, sustained >= 1000ms)
    filter.update( 700, 1000 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 700, 2000 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );
    assert( !filter.shouldWakeUp() ); // single-shot

    // 4. Slight movements at desk
    filter.update( 720, 2500 );
    assert( filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // 5. User moves back into hysteresis band (1100mm) while occupied
    filter.update( 1100, 3000 );
    assert( filter.isOccupied() );

    // 6. User departs (> 1200mm, debounced for >= 1500ms)
    filter.update( 1500, 4000 );
    assert( filter.isOccupied() ); // Debouncing departure
    filter.update( 1500, 5500 );
    assert( !filter.isOccupied() );
    assert( !filter.isBaselineLocked() );

    // 7. User arrives again gradually
    filter.update( 1150, 6000 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 800, 7000 );
    assert( !filter.isOccupied() );
    filter.update( 800, 8000 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );
}

void testTransientNoiseRejection()
{
    DeskOccupantFilter filter;

    // Initial state: out of range
    filter.update( 2000, 0 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // Single 60ms pulse at 700mm (< 1000ms debounce)
    filter.update( 700, 100 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 2000, 160 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // Pulse persisting for 500ms (< 1000ms debounce)
    filter.update( 700, 1000 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 700, 1500 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // Returns to > 1000mm at 1600ms (elapsed 600ms < 1000ms)
    filter.update( 2000, 1600 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // Single-sample close-range noise (<= 200mm, e.g. 150mm for 1 sample)
    filter.update( 150, 2000 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 2000, 2060 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );
}

void testSustainedArrival()
{
    DeskOccupantFilter filter;

    // Reading <= 1000mm (700mm) persisting for >= 1000ms at t=0, t=500, t=1000ms
    filter.update( 700, 0 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 700, 500 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    filter.update( 700, 1000 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );
    assert( !filter.shouldWakeUp() ); // single-shot consumption
}

void testFastCloseRangeWave()
{
    DeskOccupantFilter filter;

    // 1st close sample <= 200mm (150mm) at t=0
    filter.update( 150, 0 );
    assert( !filter.isOccupied() );
    assert( !filter.shouldWakeUp() );

    // 2nd consecutive close sample at t=60ms (well under 1000ms arrival debounce)
    filter.update( 150, 60 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );
    assert( !filter.shouldWakeUp() ); // single-shot consumption
}

void testDebouncedDeparture()
{
    DeskOccupantFilter filter;

    // Establish occupancy
    filter.update( 700, 0 );
    filter.update( 700, 1000 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );

    // Reading > 1200mm starts departure candidate at t=2000
    filter.update( 1500, 2000 );
    assert( filter.isOccupied() ); // Debounce in progress

    // Reading > 1200mm at 500ms elapsed (< 1500ms debounce)
    filter.update( 1500, 2500 );
    assert( filter.isOccupied() );

    // Reading > 1200mm at 1000ms elapsed (< 1500ms debounce)
    filter.update( 1500, 3000 );
    assert( filter.isOccupied() );

    // Temporary dropout returns to desk within debounce (< 1500ms)
    filter.update( 700, 3200 );
    assert( filter.isOccupied() );

    // Departure starts again at t=4000
    filter.update( 1500, 4000 );
    assert( filter.isOccupied() );

    filter.update( 1500, 4500 ); // 500ms
    assert( filter.isOccupied() );

    filter.update( 1500, 5000 ); // 1000ms
    assert( filter.isOccupied() );

    // At t=5500 (1500ms elapsed >= 1500ms debounce): transitions to unoccupied
    filter.update( 1500, 5500 );
    assert( !filter.isOccupied() );
}

void testLeanInAfterBaselineLock()
{
    DeskOccupantFilter filter( 80, 100, 30000 ); // tolerance=80mm, deltaWake=100mm, lockTime=30s

    // Establish occupancy
    filter.update( 700, 0 );
    filter.update( 700, 1000 );
    assert( filter.isOccupied() );
    assert( filter.shouldWakeUp() );

    // Sits at desk for 30 seconds
    for( uint32_t t = 2000; t <= 32000; t += 1000 ) {
        filter.update( 700, t );
    }
    assert( filter.isBaselineLocked() );
    assert( !filter.shouldWakeUp() );

    // Lean in closer: baseline is 700mm, deltaWake is 100mm, distance drops to 550mm (delta 150mm >= 100mm)
    filter.update( 550, 33000 );
    assert( filter.shouldWakeUp() );
    assert( !filter.shouldWakeUp() ); // single-shot consumption
    assert( filter.isOccupied() );
    assert( !filter.isBaselineLocked() );
}

int main()
{
    testDeskOccupantFilterBasicFlow();
    testGradualArrivalAndHysteresis();
    testProximitySensorDriver();
    testTransientNoiseRejection();
    testSustainedArrival();
    testFastCloseRangeWave();
    testDebouncedDeparture();
    testLeanInAfterBaselineLock();

    std::cout << "test_desk_occupant: All tests passed successfully!\n";
    return 0;
}
