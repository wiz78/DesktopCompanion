//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "display.h"
#include "services/webUtils.h"

#include <algorithm>
#include <cstdio>

#include <Adafruit_GFX.h>
#include <Wire.h>

static constexpr int I2C_ADDRESS = 0x3C;
static constexpr int I2C_RESET_PIN = -1;

DisplayManager::DisplayManager() : internalDisplay( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, I2C_RESET_PIN ),
                                   display( internalDisplay )
{
}

DisplayManager::DisplayManager( Adafruit_SSD1306& rawDisplay ) : internalDisplay( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,
                                                                     I2C_RESET_PIN ), display( rawDisplay )
{
}

void DisplayManager::setup()
{
    display.begin( SSD1306_SWITCHCAPVCC, I2C_ADDRESS, false, false );
    display.setTextColor( SSD1306_WHITE );
    display.clearDisplay();
    display.display();
}

void DisplayManager::clear()
{
    display.clearDisplay();
    dirty = true;
    marqueeActive = false;
    vScrollActive = false;
    sentenceDurationMs = 0;
    toastExpiresMs = 0;
}

void DisplayManager::clearHeader()
{
    display.fillRect( 0, 0, SCREEN_WIDTH, ZONE_HEADER_H, SSD1306_BLACK );
    dirty = true;
}

void DisplayManager::clearBody()
{
    display.fillRect( 0, ZONE_BODY_Y, SCREEN_WIDTH, ZONE_BODY_H, SSD1306_BLACK );
    dirty = true;
    marqueeActive = false;
    vScrollActive = false;
    sentenceDurationMs = 0;
    bodyLines.clear();
}

void DisplayManager::stopSentenceScroll()
{
    vScrollActive = false;
    sentenceDurationMs = 0;
    dirty = true;
}

bool DisplayManager::isSentenceActive( const uint32_t nowMs ) const
{
    const uint32_t currentMs = ( nowMs == 0 ) ? millis() : nowMs;
    if( sentenceDurationMs > 0 && ( currentMs - sentenceStartTime ) < sentenceDurationMs ) {
        return true;
    }
    return false;
}

void DisplayManager::setHeader( const std::string& leftText, const std::string& rightText, const uint32_t nowMs )
{
    headerLeft = leftText;
    headerRight = rightText;
    headerCentered = false;
    if( isToastActive( nowMs ) ) {
        return;
    }
    renderHeader();
}

void DisplayManager::setCenteredHeader( const std::string& centerText, const uint32_t nowMs )
{
    headerLeft = centerText;
    headerRight.clear();
    headerCentered = true;
    if( isToastActive( nowMs ) ) {
        return;
    }
    renderHeader();
}

void DisplayManager::showToast( const std::string& msg, const uint32_t durationMs, const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();
    toastMessage = msg;
    toastExpiresMs = currentMs + durationMs;

    // Render toast centered in yellow header zone (Y = 0..15)
    clearHeader();
    drawCenteredText( msg.c_str(), 4, 1 );
    display.drawFastHLine( 0, ZONE_HEADER_H - 1, SCREEN_WIDTH, SSD1306_WHITE );
    render();
}

bool DisplayManager::isToastActive( const uint32_t nowMs ) const
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();
    return ( toastExpiresMs != 0 ) && ( currentMs < toastExpiresMs );
}

void DisplayManager::render()
{
    sendBuffer();
}

void DisplayManager::drawCenteredText( const char *text, const int16_t y, const uint8_t textSize )
{
    display.setTextSize( textSize );
    display.setTextColor( SSD1306_WHITE );

    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;
    display.getTextBounds( text, 0, 0, &bx, &by, &bw, &bh );
    const int x = ( SCREEN_WIDTH - bw ) / 2;
    display.setCursor( static_cast<int16_t>(std::max( 0, x )), y );
    display.print( text );
}

