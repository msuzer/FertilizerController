#pragma once

#include "core/SystemContext.h"
#include <TinyGPSPlus.h>

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
};
