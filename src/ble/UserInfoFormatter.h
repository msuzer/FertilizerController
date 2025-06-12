#pragma once

#include <Arduino.h>
#include "core/SystemContext.h"
#include "core/SystemPreferences.h"
#include "control/DispenserChannel.h"

class UserInfoFormatter
{
public:
    static constexpr const char* PACKET_VERSION = "v1|";

    // Data structs
    struct DeviceInfoData {
        static constexpr const char* PREFIX = "dev";

        const char* bleName;
        const char* devUUID;
        const char* dsUUID;
        const char* bleMAC;
        const char* fwVer;
        const char* devVer;
    };

    struct GPSInfoData {
        static constexpr const char* PREFIX = "gps";

        const char* spdSrc;
        float minSpd;
        float simSpd;
        float gpsSpd;
        float lat;
        float lng;
        int sats;
    };

    struct PIInfoData {
        static constexpr const char* PREFIX = "pi";

        float piKp;
        float piKi;
    };

    struct TaskChannelInfoData {
        static constexpr const char* PREFIX_LEFT = "lft";
        static constexpr const char* PREFIX_RIGHT = "rgt";

        float flowDaaSet;
        float flowMinSet;
        float flowDaaReal;
        float flowMinReal;
        int tankLevel;
        float areaDone;
        int duration;
        float consumed;
    };

    // Public API
    static String makeTaskInfoPacket(const TaskChannelInfoData& left, const TaskChannelInfoData& right);
    static String makeDeviceInfoPacket(const DeviceInfoData& data);
    static String makeGPSInfoPacket(const GPSInfoData& data);
    static String makePIPacket(const PIInfoData& data);

private:
    static String formatFloat(float value);

    template<typename T>
    static String valueToString(const T& value);

    template<typename... Args>
    static String makeChannelData(const String& prefix, Args... args);
};
