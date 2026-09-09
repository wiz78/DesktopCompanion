//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/timeService.h"

#include <array>
#include <cstdio>
#include <cstdlib>

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_sntp.h>
#endif

namespace
{
    constexpr std::array<const char *, 7> DAYS_EN = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    constexpr std::array<const char *, 7> DAYS_IT = { "DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB" };

    constexpr std::array<const char *, 12> MONTHS_EN = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    constexpr std::array<const char *, 12> MONTHS_IT = {
        "GEN", "FEB", "MAR", "APR", "MAG", "GIU", "LUG", "AGO", "SET", "OTT", "NOV", "DIC"
    };
}

TimeService::TimeService() : currentTz( DEFAULT_TIMEZONE )
{
}

void TimeService::init( const std::string& posixTz )
{
    setTimezone( posixTz );
#ifdef ARDUINO
    configTzTime( currentTz.c_str(), "pool.ntp.org", "time.nist.gov" );
#endif
}

void TimeService::setTimezone( const std::string& posixTz )
{
    currentTz = posixTz.empty() ? DEFAULT_TIMEZONE : posixTz;
#ifdef ARDUINO
    setenv( "TZ", currentTz.c_str(), 1 ); tzset();
#endif
}

bool TimeService::getLocalTimeInfo( tm& outTime ) const
{
#ifdef ARDUINO
    return getLocalTime( &outTime, 50 );
#else
    const time_t now = time( nullptr );
    const tm *loc = localtime( &now );
    if( loc != nullptr ) {
        outTime = *loc;
        return true;
    }
    return false;
#endif
}

std::string TimeService::formatTime( const tm& timeInfo )
{
    char buf[ 16 ];
    std::snprintf( buf, sizeof( buf ), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min );
    return { buf };
}

std::string TimeService::formatDate( const tm& timeInfo, const Language lang )
{
    const int wday = ( timeInfo.tm_wday >= 0 && timeInfo.tm_wday < 7 ) ? timeInfo.tm_wday : 0;
    const int mon = ( timeInfo.tm_mon >= 0 && timeInfo.tm_mon < 12 ) ? timeInfo.tm_mon : 0;

    const char *dayStr = ( lang == Language::Italian ) ? DAYS_IT[ wday ] : DAYS_EN[ wday ];
    const char *monStr = ( lang == Language::Italian ) ? MONTHS_IT[ mon ] : MONTHS_EN[ mon ];

    char buf[ 32 ];
    std::snprintf( buf, sizeof( buf ), "%s %02d %s", dayStr, timeInfo.tm_mday, monStr );
    return { buf };
}
