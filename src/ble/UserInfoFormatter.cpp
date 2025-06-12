#include "UserInfoFormatter.h"

// --- Static helpers ---

String UserInfoFormatter::formatFloat(float value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", value);
    return String(buf);
}

template<typename T>
String UserInfoFormatter::valueToString(const T& value)
{
    return String(value);
}

template<>
String UserInfoFormatter::valueToString<float>(const float& value)
{
    return formatFloat(value);
}

template<typename... Args>
String UserInfoFormatter::makeChannelData(const String& prefix, Args... args)
{
    String result = prefix + "[";

    int dummy[] = {0, ((result += valueToString(args) + "|"), 0)...};
    (void)dummy;

    if (sizeof...(args) > 0)
    {
        result.remove(result.length() - 1); // remove last '|'
    }

    result += "]";
    return result;
}

// --- Public API ---

String UserInfoFormatter::makeTaskInfoPacket(const TaskChannelInfoData& left, const TaskChannelInfoData& right)
{
    String leftPart = makeChannelData(TaskChannelInfoData::PREFIX_LEFT,
        left.flowDaaSet, left.flowMinSet, left.flowDaaReal, left.flowMinReal,
        left.tankLevel, left.areaDone, left.duration, left.consumed);

    String rightPart = makeChannelData(TaskChannelInfoData::PREFIX_RIGHT,
        right.flowDaaSet, right.flowMinSet, right.flowDaaReal, right.flowMinReal,
        right.tankLevel, right.areaDone, right.duration, right.consumed);

    String packet = String(PACKET_VERSION) + leftPart + rightPart;
    return packet;
}

String UserInfoFormatter::makeDeviceInfoPacket(const DeviceInfoData& data)
{
    String packet = String(PACKET_VERSION) + makeChannelData(DeviceInfoData::PREFIX,
        data.bleName, data.devUUID, data.dsUUID, data.bleMAC, data.fwVer, data.devVer);

    return packet;
}

String UserInfoFormatter::makeGPSInfoPacket(const GPSInfoData& data)
{
    String packet = String(PACKET_VERSION) + makeChannelData(GPSInfoData::PREFIX,
        data.spdSrc, data.minSpd, data.simSpd, data.gpsSpd, data.lat, data.lng, data.sats);

    return packet;
}

String UserInfoFormatter::makePIPacket(const PIInfoData& data)
{
    String packet = String(PACKET_VERSION) + makeChannelData(PIInfoData::PREFIX,
        data.piKp, data.piKi);

    return packet;
}
