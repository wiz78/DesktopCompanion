//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/webUtils.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <ArduinoJson.h>

std::string WebUtils::sanitizeHostname( const std::string& input )
{
    if( input.empty() ) {
        return "bot50";
    }

    std::string sanitized;
    sanitized.reserve( input.size() );

    bool lastWasHyphen = false;

    for( const char ch : input ) {
        const auto uch = static_cast<unsigned char>(ch);
        if( std::isalnum( uch ) ) {
            sanitized.push_back( static_cast<char>(std::tolower( uch )) );
            lastWasHyphen = false;
        } else if( uch == ' ' || uch == '_' || uch == '-' ) {
            if( !sanitized.empty() && !lastWasHyphen ) {
                sanitized.push_back( '-' );
                lastWasHyphen = true;
            }
        }
    }

    // Trim trailing hyphens
    while( !sanitized.empty() && sanitized.back() == '-' ) {
        sanitized.pop_back();
    }

    // If empty or if original input started/ended with hyphens, or if sanitized string is "invalid"
    if( sanitized.empty() || sanitized == "invalid" || input.front() == '-' || input.back() == '-' ) {
        return "bot50";
    }

    return sanitized;
}

bool WebUtils::plainTextToJsonSentences( const std::string& text, std::string& outJson, const size_t maxBytes,
                                         size_t& outCount )
{
    outCount = 0;
    outJson.clear();

    std::vector<std::string> sentences;
    size_t lineStart = 0;

    const auto processLine = [ &sentences ]( const std::string_view line ) {
        // Trim whitespace and carriage returns from both ends
        size_t start = 0;
        while( start < line.size() && ( line[ start ] == ' ' || line[ start ] == '\t' || line[ start ] == '\r' || line[
                                            start ] == '\n' ) ) {
            ++start;
        }
        size_t end = line.size();
        while( end > start && ( line[ end - 1 ] == ' ' || line[ end - 1 ] == '\t' || line[ end - 1 ] == '\r' || line[
                                    end - 1 ] == '\n' ) ) {
            --end;
        }

        if( start < end ) {
            sentences.emplace_back( line.substr( start, end - start ) );
        }
    };

    for( size_t i = 0; i < text.size(); ++i ) {
        if( text[ i ] == '\n' || text[ i ] == '\r' ) {
            processLine( std::string_view( text.data() + lineStart, i - lineStart ) );
            if( text[ i ] == '\r' && ( i + 1 ) < text.size() && text[ i + 1 ] == '\n' ) {
                ++i;
            }
            lineStart = i + 1;
        }
    }

    if( lineStart < text.size() ) {
        processLine( std::string_view( text.data() + lineStart, text.size() - lineStart ) );
    }

    if( sentences.empty() ) {
        outJson = "[]";
        if( outJson.size() > maxBytes ) {
            outJson.clear();
            return false;
        }
        outCount = 0;
        return true;
    }

    std::string json;
    json.reserve( text.size() + sentences.size() * 4 + 4 );
    json.push_back( '[' );

    for( size_t s = 0; s < sentences.size(); ++s ) {
        if( s > 0 ) {
            json.push_back( ',' );
        }
        json.push_back( '"' );
        const auto& sentence = sentences[ s ];
        for( const char ch : sentence ) {
            switch( ch ) {
                case '"':
                    json.append( "\\\"" );
                    break;
                case '\\':
                    json.append( "\\\\" );
                    break;
                case '\b':
                    json.append( "\\b" );
                    break;
                case '\f':
                    json.append( "\\f" );
                    break;
                case '\n':
                    json.append( "\\n" );
                    break;
                case '\r':
                    json.append( "\\r" );
                    break;
                case '\t':
                    json.append( "\\t" );
                    break;
                default:
                    json.push_back( ch );
                    break;
            }
        }
        json.push_back( '"' );
    }

    json.push_back( ']' );

    if( json.size() > maxBytes ) {
        return false;
    }

    outJson = std::move( json );
    outCount = sentences.size();
    return true;
}

