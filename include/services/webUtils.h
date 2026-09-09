//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_WEBUTILS_H
#define FRIENDLYBOT_SERVICES_WEBUTILS_H

#include <cstddef>
#include <cstdint>
#include <string>

struct PartialConfig
{
    bool hasSsid = false;
    std::string ssid;

    bool hasPass = false;
    std::string pass;

    bool hasHostname = false;
    std::string hostname;

    bool hasLat = false;
    double lat = 0.0;

    bool hasLon = false;
    double lon = 0.0;

    bool hasApiKey = false;
    std::string apiKey;

    bool hasTz = false;
    std::string tz;

    bool hasLang = false;
    std::string lang;

    bool hasTimerMode = false;
    uint8_t timerMode = 0;

    bool hasInvertPotL = false;
    bool invertPotL = false;

    bool hasInvertPotR = false;
    bool invertPotR = false;

    bool hasInvertPowerSense = false;
    bool invertPowerSense = false;

    bool hasTempOffset = false;
    float tempOffset = 0.0f;

    bool hasNightThreshold = false;
    uint16_t nightThreshold = 300;

    bool hasSlapThreshold = false;
    float slapThreshold = 2.0f;
};

class WebUtils
{
public:
    [[nodiscard]] static std::string sanitizeHostname( const std::string& input );

    [[nodiscard]] static bool plainTextToJsonSentences(
        const std::string& text,
        std::string& outJson,
        size_t maxBytes,
        size_t& outCount
    );

    [[nodiscard]] static bool parsePartialConfig(
        const std::string& json,
        PartialConfig& out
    );

    [[nodiscard]] static std::string formatMarqueeField(
        const std::string& label,
        const std::string& value,
        size_t maxChars,
        size_t step
    );
};

#endif // FRIENDLYBOT_SERVICES_WEBUTILS_H
