#pragma once
#include <Wire.h>
#include "ADS1115Pins.h"

enum ADS1115Channels {
    CH0 = 0,
    CH1,
    CH2,
    CH3
};

class ADS1115 {
public:
    enum class Gain {
        FSR_6_144V = 0,
        FSR_4_096V,
        FSR_2_048V,
        FSR_1_024V,
        FSR_0_512V,
        FSR_0_256V
    };

    enum class DataRate {
        SPS_8 = 0,
        SPS_16,
        SPS_32,
        SPS_64,
        SPS_128,
        SPS_250,
        SPS_475,
        SPS_860
    };

    ADS1115(TwoWire& wire = Wire);

    bool init(const uint8_t i2c_address = 0x48, const ADS1115Pins & pins = {-1, -1});
    void setGain(Gain gain);
    void setDataRate(DataRate rate);

    void pushBuffer(uint8_t channel);

    int16_t readSingleEnded(uint8_t channel);
    int16_t readDifferential(uint8_t channel1, uint8_t channel2);
    int16_t readFiltered(uint8_t channel);
    float readFilteredVoltage(uint8_t channel);
    float readFilteredCurrent(uint8_t channel);

    float readVoltageSingleEnded(uint8_t channel);
    float readVoltageDifferential(uint8_t channel1, uint8_t channel2);

    float rawToVoltage(int16_t raw) const;
    float rawToCurrent(int16_t raw) const;
    float mapRawToFloat(int16_t raw, float conversionFactor = 1.0f, int16_t rawMin = 0, int16_t rawMax = 32767) const;

    Gain getGain() const;
    float getFSR() const;

private:
    bool _configure(uint16_t mux);
    int16_t _readConversionRegister();
    uint16_t _buildConfig(uint16_t mux);

    TwoWire* _wire;
    uint8_t _i2cAddress;
    Gain _gain = Gain::FSR_2_048V;
    DataRate _dataRate = DataRate::SPS_128;
};