void DisplayManager::renderHeader()
{
    clearHeader();
    display.setTextSize( 1 );
    display.setTextColor( SSD1306_WHITE );

    if( headerCentered ) {
        int16_t bx = 0;
        int16_t by = 0;
        uint16_t bw = 0;
        uint16_t bh = 0;
        display.getTextBounds( headerLeft.c_str(), 0, 0, &bx, &by, &bw, &bh );
        const int16_t hx = ( SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
        display.setCursor( std::max<int16_t>( 0, hx ), 4 );
        display.print( headerLeft.c_str() );
    } else {
        // Draw Left text
        display.setCursor( 0, 4 );
        display.print( headerLeft.c_str() );

        // Draw Right text
        if( !headerRight.empty() ) {
            int16_t bx = 0;
            int16_t by = 0;
            uint16_t bw = 0;
            uint16_t bh = 0;
            display.getTextBounds( headerRight.c_str(), 0, 0, &bx, &by, &bw, &bh );
            display.setCursor( static_cast<int16_t>(SCREEN_WIDTH - bw), 4 );
            display.print( headerRight.c_str() );
        }
    }

    // Horizontal separator
    display.drawFastHLine( 0, ZONE_HEADER_H - 1, SCREEN_WIDTH, SSD1306_WHITE );
    dirty = true;
}

void DisplayManager::setBodyText( const std::vector<std::string>& lines, const int textSize )
{
    const std::vector<std::string> localLines = lines;
    clearBody();
    bodyLines = localLines;
    display.setTextSize( textSize );

    const int lineHeight = textSize * 8;
    int y = ZONE_BODY_Y + ( ( ZONE_BODY_H - ( static_cast<int>(bodyLines.size()) * lineHeight ) ) / 2 );

    for( const auto& line : bodyLines ) {
        int16_t bx = 0;
        int16_t by = 0;
        uint16_t bw = 0;
        uint16_t bh = 0;

        display.getTextBounds( line.c_str(), 0, 0, &bx, &by, &bw, &bh );

        const int x = ( SCREEN_WIDTH - bw ) / 2;

        display.setCursor( static_cast<int16_t>(std::max( 0, x )), static_cast<int16_t>(y) );
        display.print( line.c_str() );
        y += lineHeight;
    }

    dirty = true;
}

void DisplayManager::setScrollingText( const std::string& text, const int y, const int textSize, const uint16_t speedMs,
                                       const uint16_t pauseMs )
{
    marqueeText = text;
    marqueeY = y;
    marqueeTextSize = textSize;
    marqueeSpeedMs = speedMs;
    marqueePauseMs = pauseMs;
    marqueeActive = true;
    marqueeInPause = true;
    marqueeStarted = false;
    marqueeOffset = 0;
    marqueeLastStepTime = 0;

    // Approximate width: 6px * size per character
    marqueeTextWidth = static_cast<int16_t>(text.length() * 6 * textSize);

    if( marqueeTextWidth <= SCREEN_WIDTH ) {
        // Fits on screen statically
        clearBody();
        display.setTextSize( textSize );
        const int x = ( SCREEN_WIDTH - marqueeTextWidth ) / 2;
        display.setCursor( x, marqueeY );
        display.print( text.c_str() );
        dirty = true;
        marqueeActive = false;
    }
}

bool DisplayManager::displaySentence( const std::string& sentence, const size_t maxWrapChars, const uint32_t nowMs,
                                      const uint16_t initialPause, const uint16_t endPause )
{
    marqueeActive = false;
    sentenceStartTime = nowMs ? nowMs : millis();

    const bool fits = TextFormatter::canFitInLines( sentence, TextFormatter::MAX_CHARS_PER_LINE,
                                                    TextFormatter::MAX_WRAPPED_LINES, maxWrapChars );

    if( fits ) {

        vScrollLines.clear();

        vScrollActive = false;
        bodyLines = TextFormatter::wrapText( sentence );

        setBodyText( bodyLines, 2 );

        sentenceDurationMs = 15000;

    } else {

        bodyLines = TextFormatter::wrapText( sentence, TextFormatter::MAX_CHARS_PER_LINE );
        vScrollLines = bodyLines;
        vScrollOffset = 0;
        vScrollMaxOffset = static_cast<int16_t>(( static_cast<int>( vScrollLines.size() ) - 3 ) * 16 );
        vScrollMaxOffset = std::max<int16_t>( vScrollMaxOffset, 0 );
        vScrollSpeedMs = 40;
        vScrollInitialPauseMs = initialPause;
        vScrollEndPauseMs = endPause;
        vScrollInInitialPause = true;
        vScrollInEndPause = false;
        vScrollActive = true;
        vScrollLastStepTime = sentenceStartTime;
        sentenceDurationMs = vScrollInitialPauseMs + static_cast<uint32_t>( vScrollMaxOffset * vScrollSpeedMs ) +
                             vScrollEndPauseMs;

        renderVerticalScroll();
    }

    return fits;
}

void DisplayManager::displaySetupScreen( const char *ssid, const char *ip, const char *url, const uint32_t nowMs )
{
    display.clearDisplay();
    marqueeActive = false;

    // Header (Yellow zone): Draw string centered: "SETUP PORTAL"
    drawCenteredText( "SETUP PORTAL", 4, 1 );

    // Divider line at y=16
    display.drawFastHLine( 0, 16, SCREEN_WIDTH, SSD1306_WHITE );

    // Compute scroll step
    const size_t step = static_cast<size_t>(nowMs / 250);

    // Blue zone lines
    // Line 1 (y=20): AP: <ssid>
    const std::string line1 = WebUtils::formatMarqueeField( "AP:  ", ( ssid != nullptr ) ? ssid : "Console50-Setup", 21,
                                                            step );
    display.setCursor( 0, 20 );
    display.print( line1.c_str() );

    // Line 2 (y=31): IP: <ip>
    const std::string line2 = WebUtils::formatMarqueeField( "IP:  ", ( ip != nullptr ) ? ip : "192.168.4.1", 21, step );
    display.setCursor( 0, 31 );
    display.print( line2.c_str() );

    // Line 3 (y=42): URL: <url>
    const std::string line3 = WebUtils::formatMarqueeField( "URL: ", ( url != nullptr ) ? url : "bot50.local", 21,
                                                            step );
    display.setCursor( 0, 42 );
    display.print( line3.c_str() );

    // Line 4 (y=53): [Action: skip]
    display.setCursor( 0, 53 );
    display.print( "[Action: skip]" );

    display.display();
    dirty = false;
}

void DisplayManager::displayResetCountdown( const uint8_t secondsRemaining )
{
    display.clearDisplay();
    marqueeActive = false;

    // Header (Yellow zone): Draw string centered: "ALERT"
    drawCenteredText( "ALERT", 4, 1 );

    // Divider line at y=16
    display.drawFastHLine( 0, 16, SCREEN_WIDTH, SSD1306_WHITE );

    // Blue zone:
    // Line 1: Centered text "RESET IN" at y=22, text size 1
    drawCenteredText( "RESET IN", 22, 1 );

    // Line 2: Centered large digit at y=36, text size 3
    const char num[ 2 ] = { static_cast<char>('0' + secondsRemaining), '\0' };
    drawCenteredText( num, 36, 3 );

    display.display();
    dirty = false;
}

void DisplayManager::displayCrtShutter( const uint8_t frame )
{
    marqueeActive = false;
    switch( frame ) {
        case 0:
            // Frame 0: collapsing curtains: fill top rectangle (0..16) and bottom rectangle (48..63) with black, leaving center band.
            display.fillRect( 0, 0, SCREEN_WIDTH, 16, SSD1306_BLACK );
            display.fillRect( 0, 48, SCREEN_WIDTH, 16, SSD1306_BLACK );
            break;
        case 1:
            // Frame 1: clear buffer, draw single 2px horizontal line across center (Y=31..32, X=0..127).
            display.clearDisplay();
            display.fillRect( 0, 31, SCREEN_WIDTH, 2, SSD1306_WHITE );
            break;
        case 2:
            // Frame 2: clear buffer, draw center phosphor rectangle (X=56..72, Y=31..32).
            display.clearDisplay();
            display.fillRect( 56, 31, 16, 2, SSD1306_WHITE );
            break;
        case 3: default:
            // Frame 3: clear buffer completely.
            display.clearDisplay();
            break;
    }
    display.display();
    dirty = false;
}

void DisplayManager::displayDiagnostics( const float leftPotNorm, const float leftPotMinutes,
                                         const uint8_t rightPotCategory, const uint16_t ldrRaw, const bool isNight,
                                         const bool isUsb, const uint16_t distanceMm, const float tempC,
                                         const bool shockDetected, const bool shakeDetected )
{
    display.clearDisplay();
    marqueeActive = false;

    // Header (Yellow zone): Draw string centered: "[ DIAGNOSTICA ]"
    drawCenteredText( "[ DIAGNOSTICA ]", 4, 1 );

    // Divider line at y=16
    display.drawFastHLine( 0, 16, SCREEN_WIDTH, SSD1306_WHITE );

    display.setTextSize( 1 );
    display.setTextColor( SSD1306_WHITE );
    display.setTextWrap( false );

    // Line 1 (y=20): POT L:%.2f(%.0fm) R:%u
    char line1[ 32 ];
    snprintf( line1, sizeof( line1 ), "POT L:%.2f(%.0fm) R:%u", leftPotNorm, leftPotMinutes, rightPotCategory );
    display.setCursor( 0, 20 );
    display.print( line1 );

    // Line 2 (y=31): LDR:%u(%s) USB:%s
    char line2[ 32 ];
    const char *lightStr = isNight ? "Ngt" : "Day";
    const char *usbStr = isUsb ? "YES" : "NO";
    snprintf( line2, sizeof( line2 ), "LDR:%u(%s) USB:%s", ldrRaw, lightStr, usbStr );
    display.setCursor( 0, 31 );
    display.print( line2 );

    // Line 3 (y=42): ToF:%umm TEMP:%.1fC
    char line3[ 32 ];
    snprintf( line3, sizeof( line3 ), "ToF:%umm TEMP:%.1fC", distanceMm, tempC );
    display.setCursor( 0, 42 );
    display.print( line3 );

    // Line 4 (y=53): SHOCK:%s SHAKE:%s
    char line4[ 32 ];
    const char *shockStr = shockDetected ? "YES" : "NO";
    const char *shakeStr = shakeDetected ? "YES" : "NO";
    snprintf( line4, sizeof( line4 ), "SHOCK:%s SHAKE:%s", shockStr, shakeStr );
    display.setCursor( 0, 53 );
    display.print( line4 );

    display.setTextWrap( true );
    display.display();
    dirty = false;
}

void DisplayManager::renderMarquee()
{
    display.fillRect( 0, ZONE_BODY_Y, SCREEN_WIDTH, ZONE_BODY_H, SSD1306_BLACK );
    display.setTextSize( marqueeTextSize );
    display.setCursor( static_cast<int16_t>(-marqueeOffset), static_cast<int16_t>(marqueeY) );
    display.print( marqueeText.c_str() );
    dirty = true;
}

void DisplayManager::renderVerticalScroll()
{
    display.fillRect( 0, ZONE_BODY_Y, SCREEN_WIDTH, ZONE_BODY_H, SSD1306_BLACK );
    display.setTextWrap( false );
    display.setTextSize( 2 );

    for( size_t i = 0; i < vScrollLines.size(); ++i ) {
        constexpr int16_t lineHeight = 16;
        if( const auto y = static_cast<int16_t>(ZONE_BODY_Y + ( static_cast<int>(i) * lineHeight ) - vScrollOffset);
            y > ( ZONE_BODY_Y - lineHeight ) && y < SCREEN_HEIGHT ) {
            int16_t bx = 0;
            int16_t by = 0;
            uint16_t bw = 0;
            uint16_t bh = 0;
            display.getTextBounds( vScrollLines[ i ].c_str(), 0, 0, &bx, &by, &bw, &bh );
            const auto x = static_cast<int16_t>(( SCREEN_WIDTH - bw ) / 2);
            display.setCursor( std::max<int16_t>( 0, x ), y );
            display.print( vScrollLines[ i ].c_str() );
        }
    }

    // Mask header zone so lines scrolling upward never bleed into the yellow header
    renderHeader();

    dirty = true;
}

void DisplayManager::drawProgressBar( const int16_t x, const int16_t y, const int16_t w, const int16_t h,
                                      const float percent )
{
    display.drawRect( x, y, w, h, SSD1306_WHITE );

    if( const auto fillW = static_cast<int16_t>(std::clamp( percent, 0.0f, 1.0f ) * static_cast<float>(w - 4));
        fillW > 0 )
        display.fillRect( static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 2), fillW, static_cast<int16_t>(h - 4),
                          SSD1306_WHITE );

    dirty = true;
}

