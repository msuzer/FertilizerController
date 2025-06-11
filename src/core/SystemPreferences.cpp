// SystemPreferences.cpp
#include "SystemPreferences.h"
#include "control/DispenserChannel.h"
#include "core/SystemContext.h"

SystemPreferences& SystemPreferences::getInstance() {
    static SystemPreferences instance;
    return instance;
}

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

    "right_rateDaa",
    "right_rateMin",
    "right_flowCoeff",

    "piKp"
    "piKi"
};

const char* SystemPreferences::getKeyName(PrefKey key) {
    return keyNames[static_cast<int>(key)];
}

void SystemPreferences::init(SystemContext& ctx) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);

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

    auto& right = ctx.getRightChannel();
    right.setTargetFlowRatePerDaa(prefs.getFloat(keyNames[KEY_RIGHT_RATE_DAA], DEFAULT_TARGET_RATE_KG_DAA));
    right.setTargetFlowRatePerMin(prefs.getFloat(keyNames[KEY_RIGHT_RATE_MIN], DEFAULT_TARGET_FLOW_PER_MIN));
    right.setFlowCoeff(prefs.getFloat(keyNames[KEY_RIGHT_FLOW_COEFF], DEFAULT_FLOW_COEFF));

    float kp = prefs.getFloat(keyNames[KEY_PI_KP], DEFAULT_KP_VALUE);
    float ki = prefs.getFloat(keyNames[KEY_PI_KI], DEFAULT_KI_VALUE);
    ctx.getLeftChannel().setPIParams(kp, ki);
    ctx.getRightChannel().setPIParams(kp, ki);

    prefs.end();
}

int SystemPreferences::getInt(PrefKey key, int defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    int val = prefs.getInt(getKeyName(key), defaultValue);
    prefs.end();
    return val;
}

float SystemPreferences::getFloat(PrefKey key, float defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    float val = prefs.getFloat(getKeyName(key), defaultValue);
    prefs.end();
    return val;
}

String SystemPreferences::getString(PrefKey key, const String& defaultValue) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);
    String val = prefs.getString(getKeyName(key), defaultValue);
    prefs.end();
    return val;
}

void SystemPreferences::save(PrefKey key, const String& value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    if (prefs.getString(name, "") != value) {
        prefs.putString(name, value);
    }
    prefs.end();
}

void SystemPreferences::save(PrefKey key, const int value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    if (!prefs.isKey(name) || prefs.getInt(name) != value) {
        prefs.putInt(name, value);
    }
    prefs.end();
}

void SystemPreferences::save(PrefKey key, const float value) {
    Preferences prefs;
    prefs.begin(storageNamespace);
    const char* name = getKeyName(key);
    if (prefs.getFloat(name, 0.0f) != value) {
        prefs.putFloat(name, value);
    }
    prefs.end();
}
