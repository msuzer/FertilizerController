#pragma once

#include <cstdint>

// Flow-related constant
constexpr float FLOW_ERROR_WARNING_THRESHOLD = 2.0f;

// Error code bitmask
enum UserErrorCodes {
    NO_ERROR                = 0,
    LIQUID_TANK_EMPTY       = 1 << 0,
    INSUFFICIENT_FLOW       = 1 << 1,
    FLOW_NOT_SETTLED        = 1 << 2,
    MOTOR_STUCK             = 1 << 3,
    DUMMY_ERROR             = 1 << 4,
    BATTERY_LOW             = 1 << 5,
    NO_SATELLITE_CONNECTED  = 1 << 6,
    INVALID_SATELLITE_INFO  = 1 << 7,
    INVALID_GPS_LOCATION    = 1 << 8,
    INVALID_GPS_SPEED       = 1 << 9,
    INVALID_PARAM_COUNT     = 1 << 10,
    MESSAGE_PARSE_ERROR     = 1 << 11,
    HARDWARE_ERROR          = 1 << 12
};

class ErrorManager {
private:
    uint32_t errorFlags = NO_ERROR;

public:
    // Accessors
    uint32_t getErrorFlags() const { return errorFlags; }
    bool hasError(uint32_t mask) const { return (errorFlags & mask) != 0; }
    bool hasAnyError() const { return errorFlags != 0; }

    // Mutators
    void setErrorFlags(uint32_t flags) { errorFlags = flags; }
    void setError(uint32_t mask) { errorFlags |= mask; }
    void clearError(uint32_t mask) { errorFlags &= ~mask; }
    void clearAllErrors() { errorFlags = 0; }
};
