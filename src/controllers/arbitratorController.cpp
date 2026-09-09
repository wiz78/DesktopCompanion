//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "controllers/arbitratorController.h"
#include "textFormatter.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>

#ifdef ESP32
#include <esp_random.h>
#endif

ArbitratorController::ArbitratorController( DisplayManager& display, Potentiometer& rightPot,
                                            MotionSensor& motion ) : StateMachine( ArbitratorState::Idle, {
                                                                         {
                                                                             ArbitratorState::Idle,
                                                                             &ArbitratorController::idleHandler
                                                                         },
                                                                         {
                                                                             ArbitratorState::CategoryBrowsing,
                                                                             &ArbitratorController::categoryBrowsingHandler
                                                                         },
                                                                         {
                                                                             ArbitratorState::CategoryLocked,
                                                                             &ArbitratorController::categoryLockedHandler
                                                                         },
                                                                         {
                                                                             ArbitratorState::Spinning,
                                                                             &ArbitratorController::spinningHandler
                                                                         },
                                                                         {
                                                                             ArbitratorState::Verdict,
                                                                             &ArbitratorController::verdictHandler
                                                                         },
                                                                     } ), display( display ), rightPot( rightPot ),
                                                                     motion( motion )
{
}

void ArbitratorController::init()
{
    setState( ArbitratorState::Idle );
    selectedCategory = 0;
    lastPotMoveTime = 0;
    verdictIndex = 0;
    lastVerdictIndex = 255;
    lastReelStepTime = 0;
    lastBlinkPhase = 255;
}

void ArbitratorController::update( const uint32_t nowMs )
{
    runStateMachine();
    if( getState() == ArbitratorState::Verdict ) {
        display.update( ( nowMs != 0 ) ? nowMs : millis() );
    }
}

void ArbitratorController::activate()
{
    display.stopSentenceScroll();
    display.setPower( true );
    display.invertDisplay( false );
    const uint8_t newCat = readCategoryFromPot();
    if( newCat != selectedCategory ) {
        selectedCategory = newCat;
        lastVerdictIndex = 255;
    }
    lastPotMoveTime = millis();
    setState( ArbitratorState::CategoryBrowsing );
    renderDrumPicker();
}

void ArbitratorController::onPotMoved()
{
    if( getState() == ArbitratorState::Spinning ) {
        return;
    }

    display.stopSentenceScroll();

    const uint8_t newCat = readCategoryFromPot();
    if( newCat != selectedCategory ) {
        selectedCategory = newCat;
        lastVerdictIndex = 255;
    }
    lastPotMoveTime = millis();

    if( getState() == ArbitratorState::Idle || getState() == ArbitratorState::CategoryLocked || getState() ==
        ArbitratorState::Verdict ) {
        display.invertDisplay( false );
        display.setPower( true );
        setState( ArbitratorState::CategoryBrowsing );
    }

    renderDrumPicker();
}

uint8_t ArbitratorController::readCategoryFromPot()
{
    return rightPot.getStep( NUM_CATEGORIES );
}

void ArbitratorController::onActionButton()
{
    if( getState() == ArbitratorState::CategoryLocked || getState() == ArbitratorState::Verdict || getState() ==
        ArbitratorState::CategoryBrowsing ) {
        display.stopSentenceScroll();
        pickRandomVerdict();
        display.invertDisplay( false );
        lastReelStepTime = 0;
        setState( ArbitratorState::Spinning );
    }
}

void ArbitratorController::onShake()
{
    if( getState() == ArbitratorState::CategoryLocked || getState() == ArbitratorState::Verdict ) {
        display.stopSentenceScroll();
        pickRandomVerdict();
        display.invertDisplay( false );
        lastReelStepTime = 0;
        setState( ArbitratorState::Spinning );
    }
}

void ArbitratorController::idleHandler()
{
}

void ArbitratorController::categoryBrowsingHandler()
{
    if( ( millis() - lastPotMoveTime ) >= CATEGORY_LOCK_DELAY_MS ) {
        lastBlinkPhase = 255;
        setState( ArbitratorState::CategoryLocked );
        renderLockedCategory();
    }
}

void ArbitratorController::categoryLockedHandler()
{
    if( stateElapsed( INACTIVITY_TIMEOUT_MS ) ) {
        setState( ArbitratorState::Idle );
        display.clear();
        display.getDriver().display();
        display.setPower( false );
        return;
    }

    const uint32_t currentPhase = ( getStateTime() / BLINK_INTERVAL_MS ) % 2;
    if( currentPhase != lastBlinkPhase ) {
        lastBlinkPhase = currentPhase;
        renderLockedCategory();
    }
}

