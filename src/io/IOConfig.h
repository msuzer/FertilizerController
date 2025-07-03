// ============================================
// File: IOConfig.h
// Purpose: Define pin configurations for various hardware components
// Part of: Hardware Abstraction Layer (HAL)
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

// Motor Driver 1 Pins
constexpr int VNH7070AS_INA1Pin = 25;
constexpr int VNH7070AS_INB1Pin = 14;
constexpr int VNH7070AS_PWM1Pin = 27;
constexpr int VNH7070AS_SEL1Pin = 26;

// Motor Driver 2 Pins
constexpr int VNH7070AS_INA2Pin = 19;
constexpr int VNH7070AS_INB2Pin = 16;
constexpr int VNH7070AS_PWM2Pin = 17;
constexpr int VNH7070AS_SEL2Pin = 18;

// GPS UART Pins
constexpr int GPS_UART_RX_PIN = 13;
constexpr int GPS_UART_TX_PIN = 21;

// RGB LED Pins
constexpr int RGB_LEDRPin = 15;
constexpr int RGB_LEDGPin = 2;
constexpr int RGB_LEDBPin = 4;

// DS18B20 OneWire Pin
constexpr int DS18B20_DataPin = 22;

// I2C Pins
constexpr int I2C_SDAPin = 32;
constexpr int I2C_SCLPin = 33;

constexpr bool isCommonAnode = true; // Set to true if using common anode RGB LED