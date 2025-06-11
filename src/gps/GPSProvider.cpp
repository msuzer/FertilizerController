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

void GPSProvider::printGPSData(void) {
  char buffer[256];
  int n = 0;

  if (gpsModule->satellites.isValid()) {
    n+= sprintf(buffer + n, "Sats: %d", gpsModule->satellites.value());
  }

  if (gpsModule->hdop.isValid()) {
      n+= sprintf(buffer + n, " | HDOP: %.2f, Age: %lu", gpsModule->hdop.hdop(), gpsModule->hdop.age()); // HDOP: Horizontal Dilution of Precision
  }

  if (gpsModule->location.isValid()) {
      n+= sprintf(buffer + n, " | Lat: %.6f, Lng: %.6f, Age: %lu", gpsModule->location.lat(), gpsModule->location.lng(), gpsModule->location.age());
  }

  if (gpsModule->altitude.isValid()) {
      n+= sprintf(buffer + n, " | Alt: %.2fm", gpsModule->altitude.meters());
  }

  if (gpsModule->course.isValid()) {
      n+= sprintf(buffer + n, " | Course: %.2f, Card: %s", gpsModule->course.deg(), TinyGPSPlus::cardinal(gpsModule->course.deg()));
  }

  if (gpsModule->speed.isValid()) {
      n+= sprintf(buffer + n, " | Speed: %.2f kmph, Age: %lu", gpsModule->speed.kmph(), gpsModule->speed.age());
  }

  TinyGPSDate &d = gpsModule->date;

  if (d.isValid()) {
    n+= sprintf(buffer + n, " | Date: %02d.%02d.%02d Age: %lu", d.month(), d.day(), d.year(), d.age());
  }

  TinyGPSTime &t = gpsModule->time;

  if (t.isValid()) {
    n+= sprintf(buffer + n, " | Time: %02d:%02d:%02d Age: %lu", t.hour(), t.minute(), t.second(), t.age());
  }

  if (n > 0) {
    printf("%s\n", buffer);
  }
}
