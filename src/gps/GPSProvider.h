// ============================================
// File: GPSProvider.h
// Purpose: Manages GPS interface and provides GPS data
// Part of: GPS Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#pragma once

#include <TinyGPSPlus.h>

class SystemContext; // Forward declaration

struct Location_t {
    double lat;
    double lng;
    Location_t(double latitude = 0.0, double longitude = 0.0) : lat(latitude), lng(longitude) {}
};

class GPSProvider {
    friend class SystemContext;
public:
    static constexpr float MIN_SPEED_MPS = 0.1f; // Minimum speed to consider GPS valid
    static constexpr float MIN_SPEED_KMPH = 0.36f; // Minimum speed in km/h
    static constexpr float MAX_HDOP_TOLERATED = 20.0f; // Maximum HDOP to consider GPS valid
    static constexpr float MIN_SATELLITES_NEEDED = 4; // Minimum satellites needed for valid GPS data

    GPSProvider(const GPSProvider&) = delete;
    GPSProvider& operator=(const GPSProvider&) = delete;
    GPSProvider(GPSProvider&&) = delete;
    GPSProvider& operator=(GPSProvider&&) = delete;

    inline void setModule(TinyGPSPlus* module) { gpsModule = module; }

    bool isValid() const;
    Location_t getLocation() const;
    float getSpeed(bool mps = false) const;
    int getSatelliteCount() const;
private:
    GPSProvider() = default;
    TinyGPSPlus* gpsModule = nullptr;
};
