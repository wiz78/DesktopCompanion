//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_DISPLAY_H
#define FRIENDLYBOT_DISPLAY_H

#include "textFormatter.h"

#include <cstdint>
#include <string>
#include <vector>

#define SSD1306_NO_SPLASH

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class DisplayManager
{
public:
    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 64;
    static constexpr int ZONE_HEADER_H = 16;
    static constexpr int ZONE_BODY_Y = 16;
    static constexpr int ZONE_BODY_H = 48;

    DisplayManager();
    explicit DisplayManager( Adafruit_SSD1306& rawDisplay );

    void setup();
    void init()
    {
        setup();
    }
    void begin()
    {
        setup();
    }

    [[nodiscard]] Adafruit_SSD1306& getDriver()
    {
        return display;
    }

    void update( uint32_t nowMs = 0 );
    void clear();
    void clearHeader();
    void clearBody();

    void setHeader( const std::string& leftText, const std::string& rightText = "", uint32_t nowMs = 0 );
    void setCenteredHeader( const std::string& centerText, uint32_t nowMs = 0 );
    void showToast( const std::string& msg, uint32_t durationMs = 2000, uint32_t nowMs = 0 );
    [[nodiscard]] bool isToastActive( uint32_t nowMs = 0 ) const;
    void render();
    void drawCenteredText( const char *text, int16_t y, uint8_t textSize = 1 );
    void setBodyText( const std::vector<std::string>& lines, int textSize = 1 );
    void setScrollingText( const std::string& text, int y = ZONE_BODY_Y + 8, int textSize = 2, uint16_t speedMs = 40, uint16_t pauseMs = 1200 );
    bool displaySentence( const std::string& sentence, size_t maxWrapChars = TextFormatter::MAX_WRAPPED_CHARS, uint32_t nowMs = 0,
                          uint16_t initialPause = 4000, uint16_t endPause = 5000 );
    void displaySetupScreen( const char *ssid, const char *ip, const char *url, uint32_t nowMs = 0 );
    void displayResetCountdown( uint8_t secondsRemaining );
    void displayCrtShutter( uint8_t frame );
    void displayDiagnostics(
        float leftPotNorm,
        float leftPotMinutes,
        uint8_t rightPotCategory,
        uint16_t ldrRaw,
        bool isNight,
        bool isUsb,
        uint16_t distanceMm,
        float tempC,
        bool shockDetected,
        bool shakeDetected
    );
    void drawProgressBar( int16_t x, int16_t y, int16_t w, int16_t h, float percent );

    void setPower( bool on );
    void setBrightnessLevel( int brightnessLevel );
    void invertDisplay( bool invert );
    void setFlipped( bool flipped );

    [[nodiscard]] bool isFlipped() const
    {
        return flipped;
    }

    [[nodiscard]] bool isMarqueeActive() const
    {
        return marqueeActive;
    }

    [[nodiscard]] bool isPoweredOn() const
    {
        return poweredOn;
    }

    [[nodiscard]] const std::string& getHeaderLeft() const
    {
        return headerLeft;
    }

    [[nodiscard]] const std::string& getHeaderRight() const
    {
        return headerRight;
    }

    [[nodiscard]] const std::vector<std::string>& getBodyLines() const
    {
        return bodyLines;
    }

    [[nodiscard]] const std::string& getMarqueeText() const
    {
        return marqueeText;
    }

    [[nodiscard]] int16_t getMarqueeOffset() const
    {
        return marqueeOffset;
    }

    [[nodiscard]] bool isMarqueeInPause() const
    {
        return marqueeInPause;
    }

    [[nodiscard]] bool isDirty() const
    {
        return dirty;
    }

    [[nodiscard]] const std::string& getToastMessage() const
    {
        return toastMessage;
    }

    [[nodiscard]] bool isSentenceActive( uint32_t nowMs = 0 ) const;

    [[nodiscard]] uint32_t getSentenceDurationMs() const
    {
        return sentenceDurationMs;
    }

    void stopSentenceScroll();

    [[nodiscard]] bool isVerticalScrollActive() const
    {
        return vScrollActive;
    }

    [[nodiscard]] int16_t getVerticalScrollOffset() const
    {
        return vScrollOffset;
    }

    [[nodiscard]] const std::vector<std::string>& getVerticalScrollLines() const
    {
        return vScrollLines;
    }

private:
    Adafruit_SSD1306 internalDisplay;
    Adafruit_SSD1306& display;

    bool dirty = false;
    bool poweredOn = true;
    bool flipped = false;
    uint32_t lastFrameTime = 0;

    // Toast state
    std::string toastMessage;
    uint32_t toastExpiresMs = 0;

    // Header state
    std::string headerLeft;
    std::string headerRight;
    bool headerCentered = false;

    // Marquee state
    bool marqueeActive = false;
    std::string marqueeText;
    int marqueeY = ZONE_BODY_Y + 8;
    int marqueeTextSize = 2;
    int16_t marqueeOffset = 0;
    int16_t marqueeTextWidth = 0;
    uint16_t marqueeSpeedMs = 40;
    uint16_t marqueePauseMs = 1200;
    uint32_t marqueeLastStepTime = 0;
    bool marqueeInPause = false;
    bool marqueeStarted = false;

    // Vertical upward scroll state (movie credits style)
    bool vScrollActive = false;
    std::vector<std::string> vScrollLines;
    int16_t vScrollOffset = 0;
    int16_t vScrollMaxOffset = 0;
    uint16_t vScrollSpeedMs = 40;
    uint16_t vScrollInitialPauseMs = 4000;
    uint16_t vScrollEndPauseMs = 5000;
    uint32_t vScrollLastStepTime = 0;
    bool vScrollInInitialPause = false;
    bool vScrollInEndPause = false;

    // Sentence duration & timing state
    uint32_t sentenceStartTime = 0;
    uint32_t sentenceDurationMs = 0;

    // Body state
    std::vector<std::string> bodyLines;

    void renderHeader();
    void renderMarquee();
    void renderVerticalScroll();
    void sendBuffer();
};

#endif // FRIENDLYBOT_DISPLAY_H
