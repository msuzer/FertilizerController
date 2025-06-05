#pragma once

#include "AppServices.h"

#define MIN_SATELLITES_NEEDED           4
#define MAX_HDOP_TOLERATED              25.0f

struct Location_t {
    double lat;
    double lng;
    Location_t(double latitude = 0.0, double longitude = 0.0) : lat(latitude), lng(longitude) {}
};

class GPSProvider {
public:
    void injectServices(AppServices* s)  { services = s; }
    bool isValid() const;
    Location_t getLocation() const;
    float getSpeed(bool mps = false) const;
    int getSatelliteCount() const;
private:
    AppServices* services = nullptr;
};