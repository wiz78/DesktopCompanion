//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/renderers/digitsBarRenderer.h"
#include "display.h"
#include "i18n.h"

#include <algorithm>
#include <cstdio>

void DigitsBarRenderer::drawHeader( DisplayManager& display, const char *headerText )
{
    auto& driver = display.getDriver();

    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );

    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;

    driver.getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );

    const auto hx = static_cast<int16_t>(( DisplayManager::SCREEN_WIDTH - bw ) / 2);

    driver.setCursor( std::max<int16_t>( 0, hx ), 4 );
    driver.print( headerText );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );
}

void DigitsBarRenderer::drawTimeDigits( DisplayManager& display, const uint16_t seconds )
{
    auto& driver = display.getDriver();
    char timeBuffer[ 16 ];
    const uint16_t mins = seconds / 60;
    const uint16_t secs = seconds % 60;

    std::snprintf( timeBuffer, sizeof( timeBuffer ), "%02u:%02u", mins, secs );

    driver.setTextSize( 3 );
    driver.setTextColor( SSD1306_WHITE );
    // Font size 3 is 18x24 pixels. 5 chars = 90px width.
    // Centered horizontally: (128 - 90) / 2 = 19
    driver.setCursor( 19, 23 );
    driver.print( timeBuffer );
}

void DigitsBarRenderer::drawProgressBar( DisplayManager& display, const uint16_t remainingSeconds,
                                         const uint16_t totalSeconds )
{
    auto& driver = display.getDriver();
    constexpr int16_t BAR_X = 14;
    constexpr int16_t BAR_Y = 54;
    constexpr int16_t BAR_W = 100;
    constexpr int16_t BAR_H = 6;

    driver.drawRect( BAR_X, BAR_Y, BAR_W, BAR_H, SSD1306_WHITE );

    if( totalSeconds > 0 ) {
        const uint16_t elapsed = ( totalSeconds > remainingSeconds ) ? ( totalSeconds - remainingSeconds ) : 0;

        if( const auto fillW = static_cast<int16_t>(( ( BAR_W - 4 ) * elapsed ) / totalSeconds); fillW > 0 ) {
            driver.fillRect( BAR_X + 2, BAR_Y + 2, std::min<int16_t>( fillW, BAR_W - 4 ), BAR_H - 4, SSD1306_WHITE );
        }
    }
}

void DigitsBarRenderer::renderSetting( DisplayManager& display, const uint16_t targetSeconds )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    drawHeader( display, I18n::instance().get( StringId::TimerTitle ) );
    drawTimeDigits( display, targetSeconds );
    drawProgressBar( display, targetSeconds, targetSeconds );

    driver.display();
}

void DigitsBarRenderer::renderRunning( DisplayManager& display, const uint16_t remainingSeconds,
                                       const uint16_t totalSeconds )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    drawHeader( display, I18n::instance().get( StringId::TimerTitle ) );
    drawTimeDigits( display, remainingSeconds );
    drawProgressBar( display, remainingSeconds, totalSeconds );

    driver.display();
}

void DigitsBarRenderer::renderPaused( DisplayManager& display, const uint16_t remainingSeconds,
                                      const uint16_t totalSeconds, const bool blinkPhase )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    const char *header = blinkPhase ? I18n::instance().get( StringId::TimerPaused ) : "";
    drawHeader( display, header );
    drawTimeDigits( display, remainingSeconds );
    drawProgressBar( display, remainingSeconds, totalSeconds );

    driver.display();
}

void DigitsBarRenderer::renderAlarming( DisplayManager& display, const bool invertPhase )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    drawHeader( display, I18n::instance().get( StringId::TimerTimeUp ) );
    drawTimeDigits( display, 0 );

    driver.display();
    display.invertDisplay( invertPhase );
}
