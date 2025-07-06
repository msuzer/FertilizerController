#pragma once

namespace Units {
    static constexpr float DAA_TO_SQUARE_METERS = 10000.0f;
    static constexpr float SQUARE_METERS_PER_DAA = 1000.0f; // used in per-second rate calc
    static constexpr float KMH_TO_MPS = 1000.0f / 3600.0f;
    static constexpr float SPRAY_RATE_DENOMINATOR = 6.0f; // legacy formula
}
