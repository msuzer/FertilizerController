// ============================================
// File: SystemPreferences.h
// Purpose: Manages persistent system preferences
// Part of: Core Services
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once
#include <Preferences.h>

class SystemContext; // Forward declaration

// Default configuration values
namespace {
    constexpr float DEFAULT_TARGET_RATE_KG_DAA    = 20.0f;
    constexpr float DEFAULT_TARGET_FLOW_PER_MIN   = 15.0f;
    constexpr float DEFAULT_FLOW_COEFF            = 1.0f;
    constexpr float DEFAULT_LEFT_BOOM_WIDTH       = 0.0f;
    constexpr float DEFAULT_RIGHT_BOOM_WIDTH      = 0.0f;
    constexpr float DEFAULT_MIN_WORKING_SPEED     = 1.0f;
    constexpr int   DEFAULT_AUTO_REFRESH_PERIOD   = 4;
    constexpr int   DEFAULT_HEARTBEAT_PERIOD      = 25;
    constexpr char  DEFAULT_SPEED_SOURCE[]        = "GPS";
    constexpr float DEFAULT_TANK_INITIAL_LEVEL    = 1000.0f;
    constexpr float DEFAULT_SIM_SPEED             = 1.0f;
    constexpr float DEFAULT_KP_VALUE              = 25.0f;
    constexpr float DEFAULT_KI_VALUE              = 4.0f;
}

enum PrefKey {
    KEY_SPEED_SRC,
    KEY_SIM_SPEED,
    KEY_MIN_SPEED,
    KEY_REFRESH,
    KEY_HEARTBEAT,
    KEY_TANK_LEVEL,
    KEY_LEFT_RATE_DAA,
    KEY_LEFT_RATE_MIN,
    KEY_LEFT_FLOW_COEFF,
    KEY_LEFT_BOOM_WIDTH,
    KEY_RIGHT_RATE_DAA,
    KEY_RIGHT_RATE_MIN,
    KEY_RIGHT_FLOW_COEFF,
    KEY_RIGHT_BOOM_WIDTH,
    KEY_PI_KP,
    KEY_PI_KI,
    KEY_LOG_LEVEL,
    KEY_COUNT
};

class SystemPreferences {
public:
    static void init(SystemContext& ctx);

    static bool getBool(PrefKey key, bool defaultValue);
    static int getInt(PrefKey key, int defaultValue);
    static float getFloat(PrefKey key, float defaultValue);
    static String getString(PrefKey key, const String& defaultValue);
    static void save(PrefKey key, const String& value);
    static void save(PrefKey key, const int value);
    static void save(PrefKey key, const float value);

    static const char* getKeyName(PrefKey key);
private:
    SystemPreferences() = default;
    static constexpr const char* storageNamespace = "UIData";
    static const char* keyNames[KEY_COUNT];
};