void ArbitratorController::spinningHandler()
{
    const uint32_t elapsed = getStateTime();
    const float t = std::min( 1.0f, static_cast<float>(elapsed) / static_cast<float>(SPIN_DURATION_MS) );
    const float ease = 1.0f - ( ( 1.0f - t ) * ( 1.0f - t ) );

    const uint32_t now = millis();
    if( t < 1.0f ) {
        if( ( now - lastReelStepTime ) >= 33 ) {
            lastReelStepTime = now;
            renderReelFrame( ease );
        }
    } else {
        setState( ArbitratorState::Verdict );
        renderVerdictFrame();
    }
}

void ArbitratorController::verdictHandler()
{
    const uint32_t timeout = std::max( INACTIVITY_TIMEOUT_MS, display.getSentenceDurationMs() );
    if( stateElapsed( timeout ) ) {
        display.invertDisplay( false );
        displayInverted = false;
        display.stopSentenceScroll();
        setState( ArbitratorState::Idle );
        display.clear();
        display.getDriver().display();
        display.setPower( false );
        return;
    }

    const uint32_t elapsed = getStateTime();
    if( elapsed < 300 ) {
        const bool invert = ( ( elapsed / 150 ) % 2 ) == 1;
        if( displayInverted != invert ) {
            displayInverted = invert;
            display.invertDisplay( invert );
        }
    } else if( displayInverted ) {
        displayInverted = false;
        display.invertDisplay( false );
    }
}

