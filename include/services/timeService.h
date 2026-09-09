//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_TIMESERVICE_H
#define FRIENDLYBOT_SERVICES_TIMESERVICE_H

#include "i18n.h"

#include <atomic>
#include <ctime>
#include <string>

class TimeService
{
public:
    static constexpr const char *DEFAULT_TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

    TimeService();

    void init( const std::string& posixTz = DEFAULT_TIMEZONE );
    void setTimezone( const std::string& posixTz );
    [[nodiscard]] const std::string& getTimezone() const
    {
        return currentTz;
    }

    [[nodiscard]] bool isTimeSynced() const
    {
        return timeSynced;
    }
    void markSynced( bool synced = true )
    {
        timeSynced = synced;
    }

    [[nodiscard]] bool getLocalTimeInfo( tm& outTime ) const;

    [[nodiscard]] static std::string formatTime( const tm& timeInfo );
    [[nodiscard]] static std::string formatDate( const tm& timeInfo, Language lang );

private:
    std::string currentTz;
    std::atomic<bool> timeSynced{ false };
};

#endif // FRIENDLYBOT_SERVICES_TIMESERVICE_H
