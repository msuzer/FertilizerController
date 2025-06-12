// ============================================
// File: GPSProvider.cpp
// Purpose: Manages GPS interface and provides GPS data
// Part of: GPS Layer
//
// License: Proprietary License
// Author: Mehmet H Suzer
// Date: 13 June 2025
// ============================================
#include "GPSProvider.h"

GPSProvider& GPSProvider::getInstance() {
    static GPSProvider instance;
    return instance;
}

bool GPSProvider::isValid() const {
    
    return gpsModule->location.isValid() &&
           gpsModule->speed.isValid() &&
           gpsModule->satellites.isValid() && gpsModule->satellites.value() >= MIN_SATELLITES_NEEDED &&
           gpsModule->hdop.isValid() && gpsModule->hdop.hdop() <= MAX_HDOP_TOLERATED;
}

Location_t GPSProvider::getLocation() const {
    return isValid() ? Location_t(gpsModule->location.lat(), gpsModule->location.lng()) : Location_t();
}

float GPSProvider::getSpeed(bool mps) const {
    return isValid() ? (mps ? gpsModule->speed.mps() : gpsModule->speed.kmph()) : 0.0f;
}

int GPSProvider::getSatelliteCount() const {
    return gpsModule->satellites.isValid() ? gpsModule->satellites.value() : 0;
}