bool WebUtils::parsePartialConfig( const std::string& json, PartialConfig& out )
{
    if( json.empty() ) {
        return false;
    }

    JsonDocument doc;
    const DeserializationError err = deserializeJson( doc, json );
    if( err != DeserializationError::Ok ) {
        return false;
    }

    if( !doc[ "ssid" ].isNull() ) {
        out.hasSsid = true;
        out.ssid = doc[ "ssid" ].as<std::string>();
    }

    if( !doc[ "pass" ].isNull() ) {
        out.hasPass = true;
        out.pass = doc[ "pass" ].as<std::string>();
    } else if( !doc[ "password" ].isNull() ) {
        out.hasPass = true;
        out.pass = doc[ "password" ].as<std::string>();
    }

    if( !doc[ "hostname" ].isNull() ) {
        out.hasHostname = true;
        out.hostname = sanitizeHostname( doc[ "hostname" ].as<std::string>() );
    }

    if( !doc[ "lat" ].isNull() ) {
        out.hasLat = true;
        out.lat = doc[ "lat" ].as<double>();
    } else if( !doc[ "latitude" ].isNull() ) {
        out.hasLat = true;
        out.lat = doc[ "latitude" ].as<double>();
    }

    if( !doc[ "lon" ].isNull() ) {
        out.hasLon = true;
        out.lon = doc[ "lon" ].as<double>();
    } else if( !doc[ "longitude" ].isNull() ) {
        out.hasLon = true;
        out.lon = doc[ "longitude" ].as<double>();
    }

    if( !doc[ "api_key" ].isNull() ) {
        out.hasApiKey = true;
        out.apiKey = doc[ "api_key" ].as<std::string>();
    } else if( !doc[ "apiKey" ].isNull() ) {
        out.hasApiKey = true;
        out.apiKey = doc[ "apiKey" ].as<std::string>();
    }

    if( !doc[ "tz" ].isNull() ) {
        out.hasTz = true;
        out.tz = doc[ "tz" ].as<std::string>();
    } else if( !doc[ "timezone" ].isNull() ) {
        out.hasTz = true;
        out.tz = doc[ "timezone" ].as<std::string>();
    }

    if( !doc[ "lang" ].isNull() ) {
        out.hasLang = true;
        out.lang = doc[ "lang" ].as<std::string>();
    } else if( !doc[ "language" ].isNull() ) {
        out.hasLang = true;
        out.lang = doc[ "language" ].as<std::string>();
    }

    if( !doc[ "timer_mode" ].isNull() ) {
        out.hasTimerMode = true;
        out.timerMode = static_cast<uint8_t>(doc[ "timer_mode" ].as<uint8_t>());
    } else if( !doc[ "timerMode" ].isNull() ) {
        out.hasTimerMode = true;
        out.timerMode = static_cast<uint8_t>(doc[ "timerMode" ].as<uint8_t>());
    }

    if( !doc[ "invert_pot_l" ].isNull() ) {
        out.hasInvertPotL = true;
        out.invertPotL = doc[ "invert_pot_l" ].as<bool>();
    }

    if( !doc[ "invert_pot_r" ].isNull() ) {
        out.hasInvertPotR = true;
        out.invertPotR = doc[ "invert_pot_r" ].as<bool>();
    }

    if( !doc[ "invert_pwr" ].isNull() ) {
        out.hasInvertPowerSense = true;
        out.invertPowerSense = doc[ "invert_pwr" ].as<bool>();
    }

    if( !doc[ "temp_offset" ].isNull() ) {
        out.hasTempOffset = true;
        out.tempOffset = doc[ "temp_offset" ].as<float>();
    }

    if( !doc[ "night_threshold" ].isNull() ) {
        out.hasNightThreshold = true;
        out.nightThreshold = std::clamp( static_cast<uint16_t>(doc[ "night_threshold" ].as<uint16_t>()),
                                         static_cast<uint16_t>(0), static_cast<uint16_t>(4095) );
    } else if( !doc[ "night_thresh" ].isNull() ) {
        out.hasNightThreshold = true;
        out.nightThreshold = std::clamp( static_cast<uint16_t>(doc[ "night_thresh" ].as<uint16_t>()),
                                         static_cast<uint16_t>(0), static_cast<uint16_t>(4095) );
    }

    if( !doc[ "slap_threshold" ].isNull() ) {
        out.hasSlapThreshold = true;
        out.slapThreshold = std::clamp( doc[ "slap_threshold" ].as<float>(), 0.5f, 5.0f );
    } else if( !doc[ "slap_thresh" ].isNull() ) {
        out.hasSlapThreshold = true;
        out.slapThreshold = std::clamp( doc[ "slap_thresh" ].as<float>(), 0.5f, 5.0f );
    }

    return true;
}

std::string WebUtils::formatMarqueeField( const std::string& label, const std::string& value, const size_t maxChars,
                                          const size_t step )
{
    if( maxChars == 0 ) {
        return "";
    }

    if( label.length() >= maxChars ) {
        return label.substr( 0, maxChars );
    }

    if( label.length() + value.length() <= maxChars ) {
        return label + value;
    }

    const size_t availChars = maxChars - label.length();
    const std::string padded = value + "   ";
    const size_t cycleLen = padded.length();
    const size_t offset = ( cycleLen > 0 ) ? ( step % cycleLen ) : 0;

    const std::string repeated = padded + padded;
    const std::string scrolled = repeated.substr( offset, availChars );

    return label + scrolled;
}
