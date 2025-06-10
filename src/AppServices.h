// AppServices.h
#pragma once

#include <TinyGPSPlus.h>

// Forward declarations to avoid circular dependencies
class BLETextServer;
class BLECommandParser;
class GPSProvider;
class PIController;
class SystemContext;
class SystemPreferences;
class TaskController;

struct AppServices {
    SystemContext* systemContext;
    TaskController* taskController;
    BLETextServer* bleServer;
    SystemPreferences* prefs;
    BLECommandParser* parser;
    TinyGPSPlus* gpsModule;
    GPSProvider* gpsProvider;
    PIController* pi1;
    PIController* pi2;

    AppServices(SystemContext* ctx, TaskController *tc, SystemPreferences *pre, BLETextServer* b, BLECommandParser* par, TinyGPSPlus* m, GPSProvider* g, PIController* c1, PIController* c2)
        : systemContext(ctx), taskController(tc), prefs(pre), bleServer(b), parser(par), gpsModule(m), gpsProvider(g), pi1(c1), pi2(c2) {}
};