void DisplayManager::setPower( const bool on )
{
    poweredOn = on;
    if( !on ) {
        stopSentenceScroll();
    }
    display.ssd1306_command( on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF );
}

void DisplayManager::setBrightnessLevel( const int brightnessLevel )
{
    display.ssd1306_command( SSD1306_SETCONTRAST );
    display.ssd1306_command( map( brightnessLevel, 0, 3, 0x05, 0xCF ) );
    display.ssd1306_command( SSD1306_SETPRECHARGE );
    display.ssd1306_command( map( brightnessLevel, 0, 3, 0x22, 0xF1 ) );
    display.ssd1306_command( SSD1306_SETVCOMDETECT );
    display.ssd1306_command( map( brightnessLevel, 0, 3, 0x20, 0x40 ) );
}

void DisplayManager::invertDisplay( const bool invert )
{
    display.invertDisplay( invert );
}

void DisplayManager::setFlipped( const bool flip )
{
    if( flipped == flip ) {
        return;
    }

    flipped = flip;
    if( flipped ) {
        display.ssd1306_command( 0xA0 ); // SSD1306_SEGREMAP | 0x0
        display.ssd1306_command( 0xC0 ); // SSD1306_COMSCANINC
    } else {
        display.ssd1306_command( 0xA1 ); // SSD1306_SEGREMAP | 0x1
        display.ssd1306_command( 0xC8 ); // SSD1306_COMSCANDEC
    }
}

