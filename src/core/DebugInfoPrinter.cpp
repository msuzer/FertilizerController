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

void DebugInfoPrinter::printAll(SystemContext& context)
{
    printf("========== SYSTEM DEBUG INFO ==========\n");

    // System info (Tank, WorkZone, Config Params)
    printSystemInfo(context);

    // Error summary
    printErrorSummary(context);

    // Real-time data (flow, PI controller, errors, GPS)
    printRealTimeData(context);

    // Detailed GPS info (TinyGPSPlus object)
    printGPSInfo(context.getGPSModule());

    printf("=======================================\n");
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
    printf("[LOG] Time: %lu\n", millis());

    printf(" LEFT  | TargetFlow: %.2f | RealFlow: %.2f | Error: %.2f | CtrlSig: %d | Distance: %d | AreaPerSec: %.2f | Liquid: %.2f\n",
           left.getTargetFlowRatePerMin(),
           left.getRealFlowRatePerMin(),
           left.getPIController().getError(),
           left.getPIController().getControlSignal(),
           left.getDistanceTaken(),
           left.getProcessedAreaPerSec(),
           left.getLiquidConsumed()
    );

    printf(" RIGHT | TargetFlow: %.2f | RealFlow: %.2f | Error: %.2f | CtrlSig: %d | Distance: %d | AreaPerSec: %.2f | Liquid: %.2f\n",
           right.getTargetFlowRatePerMin(),
           right.getRealFlowRatePerMin(),
           right.getPIController().getError(),
           right.getPIController().getControlSignal(),
           right.getDistanceTaken(),
           right.getProcessedAreaPerSec(),
           right.getLiquidConsumed()
    );

    // Task state and metrics
    printf(" LEFT  [TASK] State: %s | Duration: %d s | Distance: %d m | AreaDone: %.2f daa | LiquidUsed: %.2f L\n",
           left.getTaskStateName(),
           left.getTaskDuration(),
           left.getDistanceTaken(),
           left.getAreaCompleted(),
           left.getLiquidConsumed()
    );

    printf(" RIGHT [TASK] State: %s | Duration: %d s | Distance: %d m | AreaDone: %.2f daa | LiquidUsed: %.2f L\n",
           right.getTaskStateName(),
           right.getTaskDuration(),
           right.getDistanceTaken(),
           right.getAreaCompleted(),
           right.getLiquidConsumed()
    );

    // Error flags
    String leftErrorStr = formatErrorFlags(left.getErrorFlags());
    String rightErrorStr = formatErrorFlags(right.getErrorFlags());

    printf(" LEFT  [ERROR] Flags: %08X %s\n", left.getErrorFlags(), leftErrorStr.c_str());
    printf(" RIGHT [ERROR] Flags: %08X %s\n", right.getErrorFlags(), rightErrorStr.c_str());
}

void DebugInfoPrinter::printErrorSummary(SystemContext& context) {
    const DispenserChannel& left = context.getLeftChannel();
    const DispenserChannel& right = context.getRightChannel();

    printf("[ERROR SUMMARY] LEFT: %s | RIGHT: %s\n",
           formatErrorFlags(left.getErrorFlags()).c_str(),
           formatErrorFlags(right.getErrorFlags()).c_str());
}

void DebugInfoPrinter::printSystemInfo(SystemContext& context) {
    const SystemParams& params = context.getPrefs().getParams();

    printf("[SYSTEM INFO] TankLevel: %.2f | ClientInWorkZone: %s | MinWorkingSpeed: %.2f km/h | SimSpeed: %.2f km/h | BoomWidth Left: %.2f m | BoomWidth Right: %.2f m\n",
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
    }
    else {
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
        printf("[GPS] %s\n", buffer);
    }
}

void DebugInfoPrinter::printResetReason(const char* cpuLabel, int reason)
{
    const char* reasonStr = nullptr;

    switch (reason) {
        case 1:  reasonStr = "Vbat power on reset"; break;
        case 3:  reasonStr = "Software reset digital core"; break;
        case 4:  reasonStr = "Legacy watch dog reset digital core"; break;
        case 5:  reasonStr = "Deep Sleep reset digital core"; break;
        case 6:  reasonStr = "Reset by SLC module, reset digital core"; break;
        case 7:  reasonStr = "Timer Group0 Watch dog reset digital core"; break;
        case 8:  reasonStr = "Timer Group1 Watch dog reset digital core"; break;
        case 9:  reasonStr = "RTC Watch dog Reset digital core"; break;
        case 10: reasonStr = "Instrusion tested to reset CPU"; break;
        case 11: reasonStr = "Time Group reset CPU"; break;
        case 12: reasonStr = "Software reset CPU"; break;
        case 13: reasonStr = "RTC Watch dog Reset CPU"; break;
        case 14: reasonStr = "for APP CPU, reset by PRO CPU"; break;
        case 15: reasonStr = "Reset when the vdd voltage is not stable"; break;
        case 16: reasonStr = "RTC Watch dog reset digital core and rtc module"; break;
        default: reasonStr = "Unspecified error caused Reset"; break;
    }

    printf("[RESET] %s: %s\n", cpuLabel, reasonStr);
}

void DebugInfoPrinter::printMotorDiagnostics(float pos1, float pos2, float current1, float current2) {
    printf("[MOTORS] Pot1: %.4f V | Pot2: %.4f V | Curr1: %.4f V | Curr2: %.4f V\n",
           pos1, pos2, current1, current2);
}

void DebugInfoPrinter::printTempSensorStatus(DS18B20Sensor& sensor) {
    if (sensor.isReady()) {
        float temp = sensor.getTemperatureC();
        String id = sensor.getSensorID();

        printf("[TEMP SENSOR] DS18B20 found | SensorID: %s | Temperature: %.2f °C\n", id.c_str(), temp);
    } else {
        printf("[TEMP SENSOR] DS18B20 not found\n");
    }
}

void DebugInfoPrinter::printVersionInfo() {
    printf("[VERSION] Firmware: %s | Device: %s | Build: %s %s\n",
           FIRMWARE_VERSION, DEVICE_VERSION, BUILD_DATE, BUILD_TIME);
}
