#ifndef TEST_MOCKS_PREFERENCES_H
#define TEST_MOCKS_PREFERENCES_H

#include <cstdint>
#include <string>
#include <unordered_map>

class Preferences
{
public:
    static inline bool wasCleared = false;
    static inline std::unordered_map<std::string, std::string> stringStore;
    static inline std::unordered_map<std::string, uint8_t> ucharStore;
    static inline std::unordered_map<std::string, uint16_t> ushortStore;
    static inline std::unordered_map<std::string, float> floatStore;
    static inline std::unordered_map<std::string, double> doubleStore;

    bool begin( const char *name, bool readOnly = false )
    {
        (void)name;
        (void)readOnly;
        return true;
    }

    void end()
    {
    }

    bool clear()
    {
        wasCleared = true;
        stringStore.clear();
        ucharStore.clear();
        ushortStore.clear();
        floatStore.clear();
        doubleStore.clear();
        return true;
    }

    bool putString( const char *key, const char *value )
    {
        stringStore[ key ] = value;
        return true;
    }

    bool putUChar( const char *key, uint8_t value )
    {
        ucharStore[ key ] = value;
        return true;
    }

    bool putDouble( const char *key, double value )
    {
        doubleStore[ key ] = value;
        return true;
    }

    double getDouble( const char *key, double defaultValue = 0.0 )
    {
        const auto it = doubleStore.find( key );
        if( it != doubleStore.end() ) {
            return it->second;
        }
        return defaultValue;
    }

    bool isKey( const char *key ) const
    {
        return stringStore.count( key ) || ucharStore.count( key ) || ushortStore.count( key ) || floatStore.
               count( key ) || doubleStore.count( key );
    }

    bool remove( const char *key )
    {
        stringStore.erase( key );
        ucharStore.erase( key );
        ushortStore.erase( key );
        floatStore.erase( key );
        doubleStore.erase( key );
        return true;
    }

    std::string getString( const char *key, const char *defaultValue = "" )
    {
        const auto it = stringStore.find( key );
        if( it != stringStore.end() ) {
            return it->second;
        }
        return defaultValue;
    }

    bool putFloat( const char *key, float value )
    {
        floatStore[ key ] = value;
        return true;
    }

    float getFloat( const char *key, float defaultValue = 0.0f )
    {
        const auto it = floatStore.find( key );
        if( it != floatStore.end() ) {
            return it->second;
        }
        return defaultValue;
    }

    bool putUShort( const char *key, uint16_t value )
    {
        ushortStore[ key ] = value;
        return true;
    }

    uint16_t getUShort( const char *key, uint16_t defaultValue = 0 )
    {
        const auto it = ushortStore.find( key );
        if( it != ushortStore.end() ) {
            return it->second;
        }
        return defaultValue;
    }

    uint8_t getUChar( const char *key, uint8_t defaultValue = 0 )
    {
        const auto it = ucharStore.find( key );
        if( it != ucharStore.end() ) {
            return it->second;
        }
        return defaultValue;
    }
};

#endif // TEST_MOCKS_PREFERENCES_H
