#pragma once

#include "core/Constants.h"

class ApplicationMetrics {
private:
    int duration = 0;
    int distance = 0;
    float area = 0.0f;
    float consumption = 0.0f;

    static float tankLevel;  // Shared among all instances

public:
    // Increments
    void incrementDuration() { duration++; }
    void increaseDistance(int length) { distance += length; }
    void increaseArea(float value) { area += value; }
    void increaseConsumption(float value) { consumption += value; }

    // Clears
    void clearDuration() { duration = 0; }
    void clearDistance() { distance = 0; }
    void clearArea() { area = 0.0f; }
    void clearConsumption() { consumption = 0.0f; }

    // Accessors
    int getDuration() const { return duration; }
    int getDistance() const { return distance; }
    float getArea() const { return area; }
    float getConsumption() const { return consumption; }

    inline static float getTankLevel() { return tankLevel; }
    inline static void setTankLevel(float level) { tankLevel = level; }
    inline static void decreaseTankLevel(float value) { tankLevel -= value; }

    void applyFlowSlice(float flowRatePerMin) {
        float slice = flowRatePerMin / Units::MINUTE_TO_SECOND; // Convert to per-second rate
        increaseConsumption(slice);
        decreaseTankLevel(slice);
    }

    // Reset all
    void reset() {
        duration = 0;
        distance = 0;
        area = 0.0f;
        consumption = 0.0f;
    }

    // Merge another instance into this one
    ApplicationMetrics& operator+=(const ApplicationMetrics& other) {
        duration += other.duration;
        distance += other.distance;
        area += other.area;
        consumption += other.consumption;
        return *this;
    }
};
