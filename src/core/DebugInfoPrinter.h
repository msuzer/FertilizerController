// ============================================
// File: DebugInfoPrinter.h
// Purpose: Provides centralized system debug printing functions
// Part of: Core Services
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include "core/SystemContext.h"
#include <TinyGPSPlus.h>
#include "io/DS18B20Sensor.h"

class DebugInfoPrinter
{
public:
    static constexpr const char* MODULE_NAME = "DebugInfoPrinter";

    static void printAll(SystemContext& context);
    
    static void printRealTimeData(SystemContext& context);
    static String formatErrorFlags(uint32_t errorFlags);

    static void printErrorSummary(SystemContext& context);
    static void printSystemInfo(SystemContext& context);

    static void printGPSInfo(TinyGPSPlus& gpsModule);
    static void printResetReason(const char* cpuLabel, int reason);

    static void printMotorDiagnostics(float pos1, float pos2, float current1, float current2);

    static void printTempSensorStatus(DS18B20Sensor& sensor);

    static void printVersionInfo();
};
