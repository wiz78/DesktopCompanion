#include "controllers/timerPresets.h"

#include <cassert>
#include <iostream>

void test_preset_counts_and_boundaries()
{
    assert( TimerPresets::NUM_PRESETS == 26 );
    assert( TimerPresets::getDurationSeconds( 0 ) == 30 );
    assert( TimerPresets::getDurationSeconds( 9 ) == 300 );
    assert( TimerPresets::getDurationSeconds( 10 ) == 360 );
    assert( TimerPresets::getDurationSeconds( 19 ) == 900 );
    assert( TimerPresets::getDurationSeconds( 20 ) == 1050 );
    assert( TimerPresets::getDurationSeconds( 25 ) == 1800 );
    std::cout << "PASS: test_preset_counts_and_boundaries\n";
}

void test_preset_tier_intervals()
{
    // Tier 1: 0:30 to 5:00 in 30s increments
    for( uint8_t i = 1; i <= 9; ++i ) {
        assert( TimerPresets::getDurationSeconds( i ) - TimerPresets::getDurationSeconds( i - 1 ) == 30 );
    }

    // Tier 2: 6:00 to 15:00 in 60s increments
    for( uint8_t i = 11; i <= 19; ++i ) {
        assert( TimerPresets::getDurationSeconds( i ) - TimerPresets::getDurationSeconds( i - 1 ) == 60 );
    }

    // Tier 3: 17:30 to 30:00 in 150s (2.5m) increments
    for( uint8_t i = 21; i <= 25; ++i ) {
        assert( TimerPresets::getDurationSeconds( i ) - TimerPresets::getDurationSeconds( i - 1 ) == 150 );
    }
    std::cout << "PASS: test_preset_tier_intervals\n";
}

void test_normalized_mapping()
{
    assert( TimerPresets::getStepIndexForNormalized( 0.0f ) == 0 );
    assert( TimerPresets::getStepIndexForNormalized( 1.0f ) == 25 );
    assert( TimerPresets::getStepIndexForNormalized( -0.1f ) == 0 );
    assert( TimerPresets::getStepIndexForNormalized( 1.5f ) == 25 );
    std::cout << "PASS: test_normalized_mapping\n";
}

int main()
{
    std::cout << "Running TimerPresets Native Tests...\n";
    test_preset_counts_and_boundaries();
    test_preset_tier_intervals();
    test_normalized_mapping();
    std::cout << "✅ ALL TIMER PRESETS TESTS PASSED!\n";
    return 0;
}
