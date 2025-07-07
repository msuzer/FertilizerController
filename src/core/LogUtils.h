// ============================================
// File: LogUtils.h
// Purpose: Centralized logging helpers (die, warn, info, verbose)
// Part of: Core system utilities
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================

#pragma once

#include <Arduino.h>

enum class LogLevel {
    Silent = 0,
    Fatal,
    Error,
    Warn,
    Info,
    Verbose
};

class LogUtils {
public:
    static void setLogLevel(LogLevel level);
    static LogLevel getLogLevel();
    static const char* logLevelToString(LogLevel level);

    static void die(const char* format, ...);
    static void error(const char* format, ...);
    static void warn(const char* format, ...);
    static void info(const char* format, ...);
    static void verbose(const char* format, ...);

private:
    static LogLevel currentLogLevel;
};
