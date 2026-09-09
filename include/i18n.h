//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_I18N_H
#define FRIENDLYBOT_I18N_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class Language : uint8_t
{
    English = 0, // Factory default
    Italian = 1
};

enum class StringId : uint16_t
{
    // System & Common
    AppTitle = 0,
    AppVersion,
    WifiSetupTitle,
    FactoryResetTitle,
    PowerUsb,
    PowerBattery,

    // Mode A: Clock, Weather, Environmental
    TempInternalPrefix,
    TempExternalPrefix,
    HumidityPrefix,
    WeatherSyncing,
    WeatherOffline,

    // Mode B: Percussive Maintenance (Slap Gag)
    SlapHeader,
    SlapPrompt,
    SlapSignalLost,
    SlapReject,
    SlapRepaired,
    SlapSubtext,

    // Mode C: Couple Arbitrator (Oracle)
    OracleHeader,
    OraclePromptSpin,
    OraclePromptRetry,
    OracleFood,
    OracleRight,
    OracleChores,
    OracleEntertainment,

    // Mode D: Kitchen Timer
    TimerTitle,
    TimerTimeUp,
    TimerMinutesShort,
    TimerSecondsShort,
    TimerPaused
};

class I18n
{
public:
    static I18n& instance();

    I18n( const I18n& ) = delete;
    I18n& operator=( const I18n& ) = delete;
    I18n( I18n&& ) = delete;
    I18n& operator=( I18n&& ) = delete;

    void init( Language defaultLang = Language::English );
    void loadFromNvs();
    void saveToNvs();

    void setLanguage( Language lang );
    bool setLanguageCode( const char *code );

    [[nodiscard]] Language getLanguage() const;
    [[nodiscard]] const char *getLanguageCode() const;
    [[nodiscard]] bool isItalian() const
    {
        return getLanguage() == Language::Italian;
    }

    [[nodiscard]] const char *get( StringId id ) const;

    [[nodiscard]] const char *getOracleCategory( uint8_t index ) const;
    [[nodiscard]] const std::vector<const char *>& getOracleVerdicts( uint8_t categoryIndex ) const;

    [[nodiscard]] size_t getBuiltinAphorismCount() const;
    [[nodiscard]] const char *getBuiltinAphorism( size_t index ) const;

private:
    I18n() = default;
    std::atomic<Language> currentLang{ Language::English };
};

#endif // FRIENDLYBOT_I18N_H
