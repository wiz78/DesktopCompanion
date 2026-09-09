#include "drivers/potentiometer.h"

#include <cassert>
#include <cmath>
#include <iostream>

void testBasicRawAndNormalized()
{
    Potentiometer pot( 1, 0.2f, 15 ); // Pin 1, alpha=0.2, deadband=15

    assert( pot.getPin() == 1 );
    assert( pot.getRawValue() == 0 );

    // Initial low feed
    for( int i = 0; i < 20; ++i ) {
        pot.updateRaw( 0 );
    }
    assert( pot.getRawValue() == 0 );
    assert( std::abs( pot.getNormalized() - 0.0f ) < 0.001f );
    assert( pot.getStep( 4 ) == 0 );

    // Non-linear curve check: 0.0 normalized should return minVal
    assert( std::abs( pot.getMappedNonLinear( 0.5f, 30.0f, 2.0f ) - 0.5f ) < 0.01f );

    // Max feed: 4095
    for( int i = 0; i < 40; ++i ) {
        pot.updateRaw( 4095 );
    }
    assert( pot.getRawValue() == 4095 );
    assert( std::abs( pot.getNormalized() - 1.0f ) < 0.01f );
    assert( pot.getStep( 4 ) == 3 );
    assert( std::abs( pot.getMappedNonLinear( 0.5f, 30.0f, 2.0f ) - 30.0f ) < 0.05f );
}

void testHysteresisAndSteps()
{
    Potentiometer pot( 2, 0.2f, 15 );

    // Initialize at 0 -> Step 0 (4 steps: [0.0..0.25), [0.25..0.5), [0.5..0.75), [0.75..1.0])
    pot.updateRaw( 0 );
    assert( pot.getStep( 4, 0.03f ) == 0 );

    // Single step edge case
    assert( pot.getStep( 1 ) == 0 );
    assert( pot.getStep( 0 ) == 0 );

    // Transition towards step 1 (normalized > 0.25 + 0.03 = 0.28)
    // 0.28 * 4095 = 1146.6
    for( int i = 0; i < 50; ++i ) {
        pot.updateRaw( 1200 );
    }
    assert( pot.getStep( 4, 0.03f ) == 1 );

    // Test hysteresis at boundary between category 1 and 2
    // Middle value: ~2048 -> ~0.50 normalized
    for( int i = 0; i < 50; ++i ) {
        pot.updateRaw( 2048 );
    }
    const uint8_t step = pot.getStep( 4, 0.04f );
    assert( step == 2 || step == 1 );
}

void testDeadbandAndHasChanged()
{
    Potentiometer pot( 3, 0.5f, 20 );

    // Initial state
    pot.updateRaw( 1000 );
    assert( !pot.hasChanged( 0.05f ) ); // Initial state should not report change unless threshold exceeded

    // Small jitter below deadband (20)
    pot.updateRaw( 1010 );
    assert( pot.getRawValue() == 1010 );
    // Filtered value shouldn't have changed due to deadband
    assert( std::abs( pot.getNormalized() - ( 1000.0f / 4095.0f ) ) < 0.0001f );
    assert( !pot.hasChanged( 0.05f ) );

    // Significant jump
    for( int i = 0; i < 30; ++i ) {
        pot.updateRaw( 3000 );
    }
    assert( pot.hasChanged( 0.05f ) );
    assert( !pot.hasChanged( 0.05f ) ); // Single-shot consumption
}

void testNonLinearMapping()
{
    Potentiometer pot( 4, 1.0f, 0 );

    pot.updateRaw( 2048 ); // ~0.5 normalized
    const float norm = pot.getNormalized();
    assert( std::abs( norm - ( 2048.0f / 4095.0f ) ) < 0.001f );

    // Linear mapping with curvePower = 1.0
    const float linear = pot.getMappedNonLinear( 10.0f, 20.0f, 1.0f );
    assert( std::abs( linear - ( 10.0f + ( norm * 10.0f ) ) ) < 0.01f );

    // Quadratic mapping with curvePower = 2.0
    const float quadratic = pot.getMappedNonLinear( 0.0f, 100.0f, 2.0f );
    assert( std::abs( quadratic - ( std::pow( norm, 2.0f ) * 100.0f ) ) < 0.01f );
}

