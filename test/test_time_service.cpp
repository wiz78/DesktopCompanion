#include "i18n.h"
#include "services/timeService.h"

#include <cassert>
#include <ctime>
#include <iostream>

void testTimeFormatting()
{
    tm timeInfo{};
    timeInfo.tm_hour = 9;
    timeInfo.tm_min = 5;
    timeInfo.tm_sec = 0;

    const std::string formatted = TimeService::formatTime( timeInfo );
    assert( formatted == "09:05" );

    timeInfo.tm_hour = 23;
    timeInfo.tm_min = 59;
    assert( TimeService::formatTime( timeInfo ) == "23:59" );

    timeInfo.tm_hour = 0;
    timeInfo.tm_min = 0;
    assert( TimeService::formatTime( timeInfo ) == "00:00" );
    std::cout << "[PASS] testTimeFormatting\n";
}

void testDateFormattingItalian()
{
    tm timeInfo{};
    timeInfo.tm_wday = 2; // Tuesday
    timeInfo.tm_mday = 25;
    timeInfo.tm_mon = 9; // October (0-indexed)
    timeInfo.tm_year = 126; // 2026

    const std::string formatted = TimeService::formatDate( timeInfo, Language::Italian );
    assert( formatted == "MAR 25 OTT" );
    std::cout << "[PASS] testDateFormattingItalian\n";
}

void testDateFormattingEnglish()
{
    tm timeInfo{};
    timeInfo.tm_wday = 2; // Tuesday
    timeInfo.tm_mday = 25;
    timeInfo.tm_mon = 9; // October (0-indexed)
    timeInfo.tm_year = 126; // 2026

    const std::string formatted = TimeService::formatDate( timeInfo, Language::English );
    assert( formatted == "TUE 25 OCT" );
    std::cout << "[PASS] testDateFormattingEnglish\n";
}

void testTimeServiceState()
{
    TimeService ts;
    assert( !ts.isTimeSynced() );

    ts.markSynced( true );
    assert( ts.isTimeSynced() );

    ts.setTimezone( "UTC0" );
    assert( ts.getTimezone() == "UTC0" );
    std::cout << "[PASS] testTimeServiceState\n";
}

int main()
{
    std::cout << "Running TimeService unit tests...\n";
    testTimeFormatting();
    testDateFormattingItalian();
    testDateFormattingEnglish();
    testTimeServiceState();
    std::cout << "All TimeService tests passed!\n";
    return 0;
}
