// SystemPreferences.cpp
#include "SystemPreferences.h"
#include "PIController.h"

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

void SystemPreferences::load(SystemContext& ctx) {
    Preferences prefs;
    prefs.begin(storageNamespace, true);

    ctx.setSpeedSource(prefs.getString(keyNames[KEY_SPEED_SRC], DEFAULT_SPEED_SOURCE));
    ctx.setSimSpeed(prefs.getFloat(keyNames[KEY_SIM_SPEED], DEFAULT_SIM_SPEED));
    ctx.setMinWorkingSpeed(prefs.getFloat(keyNames[KEY_MIN_SPEED], DEFAULT_MIN_WORKING_SPEED));
    ctx.setAutoRefreshPeriod(prefs.getInt(keyNames[KEY_REFRESH], DEFAULT_AUTO_REFRESH_PERIOD));
    ctx.setHeartBeatPeriod(prefs.getInt(keyNames[KEY_HEARTBEAT], DEFAULT_HEARTBEAT_PERIOD));
    ctx.setTankLevel(prefs.getFloat(keyNames[KEY_TANK_LEVEL], DEFAULT_TANK_INITIAL_LEVEL));

    auto& left = ctx.getLeftChannel();
    left.setTargetFlowRatePerDaa(prefs.getFloat(keyNames[KEY_LEFT_RATE_DAA], DEFAULT_TARGET_RATE_KG_DAA));
    left.setTargetFlowRatePerMin(prefs.getFloat(keyNames[KEY_LEFT_RATE_MIN], DEFAULT_TARGET_FLOW_PER_MIN));
    left.setFlowCoeff(prefs.getFloat(keyNames[KEY_LEFT_FLOW_COEFF], DEFAULT_FLOW_COEFF));

    auto& right = ctx.getRightChannel();
    right.setTargetFlowRatePerDaa(prefs.getFloat(keyNames[KEY_RIGHT_RATE_DAA], DEFAULT_TARGET_RATE_KG_DAA));
    right.setTargetFlowRatePerMin(prefs.getFloat(keyNames[KEY_RIGHT_RATE_MIN], DEFAULT_TARGET_FLOW_PER_MIN));
    right.setFlowCoeff(prefs.getFloat(keyNames[KEY_RIGHT_FLOW_COEFF], DEFAULT_FLOW_COEFF));

    services->pi1->setPIKp(prefs.getFloat(keyNames[KEY_PI_KP], DEFAULT_KP_VALUE));
    services->pi2->setPIKp(prefs.getFloat(keyNames[KEY_PI_KP], DEFAULT_KP_VALUE));

    services->pi1->setPIKi(prefs.getFloat(keyNames[KEY_PI_KI], DEFAULT_KI_VALUE));
    services->pi2->setPIKi(prefs.getFloat(keyNames[KEY_PI_KI], DEFAULT_KI_VALUE));

    prefs.end();
}

void SystemPreferences::save(const SystemContext& ctx) {
    Preferences prefs;
    prefs.begin(storageNamespace);

    prefs.putString(keyNames[KEY_SPEED_SRC], ctx.getSpeedSource());
    prefs.putFloat(keyNames[KEY_SIM_SPEED], ctx.getSimSpeed());
    prefs.putFloat(keyNames[KEY_MIN_SPEED], ctx.getMinWorkingSpeed());
    prefs.putInt(keyNames[KEY_REFRESH], ctx.getAutoRefreshPeriod());
    prefs.putInt(keyNames[KEY_HEARTBEAT], ctx.getHeartBeatPeriod());
    prefs.putFloat(keyNames[KEY_TANK_LEVEL], ctx.getTankLevel());

    const auto& left = ctx.getLeftChannel();
    prefs.putFloat(keyNames[KEY_LEFT_RATE_DAA], left.getTargetFlowRatePerDaa());
    prefs.putFloat(keyNames[KEY_LEFT_RATE_MIN], left.getTargetFlowRatePerMin());
    prefs.putFloat(keyNames[KEY_LEFT_FLOW_COEFF], left.getFlowCoeff());

    const auto& right = ctx.getRightChannel();
    prefs.putFloat(keyNames[KEY_RIGHT_RATE_DAA], right.getTargetFlowRatePerDaa());
    prefs.putFloat(keyNames[KEY_RIGHT_RATE_MIN], right.getTargetFlowRatePerMin());
    prefs.putFloat(keyNames[KEY_RIGHT_FLOW_COEFF], right.getFlowCoeff());

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
