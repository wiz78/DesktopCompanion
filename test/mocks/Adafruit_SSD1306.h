#ifndef TEST_MOCKS_ADAFRUIT_SSD1306_H
#define TEST_MOCKS_ADAFRUIT_SSD1306_H

#include "Adafruit_GFX.h"
#include "Wire.h"

#include <cstdint>
#include <cstring>
#include <vector>

static constexpr uint8_t SSD1306_SWITCHCAPVCC = 0x02;
static constexpr uint16_t SSD1306_WHITE = 1;
static constexpr uint16_t SSD1306_BLACK = 0;
static constexpr uint8_t SSD1306_DISPLAYON = 0xAF;
static constexpr uint8_t SSD1306_DISPLAYOFF = 0xAE;
static constexpr uint8_t SSD1306_SETCONTRAST = 0x81;
static constexpr uint8_t SSD1306_SETPRECHARGE = 0xD9;
static constexpr uint8_t SSD1306_SETVCOMDETECT = 0xDB;

class Adafruit_SSD1306
{
public:
    Adafruit_SSD1306( int w, int h, TwoWire *wire = &Wire, int rstPin = -1 )
    {
        (void)w;
        (void)h;
        (void)wire;
        (void)rstPin;
    }

    bool begin( uint8_t switchVcc, uint8_t i2cAddr, bool reset = true, bool periphBegin = true )
    {
        (void)switchVcc;
        (void)i2cAddr;
        (void)reset;
        (void)periphBegin;
        return true;
    }

    void setTextColor( uint16_t color )
    {
        (void)color;
    }

    void clearDisplay()
    {
    }

    void setTextWrap( bool w )
    {
        (void)w;
    }

    void display()
    {
    }

    void fillRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color )
    {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)color;
    }

    void drawRect( int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color )
    {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)color;
    }

    void setTextSize( uint8_t size )
    {
        (void)size;
    }

    void setCursor( int16_t x, int16_t y )
    {
        (void)x;
        (void)y;
    }

    void print( const char *text )
    {
        (void)text;
    }

    void getTextBounds( const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h )
    {
        (void)x;
        (void)y;
        *x1 = 0;
        *y1 = 0;
        // Estimate 6px width per character
        size_t len = ( str != nullptr ) ? std::strlen( str ) : 0;
        *w = static_cast<uint16_t>(len * 6);
        *h = 8;
    }

    void drawFastHLine( int16_t x, int16_t y, int16_t w, uint16_t color )
    {
        (void)x;
        (void)y;
        (void)w;
        (void)color;
    }

    void drawLine( int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color )
    {
        (void)x0;
        (void)y0;
        (void)x1;
        (void)y1;
        (void)color;
    }

    void drawPixel( int16_t x, int16_t y, uint16_t color )
    {
        (void)x;
        (void)y;
        (void)color;
    }

    std::vector<uint8_t> commandsSent;

    void ssd1306_command( uint8_t cmd )
    {
        commandsSent.push_back( cmd );
    }

    void dim( bool dim )
    {
        (void)dim;
    }

    void invertDisplay( bool invert )
    {
        inverted = invert;
    }

    [[nodiscard]] bool isInverted() const
    {
        return inverted;
    }

    uint8_t *getBuffer()
    {
        return buffer;
    }

private:
    uint8_t buffer[ 1024 ] = { 0 };
    bool inverted = false;
};

#endif // TEST_MOCKS_ADAFRUIT_SSD1306_H
