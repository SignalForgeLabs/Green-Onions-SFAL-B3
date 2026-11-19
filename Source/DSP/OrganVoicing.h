#pragma once

#include "TonewheelTable.h"
#include <array>
#include <cmath>

namespace GreenOnions::DSP
{

struct OrganVoicingProfile
{
    const char* name;
    float lowShelfDb;      // Bass calibration taper (< 250 Hz)
    float highShelfDb;     // Capacitor aging / Mylar brightness (> 2.5 kHz)
    float crosstalkScale;  // Multiplier on generator leakage
    float clickScale;      // Multiplier on key contact transient
    float preampBiasBoost; // Extra AO-28 tube saturation bias & drive scale
    float wheelDriftScale; // Mechanical calibration variance / factory tolerance
};

inline constexpr int kNumOrganModels = 5;

inline constexpr std::array<OrganVoicingProfile, kNumOrganModels> kOrganProfiles = {{
    // 0: 1955 "Green Label" Wax-Cap Jazz B-3 — Warm, woody, smoky, rolled-off highs, early breakup
    { "1955 Wax-Cap Jazz B-3",    +2.8f, -3.5f, 1.35f, 1.10f, 1.25f, 1.30f },
    // 1: 1959 "Golden Era" Studio B-3 — Balanced, punchy Jimmy Smith / Booker T.
    { "1959 Golden Era B-3",      +1.0f, -1.0f, 1.00f, 1.00f, 1.00f, 1.00f },
    // 2: 1968 "Red-Cap" Rock B-3 — Bright Mylar caps, Jon Lord / Keith Emerson snarl
    { "1968 Red-Cap Rock B-3",    -0.5f, +3.2f, 0.65f, 1.25f, 1.45f, 0.80f },
    // 3: 1962 C-3 "Sanctuary" Console — Clean, majestic, deep 16' warmth, minimal bleed
    { "1962 Sanctuary C-3",       +2.0f, +0.5f, 0.45f, 0.70f, 0.65f, 0.60f },
    // 4: 1972 "Road-Worn" Club Chop — Gritty, heavily toured, heavy bleed & aggressive click
    { "1972 Road-Worn Club Chop", +1.5f, +1.8f, 1.75f, 1.50f, 1.60f, 1.65f }
}};

inline const OrganVoicingProfile& getOrganProfile(int modelIndex) noexcept
{
    const int idx = (modelIndex < 0) ? 0 : (modelIndex >= kNumOrganModels ? kNumOrganModels - 1 : modelIndex);
    return kOrganProfiles[static_cast<size_t>(idx)];
}

// Computes per-wheel magnetic pickup distance calibration + capacitor curve across all 91 tonewheels
inline std::array<float, kNumTonewheels> buildWheelCalibrationTable(int modelIndex) noexcept
{
    const auto& prof = getOrganProfile(modelIndex);
    std::array<float, kNumTonewheels> table {};

    for (int w = 0; w < kNumTonewheels; ++w)
    {
        const float norm = static_cast<float>(w) / static_cast<float>(kNumTonewheels - 1);
        float dbOffset = 0.0f;

        if (norm < 0.35f)
            dbOffset = prof.lowShelfDb * (1.0f - (norm / 0.35f));
        else if (norm > 0.55f)
            dbOffset = prof.highShelfDb * ((norm - 0.55f) / 0.45f);

        // Subtle per-wheel manufacturing variance (authentic factory pickup gap tolerance)
        const float tolerance = std::sin(static_cast<float>(w) * 1.6180339f) * 0.35f * prof.wheelDriftScale;
        table[static_cast<size_t>(w)] = std::pow(10.0f, (dbOffset + tolerance) / 20.0f);
    }
    return table;
}

} // namespace GreenOnions::DSP
