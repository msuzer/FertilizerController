// ============================================
// File: DebugInfoPrinter.cpp
// Purpose: Provides centralized system debug printing functions
// Part of: Core Services
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "DebugInfoPrinter.h"
#include "version.h"
#include <cstdio>
#include "core/SystemContext.h"
#include "core/LogUtils.h"

void DebugInfoPrinter::printAll(SystemContext& context) {
    LogUtils::info("========== SYSTEM DEBUG info ==========\n");

    // System LogUtils::info (Tank, WorkZone, Config Params)
    printSystemInfo(context);

    // Error summary
    printErrorSummary(context);

    // Real-time data (flow, PI controller, errors, GPS)
    printRealTimeData(context);

    // Detailed GPS LogUtils::info (TinyGPSPlus object)
    printGPSInfo(context.getGPSModule());

    LogUtils::info("=======================================\n\n");
}

String DebugInfoPrinter::formatErrorFlags(uint32_t errorFlags) {
    if (errorFlags == 0) return "[OK]";

    String result = "";

    if (errorFlags & LIQUID_TANK_EMPTY)          result += "[LT]";
    if (errorFlags & INSUFFICIENT_FLOW)          result += "[IF]";
    if (errorFlags & FLOW_NOT_SETTLED)           result += "[FS]";
    if (errorFlags & MOTOR_STUCK)                result += "[MS]";
    if (errorFlags & BATTERY_LOW)                result += "[BL]";
    if (errorFlags & NO_SATELLITE_CONNECTED)     result += "[NS]";
    if (errorFlags & INVALID_SATELLITE_INFO)     result += "[IS]";
    if (errorFlags & INVALID_GPS_LOCATION)       result += "[GL]";
    if (errorFlags & INVALID_GPS_SPEED)          result += "[GS]";
    if (errorFlags & INVALID_PARAM_COUNT)        result += "[PC]";
    if (errorFlags & MESSAGE_PARSE_ERROR)        result += "[MP]";
    if (errorFlags & HARDWARE_ERROR)             result += "[HW]";

    return result;
}

void DebugInfoPrinter::printRealTimeData(SystemContext& context) {
    const DispenserChannel& left = context.getLeftChannel();
    const DispenserChannel& right = context.getRightChannel();

    // Main PI control debug line
    LogUtils::info("[LOG] Time: %lu\n", millis());

    LogUtils::info(" LEFT  | TargetFlow: %.2f | RealFlow: %.2f | Error: %.2f | CtrlSig: %d | Distance: %d | AreaPerSec: %.2f | Liquid: %.2f\n",
           left.getTargetFlowRatePerMin(),
           left.getRealFlowRatePerMin(),
           left.getPIController().getError(),
           left.getPIController().getControlSignal(),
           left.getDistanceTaken(),
           left.getProcessedAreaPerSec(),
           left.getLiquidConsumed()
    );

    LogUtils::info(" RIGHT | TargetFlow: %.2f | RealFlow: %.2f | Error: %.2f | CtrlSig: %d | Distance: %d | AreaPerSec: %.2f | Liquid: %.2f\n",
           right.getTargetFlowRatePerMin(),
           right.getRealFlowRatePerMin(),
           right.getPIController().getError(),
           right.getPIController().getControlSignal(),
           right.getDistanceTaken(),
           right.getProcessedAreaPerSec(),
           right.getLiquidConsumed()
    );

    // Task state and metrics
    LogUtils::info(" LEFT  [TASK] State: %s | Duration: %d s | Distance: %d m | AreaDone: %.2f daa | LiquidUsed: %.2f L\n",
           left.getTaskStateName(),
           left.getTaskDuration(),
           left.getDistanceTaken(),
           left.getAreaCompleted(),
           left.getLiquidConsumed()
    );

    LogUtils::info(" RIGHT [TASK] State: %s | Duration: %d s | Distance: %d m | AreaDone: %.2f daa | LiquidUsed: %.2f L\n",
           right.getTaskStateName(),
           right.getTaskDuration(),
           right.getDistanceTaken(),
           right.getAreaCompleted(),
           right.getLiquidConsumed()
    );

    // Error flags
    String leftErrorStr = formatErrorFlags(left.getErrorFlags());
    String rightErrorStr = formatErrorFlags(right.getErrorFlags());

    LogUtils::info(" LEFT  [ERROR] Flags: %08X %s\n", left.getErrorFlags(), leftErrorStr.c_str());
    LogUtils::info(" RIGHT [ERROR] Flags: %08X %s\n", right.getErrorFlags(), rightErrorStr.c_str());
}

void DebugInfoPrinter::printErrorSummary(SystemContext& context) {
    const DispenserChannel& left = context.getLeftChannel();
    const DispenserChannel& right = context.getRightChannel();

    LogUtils::info("[ERROR SUMMARY] LEFT: %s | RIGHT: %s\n",
           formatErrorFlags(left.getErrorFlags()).c_str(),
           formatErrorFlags(right.getErrorFlags()).c_str());
}

void DebugInfoPrinter::printSystemInfo(SystemContext& context) {
    const SystemParams& params = context.getPrefs().getParams();

    LogUtils::info("[SYSTEM info] TankLevel: %.2f | ClientInWorkZone: %s | MinWorkingSpeed: %.2f km/h | SimSpeed: %.2f km/h | BoomWidth Left: %.2f m | BoomWidth Right: %.2f m\n",
           DispenserChannel::getTankLevel(),
           DispenserChannel::isClientInWorkZone() ? "YES" : "NO",
           params.minWorkingSpeed,
           params.simSpeed,
           context.getLeftChannel().getBoomWidth(),
           context.getRightChannel().getBoomWidth());
}

