//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/renderers/digitalSandRenderer.h"
#include "display.h"
#include "i18n.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// this is not complete

DigitalSandRenderer::DigitalSandRenderer()
{
    reset( 60 );
}

void DigitalSandRenderer::reset( const uint16_t totalSeconds )
{
    (void)totalSeconds;
    droppedCount = 0;
    // Distribute grains in top chamber
    for( size_t i = 0; i < NUM_GRAINS; ++i ) {
        grains[ i ].x = static_cast<int8_t>(60 + ( i % 9 ));
        grains[ i ].y = static_cast<int8_t>(22 + ( i / 9 ));
        grains[ i ].dropped = false;
    }
}

void DigitalSandRenderer::updatePhysics( const float accelX, const float accelY )
{
    gravityX = std::clamp( accelX, -2.0f, 2.0f );
    gravityY = std::clamp( accelY, -2.0f, 2.0f );

    const int8_t shiftX = ( gravityX > 0.3f ) ? 1 : ( ( gravityX < -0.3f ) ? -1 : 0 );

    for( size_t i = 0; i < droppedCount; ++i ) {
        if( grains[ i ].dropped && grains[ i ].y < 58 ) {
            grains[ i ].y += 1;
            grains[ i ].x += shiftX;
            grains[ i ].x = std::clamp<int8_t>( grains[ i ].x, 50, 78 );
        }
    }
}

void DigitalSandRenderer::drawHourglassFrame( DisplayManager& display )
{
    auto& driver = display.getDriver();
    // Centered hourglass frame in blue body
    driver.drawLine( 50, 20, 78, 20, SSD1306_WHITE );
    driver.drawLine( 50, 20, 62, 38, SSD1306_WHITE );
    driver.drawLine( 78, 20, 66, 38, SSD1306_WHITE );

    driver.drawLine( 62, 38, 62, 42, SSD1306_WHITE );
    driver.drawLine( 66, 38, 66, 42, SSD1306_WHITE );

    driver.drawLine( 62, 42, 50, 60, SSD1306_WHITE );
    driver.drawLine( 66, 42, 78, 60, SSD1306_WHITE );
    driver.drawLine( 50, 60, 78, 60, SSD1306_WHITE );
}

void DigitalSandRenderer::drawGrains( DisplayManager& display )
{
    auto& driver = display.getDriver();
    for( size_t i = 0; i < NUM_GRAINS; ++i ) {
        driver.drawPixel( grains[ i ].x, grains[ i ].y, SSD1306_WHITE );
    }
}

void DigitalSandRenderer::renderSetting( DisplayManager& display, const uint16_t targetSeconds )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    char buf[ 32 ];
    std::snprintf( buf, sizeof( buf ), "%s %02u:%02u", I18n::instance().get( StringId::TimerTitle ), targetSeconds / 60,
                   targetSeconds % 60 );
    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );
    driver.setCursor( 4, 4 );
    driver.print( buf );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    drawHourglassFrame( display );
    reset( targetSeconds );
    drawGrains( display );

    driver.display();
}

void DigitalSandRenderer::renderRunning( DisplayManager& display, const uint16_t remainingSeconds,
                                         const uint16_t totalSeconds )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    char buf[ 32 ];
    std::snprintf( buf, sizeof( buf ), "%02u:%02u", remainingSeconds / 60, remainingSeconds % 60 );
    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );
    driver.setCursor( 4, 4 );
    driver.print( buf );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    // Update target dropped count based on elapsed ratio
    if( totalSeconds > 0 ) {
        const uint16_t elapsed = ( totalSeconds > remainingSeconds ) ? ( totalSeconds - remainingSeconds ) : 0;
        const size_t shouldDrop = ( NUM_GRAINS * elapsed ) / totalSeconds;
        while( droppedCount < shouldDrop && droppedCount < NUM_GRAINS ) {
            grains[ droppedCount ].dropped = true;
            grains[ droppedCount ].x = 64;
            grains[ droppedCount ].y = 44;
            droppedCount++;
        }
    }

    drawHourglassFrame( display );
    drawGrains( display );

    driver.display();
}

void DigitalSandRenderer::renderPaused( DisplayManager& display, const uint16_t remainingSeconds,
                                        const uint16_t totalSeconds, const bool blinkPhase )
{
    renderRunning( display, remainingSeconds, totalSeconds );
    if( blinkPhase ) {
        auto& driver = display.getDriver();
        driver.setTextSize( 1 );
        driver.setTextColor( SSD1306_WHITE );
        driver.setCursor( 70, 4 );
        driver.print( I18n::instance().get( StringId::TimerPaused ) );
        driver.display();
    }
}

void DigitalSandRenderer::renderAlarming( DisplayManager& display, const bool invertPhase )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();

    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );
    driver.setCursor( 10, 4 );
    driver.print( I18n::instance().get( StringId::TimerTimeUp ) );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    driver.setTextSize( 2 );
    driver.setCursor( 34, 30 );
    driver.print( "00:00" );

    driver.display();
    display.invertDisplay( invertPhase );
}
