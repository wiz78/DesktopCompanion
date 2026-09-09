//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_PINS_H
#define FRIENDLYBOT_PINS_H

#include <cstdint>

// I2C Bus (Shared across OLED, MPU6050, VL53L0X)
static constexpr uint8_t PIN_I2C_SDA = 8;
static constexpr uint8_t PIN_I2C_SCL = 9;

// Motion Sensor Interrupt (MPU6050 Wake-On-Motion / Ext Wakeup)
static constexpr uint8_t PIN_MPU_INT = 6;

// Analog Inputs (ADC1 channels)
static constexpr uint8_t PIN_POT_LEFT = 1;     // ADC1_CH0 - Left Eye (Kitchen Timer)
static constexpr uint8_t PIN_POT_RIGHT = 2;    // ADC1_CH1 - Right Eye (Arbitrator)
static constexpr uint8_t PIN_LDR = 3;          // ADC1_CH2 - 5528 Light Sensor

// Power Detection
static constexpr uint8_t PIN_USB_DETECT = 4;   // Dual-Pole Switch / Power Sense (HIGH = USB, LOW = Battery)

// Digital Inputs (Tactile switches, active LOW)
static constexpr uint8_t PIN_BUTTON_ACTION = 10; // Front Main Action Button
static constexpr uint8_t PIN_BUTTON_RESET = 12;  // Hidden Back Factory Reset Button

// Digital Outputs
static constexpr uint8_t PIN_BUZZER = 5;       // Active Buzzer (Cooking Timer Alarm)

#endif // FRIENDLYBOT_PINS_H