void ArbitratorController::renderDrumPicker()
{
    auto& driver = display.getDriver();
    driver.clearDisplay();
    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );

    // Yellow Header zone: Y = 0..15
    const char *headerText = I18n::instance().get( StringId::OracleHeader );
    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;
    driver.getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t hx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    driver.setCursor( std::max<int16_t>( 0, hx ), 4 );
    driver.print( headerText );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    // Row -1 (Y = 18): Previous category if > 0
    if( selectedCategory > 0 ) {
        const char *prevText = I18n::instance().getOracleCategory( selectedCategory - 1 );
        driver.getTextBounds( prevText, 0, 0, &bx, &by, &bw, &bh );
        const int16_t px = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
        driver.setCursor( std::max<int16_t>( 0, px ), 18 );
        driver.print( prevText );
    }

    // Drum separator lines at Y = 28 and Y = 46
    driver.drawFastHLine( 0, 28, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );
    driver.drawFastHLine( 0, 46, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    // Row 0 (Y = 32): Active category centered and framed
    const char *currText = I18n::instance().getOracleCategory( selectedCategory );
    driver.getTextBounds( currText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t cx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    driver.setCursor( std::max<int16_t>( 0, cx ), 32 );
    driver.print( currText );

    // Row +1 (Y = 50): Next category if < 3
    if( selectedCategory < NUM_CATEGORIES - 1 ) {
        const char *nextText = I18n::instance().getOracleCategory( selectedCategory + 1 );
        driver.getTextBounds( nextText, 0, 0, &bx, &by, &bw, &bh );
        const int16_t nx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
        driver.setCursor( std::max<int16_t>( 0, nx ), 50 );
        driver.print( nextText );
    }

    driver.display();
}

void ArbitratorController::renderLockedCategory()
{
    auto& driver = display.getDriver();
    driver.clearDisplay();
    driver.setTextSize( 1 );
    driver.setTextColor( SSD1306_WHITE );

    // Header: ORACOLO / ORACLE
    const char *headerText = I18n::instance().get( StringId::OracleHeader );
    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;
    driver.getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t hx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    driver.setCursor( std::max<int16_t>( 0, hx ), 4 );
    driver.print( headerText );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    // Row 0 (Y = 26): Selected category name
    const char *catName = I18n::instance().getOracleCategory( selectedCategory );
    driver.getTextBounds( catName, 0, 0, &bx, &by, &bw, &bh );
    const int16_t cx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    driver.setCursor( std::max<int16_t>( 0, cx ), 26 );
    driver.print( catName );

    // Row 1 (Y = 48): Prompt blinking every 500ms: >> PREMI O AGITA <<
    if( ( ( getStateTime() / BLINK_INTERVAL_MS ) % 2 ) == 0 ) {
        const char *prompt = I18n::instance().get( StringId::OraclePromptSpin );
        driver.getTextBounds( prompt, 0, 0, &bx, &by, &bw, &bh );
        const int16_t px = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
        driver.setCursor( std::max<int16_t>( 0, px ), 48 );
        driver.print( prompt );
    }

    driver.display();
}

void ArbitratorController::renderReelFrame( const float progress )
{
    auto& driver = display.getDriver();
    driver.clearDisplay();
    driver.setTextColor( SSD1306_WHITE );

    const auto& verdicts = I18n::instance().getOracleVerdicts( selectedCategory );
    if( verdicts.empty() ) {
        return;
    }

    const size_t numVerdicts = verdicts.size();
    constexpr int rowHeight = 16;
    const size_t totalItemsToScroll = ( numVerdicts * 5 ) + verdictIndex;
    const float totalPixelDistance = static_cast<float>(totalItemsToScroll * rowHeight);
    const float currentPixelOffset = progress * totalPixelDistance;

    const int baseItem = static_cast<int>(currentPixelOffset) / rowHeight;
    const int subPixel = static_cast<int>(currentPixelOffset) % rowHeight;

    int16_t bx = 0;
    int16_t by = 0;
    uint16_t bw = 0;
    uint16_t bh = 0;

    for( int r = -1; r <= 2; ++r ) {
        int itemIndex = ( baseItem + r ) % static_cast<int>(numVerdicts);
        if( itemIndex < 0 ) {
            itemIndex += static_cast<int>(numVerdicts);
        }

        const int16_t rowY = 32 + ( r * rowHeight ) - subPixel;
        if( rowY >= 16 && rowY <= 56 ) {
            const char *text = verdicts[ static_cast<size_t>(itemIndex) ];
            driver.setTextSize( 1 );
            driver.getTextBounds( text, 0, 0, &bx, &by, &bw, &bh );
            const int16_t tx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
            driver.setCursor( std::max<int16_t>( 0, tx ), rowY );
            driver.print( text );
        }
    }

    // Drum separator lines at Y = 28 and Y = 46
    driver.drawFastHLine( 0, 28, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );
    driver.drawFastHLine( 0, 46, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    // Yellow Header zone: Y = 0..15 (cleared so scrolling items don't bleed into header)
    driver.fillRect( 0, 0, DisplayManager::SCREEN_WIDTH, DisplayManager::ZONE_HEADER_H, SSD1306_BLACK );
    const char *headerText = I18n::instance().get( StringId::OracleHeader );
    driver.setTextSize( 1 );
    driver.getTextBounds( headerText, 0, 0, &bx, &by, &bw, &bh );
    const int16_t hx = ( DisplayManager::SCREEN_WIDTH - static_cast<int16_t>(bw) ) / 2;
    driver.setCursor( std::max<int16_t>( 0, hx ), 4 );
    driver.print( headerText );
    driver.drawFastHLine( 0, DisplayManager::ZONE_HEADER_H - 1, DisplayManager::SCREEN_WIDTH, SSD1306_WHITE );

    driver.display();
}

void ArbitratorController::renderVerdictFrame()
{
    const char *headerText = I18n::instance().get( StringId::OracleHeader );
    display.setCenteredHeader( headerText );

    // Winning verdict text
    const auto& verdicts = I18n::instance().getOracleVerdicts( selectedCategory );
    const char *verdictText = ( verdictIndex < verdicts.size() ) ? verdicts[ verdictIndex ] : "";

    display.displaySentence( verdictText, TextFormatter::MAX_WRAPPED_CHARS, millis() );
    display.render();
}

void ArbitratorController::pickRandomVerdict()
{
    const auto& verdicts = I18n::instance().getOracleVerdicts( selectedCategory );
    if( verdicts.empty() ) {
        verdictIndex = 0;
        return;
    }
    if( verdicts.size() == 1 ) {
        verdictIndex = 0;
        lastVerdictIndex = 0;
        return;
    }

    size_t newIndex = 0;
    do {
#ifdef ESP32
        newIndex = esp_random() % verdicts.size();
#else
        static std::random_device rd;
        static std::mt19937 gen( rd() );
        std::uniform_int_distribution<size_t> dist( 0, verdicts.size() - 1 );
        newIndex = dist( gen );
#endif
    } while( newIndex == lastVerdictIndex );

    verdictIndex = newIndex;
    lastVerdictIndex = newIndex;
}
