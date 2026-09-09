//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "services/powerManager.h"
#include "i18n.h"
#include "pins.h"

#include <utility>

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>

PowerManager& PowerManager::instance()
{
    static PowerManager inst;
    return inst;
}

PowerManager::PowerManager() = default;

void PowerManager::init( PowerSense& powerSense, DisplayManager& display, Buzzer& buzzer, NetworkWorker& netWorker )
{
    this->powerSense = &powerSense;
    this->display = &display;
    this->buzzer = &buzzer;
    this->netWorker = &netWorker;
    this->lastActivityMs = 0;
}

void PowerManager::registerSleepGate( CanSleepCallback gate )
{
    if( gate ) {
        sleepGates.push_back( std::move( gate ) );
    }
}

void PowerManager::clearSleepGates()
{
    sleepGates.clear();
}

void PowerManager::update( const uint32_t nowMs )
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();

    if( powerSense != nullptr && powerSense->wasPowerSourceChanged() ) {
        if( powerSense->isUsbPowered() ) {
            if( buzzer != nullptr ) {
                buzzer->beep( 30, currentMs );
            }
            if( display != nullptr ) {
                display->showToast( I18n::instance().isItalian() ? "ALIMENTAZ. USB" : "USB POWER", 2000, currentMs );
            }
            if( netWorker != nullptr ) {
                netWorker->setPowerProfile( PowerProfile::Usb );
            }
        } else {
            if( buzzer != nullptr ) {
                buzzer->doubleBeep( 50, 50, currentMs );
            }
            if( display != nullptr ) {
                display->showToast( I18n::instance().isItalian() ? "MODO BATTERIA" : "BATTERY MODE", 2000, currentMs );
            }
            if( netWorker != nullptr ) {
                netWorker->setPowerProfile( PowerProfile::Battery );
            }
        }

        resetActivity( currentMs );
    }
}

void PowerManager::resetActivity( const uint32_t nowMs )
{
    lastActivityMs = ( nowMs != 0 ) ? nowMs : millis();
}

bool PowerManager::canEnterSleep() const
{
    for( const auto& gate : sleepGates ) {
        if( gate && !gate() ) {
            return false;
        }
    }
    return true;
}

bool PowerManager::shouldEnterSleep( const uint32_t nowMs ) const
{
    const uint32_t currentMs = ( nowMs != 0 ) ? nowMs : millis();

    if( currentMs - lastActivityMs < SLEEP_INACTIVITY_MS ) {
        return false;
    }

    return canEnterSleep();
}

bool PowerManager::shouldSyncWifiOnWake( const uint32_t nowEpoch, const uint32_t lastSyncEpoch ) const
{
    if( lastSyncEpoch == 0 ) {
        return true;
    }
    if( nowEpoch < lastSyncEpoch ) {
        return true;
    }

    return ( nowEpoch - lastSyncEpoch >= WIFI_SYNC_INTERVAL_SEC );
}

void PowerManager::enterDeepSleep( MotionSensor& motion )
{
    if( display != nullptr ) {
        display->setPower( false );
    }

    motion.enableWakeOnMotion( 20 );

    gpio_pullup_en( static_cast<gpio_num_t>(PIN_USB_DETECT) );
    gpio_pulldown_dis( static_cast<gpio_num_t>(PIN_USB_DETECT) );

    esp_sleep_enable_ext0_wakeup( static_cast<gpio_num_t>(PIN_BUTTON_ACTION), 0 );
    esp_sleep_enable_ext1_wakeup( ( 1ULL << PIN_MPU_INT ) | ( 1ULL << PIN_USB_DETECT ), ESP_EXT1_WAKEUP_ANY_HIGH );

    WiFi.mode( WIFI_OFF );

    esp_deep_sleep_start();
}
