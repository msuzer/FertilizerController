#include "ADS1115.h"

// Your resistor values for current measurement
const float Rtop = 4700.0f;   // 4.7k
const float Rbottom = 1000.0f; // 1k

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

ADS1115::ADS1115(TwoWire& wire) : _wire(&wire) {}

bool ADS1115::begin(uint8_t i2c_address) {
    _i2cAddress = i2c_address;
    _wire->begin();
    return true;
}

void ADS1115::setGain(Gain gain) {
    _gain = gain;
}

void ADS1115::setDataRate(DataRate rate) {
    _dataRate = rate;
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
  const float csSensitivity = 0.010f;
  const float dividerFactor = (Rtop + Rbottom) / Rbottom; // 5.7
  float csVoltage = rawToVoltage(raw) * dividerFactor;
  return csVoltage / csSensitivity;
}
