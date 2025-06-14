// ============================================
// File: SystemPreferences.cpp
// Purpose: Manages persistent system preferences
// Part of: Core Services
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "SystemPreferences.h"
#include "control/DispenserChannel.h"
#include "core/SystemContext.h"
#include "core/LogUtils.h"

#define LOG_VERBOSE 1

const char* SystemPreferences::keyNames[KEY_COUNT] = {
    "speedSrc",
    "simSpeed",
    "minSpeed",
    "refresh",
    "heartbeat",
    "tankLevel",

    "left_rateDaa",
    "left_rateMin",
    "left_flowCoeff",
    "left_boomWidth",

    "right_rateDaa",
    "right_rateMin",
    "right_flowCoeff",
    "right_boomWidth",

    "piKp",
    "piKi",
    "logLevel",
};

const char* SystemPreferences::getKeyName(PrefKey key) {
    return keyNames[static_cast<int>(key)];
}

void SystemPreferences::init(SystemContext& ctx) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);

    int logLevel = prefs.getInt(keyNames[KEY_LOG_LEVEL], static_cast<int>(LogLevel::Info));
    if (logLevel > static_cast<int>(LogLevel::Info)) {
        logLevel = static_cast<int>(LogLevel::Info); // No verbose logging on boot up.
    }
    LogUtils::setLogLevel(static_cast<LogLevel>(logLevel));

    params.speedSource = prefs.getString(keyNames[KEY_SPEED_SRC], DEFAULT_SPEED_SOURCE);
    params.simSpeed = prefs.getFloat(keyNames[KEY_SIM_SPEED], DEFAULT_SIM_SPEED);
    params.minWorkingSpeed = prefs.getFloat(keyNames[KEY_MIN_SPEED], DEFAULT_MIN_WORKING_SPEED);
    params.autoRefreshPeriod = prefs.getInt(keyNames[KEY_REFRESH], DEFAULT_AUTO_REFRESH_PERIOD);
    params.heartBeatPeriod = prefs.getInt(keyNames[KEY_HEARTBEAT], DEFAULT_HEARTBEAT_PERIOD);
    DispenserChannel::setTankLevel(prefs.getFloat(keyNames[KEY_TANK_LEVEL], DEFAULT_TANK_INITIAL_LEVEL));

    auto& left = ctx.getLeftChannel();
    left.setTargetFlowRatePerDaa(prefs.getFloat(keyNames[KEY_LEFT_RATE_DAA], DEFAULT_TARGET_RATE_KG_DAA));
    left.setTargetFlowRatePerMin(prefs.getFloat(keyNames[KEY_LEFT_RATE_MIN], DEFAULT_TARGET_FLOW_PER_MIN));
    left.setFlowCoeff(prefs.getFloat(keyNames[KEY_LEFT_FLOW_COEFF], DEFAULT_FLOW_COEFF));
    left.setBoomWidth(prefs.getFloat(keyNames[KEY_LEFT_BOOM_WIDTH], DEFAULT_LEFT_BOOM_WIDTH));

    auto& right = ctx.getRightChannel();
    right.setTargetFlowRatePerDaa(prefs.getFloat(keyNames[KEY_RIGHT_RATE_DAA], DEFAULT_TARGET_RATE_KG_DAA));
    right.setTargetFlowRatePerMin(prefs.getFloat(keyNames[KEY_RIGHT_RATE_MIN], DEFAULT_TARGET_FLOW_PER_MIN));
    right.setFlowCoeff(prefs.getFloat(keyNames[KEY_RIGHT_FLOW_COEFF], DEFAULT_FLOW_COEFF));
    right.setBoomWidth(prefs.getFloat(keyNames[KEY_RIGHT_BOOM_WIDTH], DEFAULT_RIGHT_BOOM_WIDTH));

    float kp = prefs.getFloat(keyNames[KEY_PI_KP], DEFAULT_KP_VALUE);
    float ki = prefs.getFloat(keyNames[KEY_PI_KI], DEFAULT_KI_VALUE);
    ctx.getLeftChannel().setPIParams(kp, ki);
    ctx.getRightChannel().setPIParams(kp, ki);

    prefs.end();
}

bool SystemPreferences::getBool(PrefKey key, bool defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    bool val = prefs.getBool(getKeyName(key), defaultValue);
    prefs.end();
    
    LogUtils::verbose("[PREF] %s = %s (default %s)\n",
            getKeyName(key),
            val ? "true" : "false",
            defaultValue ? "true" : "false");
    return val;
}

int SystemPreferences::getInt(PrefKey key, int defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    int val = prefs.getInt(getKeyName(key), defaultValue);
    prefs.end();

    LogUtils::verbose("[PREF] %s = %d (default %d)\n", getKeyName(key), val, defaultValue);
    return val;
}

float SystemPreferences::getFloat(PrefKey key, float defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    float val = prefs.getFloat(getKeyName(key), defaultValue);
    prefs.end();

    LogUtils::verbose("[PREF] %s = %.2f (default %.2f)\n", getKeyName(key), val, defaultValue);
    return val;
}

String SystemPreferences::getString(PrefKey key, const String& defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    String val = prefs.getString(getKeyName(key), defaultValue);
    prefs.end();

    LogUtils::verbose("[PREF] %s = %s (default %s)\n", getKeyName(key), val.c_str(), defaultValue.c_str());
    return val;
}

void SystemPreferences::save(PrefKey key, const String& value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    String oldValue = prefs.getString(name, "");
    bool valueExists = prefs.isKey(name);
    bool valueChanged = (oldValue != value);
    if (!valueExists || valueChanged) {
        prefs.putString(name, value);
        LogUtils::verbose("[PREF] %s <- \"%s\" (was \"%s\")\n", name, value.c_str(), oldValue.c_str());
    } else {
        LogUtils::verbose("[PREF] %s unchanged (still \"%s\")\n", name, value.c_str());
    }
    prefs.end();
}

void SystemPreferences::save(PrefKey key, const int value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    int oldValue = prefs.getInt(name);
    bool valueExists = prefs.isKey(name);
    bool valueChanged = (oldValue != value);

    if (!valueExists || valueChanged) {
        prefs.putInt(name, value);
        LogUtils::verbose("[PREF] %s <- \"%d\" (was \"%d\")\n", name, value, oldValue);
    } else {
        LogUtils::verbose("[PREF] %s unchanged (still \"%d\")\n", name, value);
    }

    prefs.end();
}

void SystemPreferences::save(PrefKey key, const float value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    float oldValue = prefs.getFloat(name, 0.0f);
    bool valueExists = prefs.isKey(name);
    bool valueChanged = (oldValue != value);
    
    if (!valueExists || valueChanged) {
        prefs.putFloat(name, value);
        LogUtils::verbose("[PREF] %s <- \"%.2f\" (was \"%.2f\")\n", name, value, oldValue);
    } else {
        LogUtils::verbose("[PREF] %s unchanged (still \"%.2f\")\n", name, value);
    }
    prefs.end();
}