void DisplayManager::update( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs == 0 ) ? millis() : nowMs;

    if( toastExpiresMs != 0 && currentMs >= toastExpiresMs ) {
        toastExpiresMs = 0;
        renderHeader();
    }

    if( marqueeActive ) {
        if( !marqueeStarted ) {
            marqueeStarted = true;
            marqueeLastStepTime = currentMs;
            renderMarquee();
        } else if( marqueeInPause ) {
            if( ( currentMs - marqueeLastStepTime ) >= marqueePauseMs ) {
                marqueeInPause = false;
                marqueeLastStepTime = currentMs;
            }
        } else {
            if( ( currentMs - marqueeLastStepTime ) >= marqueeSpeedMs ) {
                marqueeOffset += 2;
                if( marqueeOffset > ( marqueeTextWidth - SCREEN_WIDTH + 10 ) ) {
                    marqueeOffset = 0;
                    marqueeInPause = true;
                }
                marqueeLastStepTime = currentMs;
                renderMarquee();
            }
        }
    }

    if( vScrollActive ) {
        if( vScrollInInitialPause ) {
            if( ( currentMs - vScrollLastStepTime ) >= vScrollInitialPauseMs ) {
                vScrollInInitialPause = false;
                vScrollLastStepTime += vScrollInitialPauseMs;
            }
        }

        if( !vScrollInInitialPause && !vScrollInEndPause ) {
            const uint32_t elapsed = currentMs - vScrollLastStepTime;

            if( const auto steps = static_cast<int16_t>(elapsed / vScrollSpeedMs); steps > 0 ) {
                vScrollOffset += steps;
                vScrollLastStepTime += static_cast<uint32_t>(steps * vScrollSpeedMs);

                if( vScrollOffset >= vScrollMaxOffset ) {
                    vScrollOffset = vScrollMaxOffset;
                    vScrollInEndPause = true;
                    vScrollLastStepTime = currentMs;
                }

                renderVerticalScroll();
            }
        } else if( vScrollInEndPause ) {
            if( ( currentMs - vScrollLastStepTime ) >= vScrollEndPauseMs ) {
                vScrollInEndPause = false;
                vScrollActive = false;
            }
        }
    }

    // Max 30 FPS
    if( dirty && ( ( currentMs - lastFrameTime ) >= 33 ) ) {
        sendBuffer();
        lastFrameTime = currentMs;
    }
}

void DisplayManager::sendBuffer()
{
    display.display();
    dirty = false;
}
