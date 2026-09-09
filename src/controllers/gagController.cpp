//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/gagController.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace
{
    uint32_t xorshift32( uint32_t& state )
    {
        uint32_t x = state;
        x ^= x << 13U;
        x ^= x >> 17U;
        x ^= x << 5U;
        state = ( x == 0 ) ? 0x12345678 : x;
        return state;
    }
} // namespace

GagController::GagController( DisplayManager& display, Buzzer& buzzer, MotionSensor& motion, LdrSensor& ldr,
                              PowerSense& powerSense ) : StateMachine<GagController, GagState>( GagState::Idle, {
                                                             { GagState::Idle, &GagController::idleHandler },
                                                             {
                                                                 GagState::Glitching,
                                                                 &GagController::glitchingHandler
                                                             },
                                                             {
                                                                 GagState::RejectingButton,
                                                                 &GagController::rejectingButtonHandler
                                                             },
                                                             {
                                                                 GagState::RepairingAnimation,
                                                                 &GagController::repairingAnimationHandler
                                                             },
                                                             {
                                                                 GagState::RepairedSuccess,
                                                                 &GagController::repairedSuccessHandler
                                                             }
                                                         } ), display( display ), buzzer( buzzer ),
                                                         motion( motion ), ldr( ldr ), powerSense( powerSense )
{
}

void GagController::init()
{
    setState( GagState::Idle );
    lastTriggerTime = 0;
    triggerCount = 0;
    noiseSeed = 0x12345678;
    display.invertDisplay( false );
}

bool GagController::canTrigger( const uint32_t nowMs ) const
{
    const uint32_t currentMs = ( nowMs == 0 ) ? millis() : nowMs;

    if( getState() != GagState::Idle ) {
        return false;
    }

    if( !powerSense.isUsbPowered() ) {
        return false;
    }

    if( ldr.isNight() ) {
        return false;
    }

    if( triggerCount >= MAX_DAILY_BUDGET ) {
        return false;
    }

    if( ( lastTriggerTime != 0 ) && ( ( currentMs - lastTriggerTime ) < MIN_COOLDOWN_MS ) ) {
        return false;
    }

    return true;
}

bool GagController::trigger( const uint32_t nowMs, const bool force )
{
    const uint32_t currentMs = ( nowMs == 0 ) ? millis() : nowMs;

    if( !force && !canTrigger( currentMs ) ) {
        return false;
    }

    if( triggerCount < 255 ) {
        ++triggerCount;
    }
    lastTriggerTime = currentMs;

    display.setPower( true );
    setState( GagState::Glitching );
    renderGlitchFrame();
    return true;
}

void GagController::onActionButton( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs == 0 ) ? millis() : nowMs;

    if( getState() == GagState::Glitching ) {
        setState( GagState::RejectingButton );
        buzzer.beep( 40, currentMs );
        renderRejectFrame();
    } else if( getState() == GagState::RejectingButton ) {
        display.invertDisplay( false );
        setState( GagState::RepairingAnimation );
        renderRepairFrame();
    }
}

void GagController::onSlap( [[maybe_unused]] const uint32_t nowMs )
{
    if( getState() == GagState::Glitching || getState() == GagState::RejectingButton ) {
        display.invertDisplay( false );
        setState( GagState::RepairingAnimation );
        renderRepairFrame();
    }
}

void GagController::idleHandler()
{
}

void GagController::glitchingHandler()
{
    if( stateElapsed( GLITCH_TIMEOUT_MS ) ) {
        setState( GagState::Idle );
        display.invertDisplay( false );
        display.setPower( false );
    } else {
        renderGlitchFrame();
    }
}

void GagController::rejectingButtonHandler()
{
    if( stateElapsed( REJECT_DURATION_MS ) ) {
        setState( GagState::Glitching );
        renderGlitchFrame();
    } else {
        renderRejectFrame();
    }
}

void GagController::repairingAnimationHandler()
{
    if( stateElapsed( FLASH_DURATION_MS + COLLAPSE_DURATION_MS ) ) {
        display.invertDisplay( false );
        setState( GagState::RepairedSuccess );
        buzzer.doubleBeep( 40, 40, millis() );
        renderSuccessFrame();
    } else {
        renderRepairFrame();
    }
}

void GagController::repairedSuccessHandler()
{
    if( stateElapsed( SUCCESS_DURATION_MS ) ) {
        setState( GagState::Idle );
        display.invertDisplay( false );
        display.clear();
    }
}

void GagController::update( [[maybe_unused]] const uint32_t nowMs )
{
    runStateMachine();
}

