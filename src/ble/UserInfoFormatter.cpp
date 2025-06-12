// ============================================
// File: UserInfoFormatter.cpp
// Purpose: Formats system/user info into BLE-friendly packets
// Part of: BLE Layer / Communication
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "UserInfoFormatter.h"
#include "core/DebugInfoPrinter.h"
#include "core/version.h"

uint32_t UserInfoFormatter::packetCounter = 0;

// --- Static helpers ---

String UserInfoFormatter::formatFloat(float value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", value);
    return String(buf);
}

template<typename T>
String UserInfoFormatter::valueToString(const T& value) {
    return String(value);
}

template<>
String UserInfoFormatter::valueToString<float>(const float& value) {
    return formatFloat(value);
}

template<typename... Args>
String UserInfoFormatter::makeChannelData(const String& prefix, Args... args) {
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

String UserInfoFormatter::makePktIdField() {
    return "pktId[" + String(packetCounter++) + "]";
}

// --- Public API ---
String UserInfoFormatter::makeVersionInfoPacket() {
    return String(PACKET_VERSION) + "ver[" +
           FIRMWARE_VERSION + "|" +
           DEVICE_VERSION + "|" +
           BUILD_DATE + "|" +
           BUILD_TIME + "]" +
           makePktIdField();
}

String UserInfoFormatter::makeTaskInfoPacket(const TaskChannelInfoData& left, const TaskChannelInfoData& right) {
    String leftPart = makeChannelData(TaskChannelInfoData::PREFIX_LEFT,
        left.flowDaaSet, left.flowMinSet, left.flowDaaReal, left.flowMinReal,
        left.tankLevel, left.areaDone, left.duration, left.consumed);

    String rightPart = makeChannelData(TaskChannelInfoData::PREFIX_RIGHT,
        right.flowDaaSet, right.flowMinSet, right.flowDaaReal, right.flowMinReal,
        right.tankLevel, right.areaDone, right.duration, right.consumed);

    String packet = String(PACKET_VERSION) + leftPart + rightPart + makePktIdField();
    return packet;
}

String UserInfoFormatter::makeDeviceInfoPacket(const DeviceInfoData& data) {
    String packet = String(PACKET_VERSION) + makeChannelData(DeviceInfoData::PREFIX,
        data.bleName, data.devUUID, data.dsUUID, data.bleMAC) + makePktIdField();

    return packet;
}

String UserInfoFormatter::makeGPSInfoPacket(const GPSInfoData& data) {
    String packet = String(PACKET_VERSION) + makeChannelData(GPSInfoData::PREFIX,
        data.spdSrc, data.minSpd, data.simSpd, data.gpsSpd, data.lat, data.lng, data.sats)
        + makePktIdField();

    return packet;
}

String UserInfoFormatter::makePIPacket(const PIInfoData& data) {
    String packet = String(PACKET_VERSION) + makeChannelData(PIInfoData::PREFIX,
        data.piKp, data.piKi) + makePktIdField();

    return packet;
}

String UserInfoFormatter::makeErrorInfoPacket(uint32_t errorFlags, bool verbose) {
    String packet = String(PACKET_VERSION) + "err[0x" + String(errorFlags, HEX);

    if (verbose) {
        String errorStr = DebugInfoPrinter::formatErrorFlags(errorFlags);
        packet += "|" + errorStr;
    }

    packet += "]" + makePktIdField();

    return packet;
}