// Portable helper to add a formatted field to buffer with " | " separator
static int addFieldToBuffer(char* buffer, int& n, bool& first, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (!first) {
        n += sprintf(buffer + n, " | ");
    } else {
        first = false;
    }

    n += vsprintf(buffer + n, fmt, args);

    va_end(args);
    return n;
}

void DebugInfoPrinter::printGPSInfo(TinyGPSPlus& gpsModule) {
    char buffer[256];
    int n = 0;
    bool first = true;

    // --- GPS FIX indicator ---
    bool gpsFix = gpsModule.location.isValid() &&
                  gpsModule.satellites.isValid() &&
                  gpsModule.satellites.value() >= 4;

    n += sprintf(buffer + n, "[%s] ", gpsFix ? "FIX OK" : "NO FIX");

    if (gpsModule.satellites.isValid()) {
        addFieldToBuffer(buffer, n, first, "Sats: %d", gpsModule.satellites.value());
    }

    if (gpsModule.hdop.isValid()) {
        addFieldToBuffer(buffer, n, first, "HDOP: %.2f, Age: %lu", gpsModule.hdop.hdop(), gpsModule.hdop.age());
    }

    if (gpsModule.location.isValid()) {
        addFieldToBuffer(buffer, n, first, "Lat: %.6f, Lng: %.6f, Age: %lu",
                         gpsModule.location.lat(), gpsModule.location.lng(), gpsModule.location.age());
    }

    if (gpsModule.altitude.isValid()) {
        addFieldToBuffer(buffer, n, first, "Alt: %.2f m", gpsModule.altitude.meters());
    }

    if (gpsModule.course.isValid()) {
        addFieldToBuffer(buffer, n, first, "Course: %.2f deg, Card: %s",
                         gpsModule.course.deg(), TinyGPSPlus::cardinal(gpsModule.course.deg()));
    }

    if (gpsModule.speed.isValid()) {
        addFieldToBuffer(buffer, n, first, "Speed: %.2f kmph, Age: %lu", gpsModule.speed.kmph(), gpsModule.speed.age());
    }

    TinyGPSDate &d = gpsModule.date;
    if (d.isValid()) {
        addFieldToBuffer(buffer, n, first, "Date: %02d.%02d.%02d Age: %lu",
                         d.month(), d.day(), d.year(), d.age());
    }

    TinyGPSTime &t = gpsModule.time;
    if (t.isValid()) {
        addFieldToBuffer(buffer, n, first, "Time: %02d:%02d:%02d Age: %lu",
                         t.hour(), t.minute(), t.second(), t.age());
    }

    if (n > 0) {
        LogUtils::info("[GPS] %s\n", buffer);
    }
}

void DebugInfoPrinter::printResetReason(const char* cpuLabel, int reason) {
    const char* reasonStr = nullptr;

    switch (reason) {
        case 1:  reasonStr = "Vbat reset"; break;
        case 3:  reasonStr = "SW reset core"; break;
        case 4:  reasonStr = "Legacy WDT core"; break;
        case 5:  reasonStr = "DeepSleep reset"; break;
        case 6:  reasonStr = "SLC reset core"; break;
        case 7:  reasonStr = "TGrp0 WDT core"; break;
        case 8:  reasonStr = "TGrp1 WDT core"; break;
        case 9:  reasonStr = "RTC WDT core"; break;
        case 10: reasonStr = "Instrusion reset"; break;
        case 11: reasonStr = "TimeGrp reset CPU"; break;
        case 12: reasonStr = "SW reset CPU"; break;
        case 13: reasonStr = "RTC WDT CPU"; break;
        case 14: reasonStr = "APP CPU reset by PRO"; break;
        case 15: reasonStr = "VDD unstable reset"; break;
        case 16: reasonStr = "RTC WDT core+RTC"; break;
        default: reasonStr = "Unspecified reset"; break;
    }

    LogUtils::info("[RESET] %s: %s\n", cpuLabel, reasonStr);
}

void DebugInfoPrinter::printMotorDiagnostics(float pos1, float pos2, float current1, float current2) {
    LogUtils::info("[MOTORS] Pot1: %.2fV | Pot2: %.2fV | Curr1: %.2fA | Curr2: %.2fA\n",
           pos1, pos2, current1, current2);
}

void DebugInfoPrinter::printTempSensorStatus(DS18B20Sensor& sensor) {
    if (sensor.isReady()) {
        float temp = sensor.getTemperatureC();
        String id = sensor.getSensorID();

        LogUtils::info("[TEMP SENSOR] DS18B20 found | SensorID: %s | Temperature: %.2f °C\n", id.c_str(), temp);
    } else {
        LogUtils::info("[TEMP SENSOR] DS18B20 not found\n");
    }
}

void DebugInfoPrinter::printDeviceIdentifiers(SystemContext &context) {
    String boardID = context.getBoardID();
    String espID = context.getEspID();
    String bleMAC = context.getBleMAC();

    LogUtils::info("[DEVICE] ChipID: %s | BLE MAC: %s | BoardID: %s\n", espID.c_str(), bleMAC.c_str(), boardID.c_str());
}

void DebugInfoPrinter::printVersionInfo() {
    LogUtils::info("[VERSION] Firmware: %s | Device: %s | Build: %s %s\n",
           FIRMWARE_VERSION, DEVICE_VERSION, BUILD_DATE, BUILD_TIME);
}

void DebugInfoPrinter::printAppInfo() {
    LogUtils::info("[APP] Fertilizer Dispenser Control System ... Bootup ...\n");
}
