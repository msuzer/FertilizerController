// ============================================
// File: ADS1115.cpp
// Purpose: Driver for ADS1115 ADC with filtering support
// Part of: Hardware Abstraction Layer (HAL)
// Dependencies: Wire, ADS1115
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "ADS1115.h"
#include "io/CircularBuffer.h"

// --- Allocate fixed-size buffers for each ADS1115 channel ---
constexpr size_t ADS1115_BUF_SIZE = 8;

static int16_t buffer0[ADS1115_BUF_SIZE];
static int16_t buffer1[ADS1115_BUF_SIZE];
static int16_t buffer2[ADS1115_BUF_SIZE];
static int16_t buffer3[ADS1115_BUF_SIZE];

// --- Create CircularBuffer instances for each channel ---
static CircularBuffer ch0(buffer0, ADS1115_BUF_SIZE);
static CircularBuffer ch1(buffer1, ADS1115_BUF_SIZE);
static CircularBuffer ch2(buffer2, ADS1115_BUF_SIZE);
static CircularBuffer ch3(buffer3, ADS1115_BUF_SIZE);

// ADS1115 Register Addresses
constexpr uint8_t ADS1115_REG_CONVERSION = 0x00;
constexpr uint8_t ADS1115_REG_CONFIG     = 0x01;

// Config Register Bit Fields (Masks & Shifts)
constexpr uint16_t ADS1115_OS_SINGLE     = 0x8000;
constexpr uint16_t ADS1115_MODE_SINGLE   = 0x0100;
constexpr uint16_t ADS1115_MODE_CONT     = 0x0000;

// PGA Settings
constexpr uint16_t ADS1115_PGA_TABLE[] = {
    0x0000, 0x0200, 0x0400, 0x0600, 0x0800, 0x0A00
};

constexpr float ADS1115_FSR_TABLE[] = {
    6.144f, 4.096f, 2.048f, 1.024f, 0.512f, 0.256f
};

// Data Rate Settings
constexpr uint16_t ADS1115_DR_TABLE[] = {
    0x0000, 0x0020, 0x0040, 0x0060, 0x0080, 0x00A0, 0x00C0, 0x00E0
};

bool ADS1115::init(const uint8_t i2c_address, const ADS1115Pins & pins) {
    _i2cAddress = i2c_address;
    return _wire->begin(pins.SDA, pins.SCL);
}

void ADS1115::setGain(Gain gain) {
    _gain = gain;
}

void ADS1115::setDataRate(DataRate rate) {
    _dataRate = rate;
}

void ADS1115::pushBuffer() {
    pushBuffer(ADS1115Channels::CH0); // Channel 0
    pushBuffer(ADS1115Channels::CH1); // Channel 1
    pushBuffer(ADS1115Channels::CH2); // Channel 2
    pushBuffer(ADS1115Channels::CH3); // Channel 3
}

void ADS1115::pushBuffer(uint8_t channel) {
    int16_t raw = readSingleEnded(channel);
    if (channel == 0) {
        ch0.push(raw);
    } else if (channel == 1) {
        ch1.push(raw);
    } else if (channel == 2) {
        ch2.push(raw);
    } else if (channel == 3) {
        ch3.push(raw);
    }
}

ADS1115::Gain ADS1115::getGain() const {
    return _gain;
}

float ADS1115::getFSR() const {
    return ADS1115_FSR_TABLE[static_cast<uint8_t>(_gain)];
}

bool ADS1115::_configure(uint16_t mux) {
    uint16_t config = _buildConfig(mux);
    _wire->beginTransmission(_i2cAddress);
    _wire->write(ADS1115_REG_CONFIG);
    _wire->write(config >> 8);
    _wire->write(config & 0xFF);
    return (_wire->endTransmission() == 0);
}

uint16_t ADS1115::_buildConfig(uint16_t mux) {
    uint16_t config = ADS1115_OS_SINGLE | mux;
    config |= ADS1115_PGA_TABLE[static_cast<uint8_t>(_gain)];
    config |= ADS1115_DR_TABLE[static_cast<uint8_t>(_dataRate)];
    config |= ADS1115_MODE_SINGLE;
    config |= 0x0003; // Disable comparator
    return config;
}

int16_t ADS1115::_readConversionRegister() {
    _wire->beginTransmission(_i2cAddress);
    _wire->write(ADS1115_REG_CONVERSION);
    if (_wire->endTransmission() != 0) return INT16_MIN;

    _wire->requestFrom(_i2cAddress, (uint8_t)2);
    if (_wire->available() < 2) return INT16_MIN;

    uint16_t result = (_wire->read() << 8) | _wire->read();
    return (int16_t)result;
}

int16_t ADS1115::readSingleEnded(uint8_t channel) {
    if (channel > 3) return INT16_MIN;
    if (!_configure(0x4000 | (channel << 12))) return INT16_MIN;
    delay(10); // Simple conversion delay
    return _readConversionRegister();
}

int16_t ADS1115::readDifferential(uint8_t channel1, uint8_t channel2) {
    uint16_t mux = 0;
    if ((channel1 == 0 && channel2 == 1)) mux = 0x0000;
    else if ((channel1 == 0 && channel2 == 3)) mux = 0x1000;
    else if ((channel1 == 1 && channel2 == 3)) mux = 0x2000;
    else if ((channel1 == 2 && channel2 == 3)) mux = 0x3000;
    else return INT16_MIN;

    if (!_configure(mux)) return INT16_MIN;
    delay(10);
    return _readConversionRegister();
}

int16_t ADS1115::readFiltered(uint8_t channel) {
    return (channel == 0) ? ch0.average() :
           (channel == 1) ? ch1.average() :
           (channel == 2) ? ch2.average() :
           (channel == 3) ? ch3.average() : INT16_MIN;
}

float ADS1115::readFilteredVoltage(uint8_t channel) {
    return rawToVoltage(readFiltered(channel));
}

float ADS1115::readFilteredCurrent(uint8_t channel) {
    return rawToCurrent(readFiltered(channel));
}

float ADS1115::readVoltageSingleEnded(uint8_t channel) {
    int16_t raw = readSingleEnded(channel);
    return (raw == INT16_MIN) ? NAN : raw * getFSR() / 32768.0f;
}

float ADS1115::readVoltageDifferential(uint8_t channel1, uint8_t channel2) {
    int16_t raw = readDifferential(channel1, channel2);
    return (raw == INT16_MIN) ? NAN : raw * getFSR() / 32768.0f;
}

float ADS1115::rawToVoltage(int16_t raw) const {
    // ADS1115 uses 16-bit signed integer: -32768 to +32767
    return static_cast<float>(raw) * getFSR() / 32768.0f;
}

float ADS1115::mapRawToFloat(int16_t raw, float conversionFactor, int16_t rawMin, int16_t rawMax) const {
    if (rawMax == rawMin) return 0.0f; // avoid division by zero

    // Clamp raw to defined range
    if (raw < rawMin) raw = rawMin;
    if (raw > rawMax) raw = rawMax;

    float percent = static_cast<float>(raw - rawMin) / (rawMax - rawMin);  // 0.0 to 1.0
    return percent * conversionFactor;
}

float ADS1115::rawToCurrent(int16_t raw) const {
  const float kFactor = 0.0014f;
  const float Resistor = 10000.0f;   // 10.0k

  const float dividerFactor = Resistor * kFactor;
  float csVoltage = rawToVoltage(raw) / dividerFactor;
  return csVoltage;
}
