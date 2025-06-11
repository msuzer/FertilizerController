// SystemPreferences.h
#pragma once
#include <Preferences.h>

class SystemContext; // Forward declaration

// Default values (copied from UserInterface.h)
#define DEFAULT_TARGET_RATE_KG_DAA      20.0f
#define DEFAULT_TARGET_FLOW_PER_MIN     15.0f
#define DEFAULT_FLOW_COEFF              1.0f
#define DEFAULT_MIN_WORKING_SPEED       1.0f
#define DEFAULT_AUTO_REFRESH_PERIOD     4
#define DEFAULT_HEARTBEAT_PERIOD        25
#define DEFAULT_SPEED_SOURCE            "GPS"
#define DEFAULT_TANK_INITIAL_LEVEL      1000.0f
#define DEFAULT_SIM_SPEED               1.0f
#define DEFAULT_KP_VALUE                25.0f
#define DEFAULT_KI_VALUE                4.0f

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
        KEY_RIGHT_RATE_DAA,
        KEY_RIGHT_RATE_MIN,
        KEY_RIGHT_FLOW_COEFF,
        KEY_PI_KP,
        KEY_PI_KI,
        KEY_COUNT
    };

struct SystemParams {
    String speedSource;
    float simSpeed;
    float minWorkingSpeed;
    int autoRefreshPeriod;
    int heartBeatPeriod;
};

class SystemPreferences {
public:
    static SystemPreferences& getInstance();
    SystemPreferences() = default;
    SystemPreferences(const SystemPreferences&) = delete;
    SystemPreferences& operator=(const SystemPreferences&) = delete;

    const SystemParams& getParams() const { return params; }
    SystemParams& getParams() { return params; }
    void setParams(const SystemParams& p) { params = p; }

    void init(SystemContext& ctx);

    int getInt(PrefKey key, int defaultValue);
    float getFloat(PrefKey key, float defaultValue);
    String getString(PrefKey key, const String& defaultValue);
    void save(PrefKey key, const String& value);
    void save(PrefKey key, const int value);
    void save(PrefKey key, const float value);

    static const char* getKeyName(PrefKey key);
private:
    SystemParams params;
    static constexpr const char* storageNamespace = "UIData";
    static const char* keyNames[KEY_COUNT];
};