void testMultiStepRotation()
{
    Potentiometer pot( 5, 1.0f, 0 ); // Instant filtering (alpha = 1.0)

    // 1. Start at 0 -> Step 0 (4 steps: [0..0.25), [0.25..0.5), [0.5..0.75), [0.75..1.0])
    pot.updateRaw( 0 );
    assert( pot.getStep( 4, 0.03f ) == 0 );

    // 2. Fast turn jumping from Step 0 straight to Step 3 (raw 4000 -> norm ~0.976)
    pot.updateRaw( 4000 );
    assert( pot.getStep( 4, 0.03f ) == 3 );

    // 3. Fast turn jumping from Step 3 straight to Step 0 (raw 10 -> norm ~0.002)
    pot.updateRaw( 10 );
    assert( pot.getStep( 4, 0.03f ) == 0 );

    // 4. Jump from Step 0 to near-boundary of Step 3 (norm = 0.76)
    // Candidate is 3 (>= 0.75), but threshold + hysteresis = 0.75 + 0.03 = 0.78.
    // Since 0.76 < 0.78, it advances to candidate - 1 = 2 (not stuck at 0!)
    const auto raw76 = static_cast<uint16_t>(0.76f * 4095.0f);
    pot.updateRaw( raw76 );
    assert( pot.getStep( 4, 0.03f ) == 2 );

    // Moving a little further past hysteresis threshold (norm = 0.82)
    const auto raw82 = static_cast<uint16_t>(0.82f * 4095.0f);
    pot.updateRaw( raw82 );
    assert( pot.getStep( 4, 0.03f ) == 3 );

    // 5. Jump downwards from Step 3 to near-boundary of Step 0 (norm = 0.24)
    // Candidate is 0 (< 0.25), but threshold - hysteresis = 0.25 - 0.03 = 0.22.
    // Since 0.24 > 0.22, it drops to candidate + 1 = 1 (not stuck at 3!)
    const auto raw24 = static_cast<uint16_t>(0.24f * 4095.0f);
    pot.updateRaw( raw24 );
    assert( pot.getStep( 4, 0.03f ) == 1 );

    // Moving below hysteresis threshold (norm = 0.15)
    const auto raw15 = static_cast<uint16_t>(0.15f * 4095.0f);
    pot.updateRaw( raw15 );
    assert( pot.getStep( 4, 0.03f ) == 0 );
}

void testInversion()
{
    // Test with alpha = 1.0 for instant response
    Potentiometer invPot( 1, 1.0f, 0 );
    assert( !invPot.isInverted() );
    invPot.setInverted( true );
    assert( invPot.isInverted() );
    invPot.updateRaw( 0 );
    assert( std::abs( invPot.getNormalized() - 1.0f ) < 0.001f );
    assert( invPot.getStep( 4 ) == 3 );
    invPot.updateRaw( 4095 );
    assert( std::abs( invPot.getNormalized() - 0.0f ) < 0.001f );
    assert( invPot.getStep( 4 ) == 0 );

    // Test with default filtered potentiometer over multiple updates
    Potentiometer filteredPot( 1 );
    filteredPot.setInverted( true );
    assert( filteredPot.isInverted() );
    filteredPot.updateRaw( 0 );
    assert( std::abs( filteredPot.getNormalized() - 1.0f ) < 0.001f );
    assert( filteredPot.getStep( 4 ) == 3 );
    for( int i = 0; i < 40; ++i ) {
        filteredPot.updateRaw( 4095 );
    }
    assert( std::abs( filteredPot.getNormalized() - 0.0f ) < 0.001f );
    assert( filteredPot.getStep( 4 ) == 0 );
}

int main()
{
    testBasicRawAndNormalized();
    testHysteresisAndSteps();
    testMultiStepRotation();
    testDeadbandAndHasChanged();
    testNonLinearMapping();
    testInversion();

    std::cout << "test_potentiometer: All tests passed successfully!\n";
    return 0;
}
