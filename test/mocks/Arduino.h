#ifndef TEST_MOCKS_ARDUINO_H
#define TEST_MOCKS_ARDUINO_H

#include <cstdint>

static constexpr uint8_t INPUT = 0;
static constexpr uint8_t OUTPUT = 1;
static constexpr uint8_t INPUT_PULLUP = 2;

static constexpr uint8_t LOW = 0;
static constexpr uint8_t HIGH = 1;

inline void pinMode( uint8_t pin, uint8_t mode )
{
    (void)pin;
    (void)mode;
}

inline int digitalRead( uint8_t pin )
{
    (void)pin;
    return LOW;
}

inline void digitalWrite( uint8_t pin, uint8_t val )
{
    (void)pin;
    (void)val;
}

inline uint16_t analogRead( uint8_t pin )
{
    (void)pin;
    return 0;
}

inline long map( long x, long in_min, long in_max, long out_min, long out_max )
{
    return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}

inline uint32_t g_mockMillis = 0;

inline void setMockMillis( const uint32_t ms )
{
    g_mockMillis = ms;
}

inline void advanceMockMillis( const uint32_t ms )
{
    g_mockMillis += ms;
}

inline uint32_t millis()
{
    return g_mockMillis;
}

inline void delay( const unsigned long ms )
{
    (void)ms;
}

#include <cstdarg>
#include <string>

#ifndef ARDUINO
class String
{
public:
    String() = default;

    String( const char *cstr ) :
        str( cstr ? cstr : "" )
    {
    }

    String( const std::string &s ) :
        str( s )
    {
    }

    String( int val ) :
        str( std::to_string( val ) )
    {
    }

    String( unsigned int val ) :
        str( std::to_string( val ) )
    {
    }

    String( float val ) :
        str( std::to_string( val ) )
    {
    }

    [[nodiscard]] unsigned int length() const
    {
        return static_cast<unsigned int>(str.length());
    }

    [[nodiscard]] const char *c_str() const
    {
        return str.c_str();
    }

    [[nodiscard]] bool startsWith( const char *prefix ) const
    {
        if( !prefix ) {
            return false;
        }
        return str.rfind( prefix, 0 ) == 0;
    }

    [[nodiscard]] bool startsWith( const String &prefix ) const
    {
        return startsWith( prefix.c_str() );
    }

    [[nodiscard]] String substring( unsigned int from, unsigned int to ) const
    {
        if( from >= str.length() ) {
            return String( "" );
        }
        if( to > str.length() ) {
            to = static_cast<unsigned int>(str.length());
        }
        if( to < from ) {
            return String( "" );
        }
        return String( str.substr( from, to - from ) );
    }

    [[nodiscard]] String substring( unsigned int from ) const
    {
        if( from >= str.length() ) {
            return String( "" );
        }
        return String( str.substr( from ) );
    }

    [[nodiscard]] int indexOf( char ch, unsigned int from = 0 ) const
    {
        const auto pos = str.find( ch, from );
        return ( pos == std::string::npos ) ? -1 : static_cast<int>(pos);
    }

    [[nodiscard]] int toInt() const
    {
        try {
            return std::stoi( str );
        }
        catch( ... ) {
            return 0;
        }
    }

    void trim()
    {
        const auto start = str.find_first_not_of( " \t\r\n" );
        if( start == std::string::npos ) {
            str.clear();
            return;
        }
        const auto end = str.find_last_not_of( " \t\r\n" );
        str = str.substr( start, end - start + 1 );
    }

    bool operator==( const String &other ) const
    {
        return str == other.str;
    }

    bool operator==( const char *other ) const
    {
        return str == ( other ? other : "" );
    }

    bool operator!=( const String &other ) const
    {
        return str != other.str;
    }

    String operator+( const String &other ) const
    {
        return String( str + other.str );
    }

    String operator+( const char *other ) const
    {
        return String( str + ( other ? other : "" ) );
    }

    String &operator+=( const String &other )
    {
        str += other.str;
        return *this;
    }

    String &operator+=( const char *other )
    {
        if( other ) {
            str += other;
        }
        return *this;
    }

    char operator[]( unsigned int idx ) const
    {
        return str[ idx ];
    }

private:
    std::string str;
};
#endif

class HardwareSerial
{
public:
    void print( const char *msg = "" )
    {
        if( msg ) {
            captured += msg;
        }
    }

    void print( const String &s )
    {
        captured += s.c_str();
    }

    void begin( const unsigned long baud )
    {
        (void)baud;
    }

    void println( const char *msg = "" )
    {
        if( msg ) {
            captured += msg;
        }
        captured += "\n";
    }

    void println( const String &s )
    {
        captured += s.c_str();
        captured += "\n";
    }

    void printf( const char *fmt, ... )
    {
        if( !fmt ) {
            return;
        }
        char buf[ 512 ];
        va_list args;
        va_start( args, fmt );
        vsnprintf( buf, sizeof( buf ), fmt, args );
        va_end( args );
        captured += buf;
    }

    void queueInput( const String &input )
    {
        inputBuffer += input.c_str();
    }

    int available()
    {
        return static_cast<int>(inputBuffer.length());
    }

    String readStringUntil( char terminator )
    {
        const auto pos = inputBuffer.find( terminator );
        if( pos == std::string::npos ) {
            std::string res = inputBuffer;
            inputBuffer.clear();
            return String( res );
        }
        std::string res = inputBuffer.substr( 0, pos );
        inputBuffer.erase( 0, pos + 1 );
        return String( res );
    }

    [[nodiscard]] std::string getCapturedOutput() const
    {
        return captured;
    }

    void clearCapturedOutput()
    {
        captured.clear();
    }

    void clearInputBuffer()
    {
        inputBuffer.clear();
    }

private:
    std::string captured;
    std::string inputBuffer;
};

inline HardwareSerial Serial;

class MockEspClass
{
public:
    bool restarted = false;

    void restart()
    {
        restarted = true;
    }
};

inline MockEspClass ESP;

#endif // TEST_MOCKS_ARDUINO_H
