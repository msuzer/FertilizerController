#include "GPSProvider.h"

bool GPSProvider::isValid() const {
    return services->gpsModule->location.isValid() &&
           services->gpsModule->speed.isValid() &&
           services->gpsModule->satellites.isValid() && services->gpsModule->satellites.value() >= MIN_SATELLITES_NEEDED &&
           services->gpsModule->hdop.isValid() && services->gpsModule->hdop.hdop() <= MAX_HDOP_TOLERATED;
}

Location_t GPSProvider::getLocation() const {
    return isValid() ? Location_t(services->gpsModule->location.lat(), services->gpsModule->location.lng()) : Location_t();
}

float GPSProvider::getSpeed(bool mps) const {
    return isValid() ? (mps ? services->gpsModule->speed.mps() : services->gpsModule->speed.kmph()) : 0.0f;
}

int GPSProvider::getSatelliteCount() const {
    return services->gpsModule->satellites.isValid() ? services->gpsModule->satellites.value() : 0;
}