void GagController::renderGlitchFrame()
{
    const bool inverted = ( ( getStateTime() / 500 ) % 2 ) == 1;

    display.getDriver().fillRect( 0, 0, DisplayManager::SCREEN_WIDTH, DisplayManager::ZONE_HEADER_H,
                                  inverted ? SSD1306_WHITE : SSD1306_BLACK );
    display.getDriver().setTextColor( inverted ? SSD1306_BLACK : SSD1306_WHITE );
    display.getDriver().setTextSize( 1 );

    const char *headerText = I18n::instance().get( StringId::SlapHeader );
    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;
    display.getDriver().getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t hx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    display.getDriver().setCursor( std::max<int16_t>( 0, hx ), 4 );
    display.getDriver().print( headerText );
    display.getDriver().drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH,
                                       inverted ? SSD1306_BLACK : SSD1306_WHITE );

    if( uint8_t *buf = display.getDriver().getBuffer() ) {
        auto *words = reinterpret_cast<uint32_t *>(buf + 256);
        for( size_t i = 0; i < 192; ++i ) {
            words[ i ] = xorshift32( noiseSeed );
        }
    }

    const char *line1 = I18n::instance().get( StringId::SlapSignalLost );
    const char *line2 = I18n::instance().get( StringId::SlapPrompt );

    display.getDriver().setTextSize( 1 );
    display.getDriver().setTextColor( SSD1306_WHITE );

    display.getDriver().getTextBounds( line1, 0, 0, &bx, &by, &bw, &bh );
    const int16_t x1 = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    const int16_t y1 = 26;
    display.getDriver().fillRect( std::max<int16_t>( 0, x1 - 3 ), y1 - 2, bw + 6, bh + 4, SSD1306_BLACK );
    display.getDriver().setCursor( std::max<int16_t>( 0, x1 ), y1 );
    display.getDriver().print( line1 );

    display.getDriver().getTextBounds( line2, 0, 0, &bx, &by, &bw, &bh );
    const int16_t x2 = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    const int16_t y2 = 44;
    display.getDriver().fillRect( std::max<int16_t>( 0, x2 - 3 ), y2 - 2, bw + 6, bh + 4, SSD1306_BLACK );
    display.getDriver().setCursor( std::max<int16_t>( 0, x2 ), y2 );
    display.getDriver().print( line2 );

    display.getDriver().display();
}

void GagController::renderRejectFrame()
{
    display.getDriver().fillRect( 0, 0, DisplayManager::SCREEN_WIDTH, DisplayManager::ZONE_HEADER_H, SSD1306_WHITE );
    display.getDriver().setTextColor( SSD1306_BLACK );
    display.getDriver().setTextSize( 1 );

    const char *headerText = I18n::instance().get( StringId::SlapHeader );
    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;
    display.getDriver().getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t hx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    display.getDriver().setCursor( std::max<int16_t>( 0, hx ), 4 );
    display.getDriver().print( headerText );
    display.getDriver().drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH,
                                       SSD1306_BLACK );

    display.getDriver().fillRect( 0, DisplayManager::ZONE_BODY_Y, DisplayManager::SCREEN_WIDTH,
                                  DisplayManager::ZONE_BODY_H, SSD1306_BLACK );
    display.getDriver().setTextColor( SSD1306_WHITE );
    display.getDriver().setTextSize( 1 );

    const char *rejectText = I18n::instance().get( StringId::SlapReject );
    display.getDriver().getTextBounds( rejectText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t rx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    const int16_t ry = DisplayManager::ZONE_BODY_Y + ( DisplayManager::ZONE_BODY_H - static_cast<int16_t>(bh) ) / 2;
    display.getDriver().setCursor( std::max<int16_t>( 0, rx ), ry );
    display.getDriver().print( rejectText );

    display.getDriver().display();
}

void GagController::renderRepairFrame()
{
    const uint32_t elapsed = getStateTime();

    if( elapsed < FLASH_DURATION_MS ) {
        display.getDriver().fillRect( 0, 0, DisplayManager::SCREEN_WIDTH, DisplayManager::SCREEN_HEIGHT,
                                      SSD1306_WHITE );
        display.getDriver().display();
    } else {
        const uint32_t collapseElapsed = elapsed - FLASH_DURATION_MS;
        const float progress = static_cast<float>(collapseElapsed) / static_cast<float>(COLLAPSE_DURATION_MS);
        const float clampedProgress = std::clamp( progress, 0.0f, 1.0f );
        const int h = static_cast<int>(48.0f * ( 1.0f - clampedProgress ));

        display.getDriver().clearDisplay();

        if( h <= 1 ) {
            display.getDriver().drawFastHLine( 0, 39, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );
        } else {
            if( uint8_t *buf = display.getDriver().getBuffer() ) {
                auto *words = reinterpret_cast<uint32_t *>(buf + 256);
                for( size_t i = 0; i < 192; ++i ) {
                    words[ i ] = xorshift32( noiseSeed );
                }
            }

            const int yTop = 39 - ( h / 2 );

            if( yTop > 0 ) {
                display.getDriver().fillRect( 0, 0, DisplayManager::SCREEN_WIDTH, static_cast<int16_t>(yTop),
                                              SSD1306_BLACK );
            }

            if( const int yBottom = yTop + h; yBottom < DisplayManager::SCREEN_HEIGHT ) {
                display.getDriver().fillRect( 0, static_cast<int16_t>(yBottom), DisplayManager::SCREEN_WIDTH,
                                              static_cast<int16_t>(DisplayManager::SCREEN_HEIGHT - yBottom),
                                              SSD1306_BLACK );
            }
        }

        display.getDriver().display();
    }
}

void GagController::renderSuccessFrame()
{
    display.setCenteredHeader( I18n::instance().get( StringId::SlapRepaired ) );
    display.displaySentence( I18n::instance().get( StringId::SlapSubtext ), 30, millis() );
}
