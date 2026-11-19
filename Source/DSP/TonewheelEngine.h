#pragma once

#include "TonewheelTable.h"
#include "OrganVoicing.h"
#include "KeyMatrix.h"
#include "PercussionEngine.h"

#include <array>
#include <cmath>
#include <numbers>

namespace GreenOnions::DSP
{

class TonewheelEngine
{
public:
    struct Parameters
    {
        std::array<float, kNumDrawbars> upperDrawbars      = { 8, 8, 8, 8, 0, 0, 0, 0, 0 }; // 0..8
        std::array<float, kNumDrawbars> lowerDrawbars      = { 0, 0, 8, 8, 8, 0, 0, 0, 0 }; // 0..8
        std::array<float, kNumPedalDrawbars> pedalDrawbars = { 6, 4 };                      // 0..8

        PercussionEngine::Settings percussion {};

        float keyClick = 0.55f; // [0..1]
        float leakage  = 0.25f; // [0..1] tonewheel crosstalk & generator age

        bool upperVibratoOn = true;
        bool lowerVibratoOn = false;

        int  organModel   = 1;    // 0..4 ("Load Your Organ" console voicing)
        bool muteWhenIdle = true; // Smart Idle Gate: mutes continuous generator bleed after 1.2s rest
    };

    struct SampleOutput
    {
        float toVibrato = 0.0f; // Routed through Scanner Vibrato / Chorus
        float dryBypass = 0.0f; // Bypasses Scanner V/C (unrouted manual, percussion, pedals)
    };

    void prepare(double sampleRate) noexcept
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        const double twoPiOverSr = (2.0 * std::numbers::pi) / sr_;

        for (int w = 0; w < kNumTonewheels; ++w)
        {
            phaseInc_[static_cast<size_t>(w)] = table_.frequencies[static_cast<size_t>(w)] * twoPiOverSr;
        }

        // Deterministic staggered initial phases (physical gear mesh at motor start-up)
        // Only initialized once in prepare() — NEVER reset on note-on!
        if (!phasesInitialized_)
        {
            uint32_t seed = 0xB3122962u;
            for (int w = 0; w < kNumTonewheels; ++w)
            {
                seed ^= seed << 13;
                seed ^= seed >> 17;
                seed ^= seed << 5;
                const double norm = static_cast<double>(seed) * (1.0 / 4294967296.0);
                phase_[static_cast<size_t>(w)] = norm * 2.0 * std::numbers::pi;
            }
            phasesInitialized_ = true;
        }

        keyMatrix_.prepare(sr_);
        percussion_.prepare(sr_);

        // Smooth ~25ms calibration crossfade for click-free live console hot-swapping
        calSmoothCoeff_    = 1.0f - std::exp(-1.0f / static_cast<float>(0.025 * sr_));
        idleAttackStep_    = 1.0f / static_cast<float>(0.0004 * sr_); // 0.4ms wake-up
        idleReleaseStep_   = 1.0f / static_cast<float>(0.4000 * sr_); // 400ms smooth fade-out
        idleHoldSamples_   = static_cast<int>(1.2 * sr_);             // 1.2s natural phrasing hold
        idleGateGain_      = 0.0f;
        idleHoldCounter_   = 0;

        activeModelIndex_  = 1;
        targetCalibration_ = buildWheelCalibrationTable(activeModelIndex_);
        wheelCalibration_  = targetCalibration_;
    }

    void reset() noexcept
    {
        keyMatrix_.reset();
        percussion_.reset();
        idleGateGain_    = 0.0f;
        idleHoldCounter_ = 0;
    }

    void noteOnUpper(int midiNote, float velocity) noexcept
    {
        const int keyIdx = midiNote - kManualBaseMidi;
        if (keyIdx < 0 || keyIdx >= kNumManualKeys) return;
        const bool triggerPerc = keyMatrix_.noteOnUpper(keyIdx, velocity);
        if (triggerPerc)
            percussion_.triggerSingle();
    }

    void noteOffUpper(int midiNote) noexcept
    {
        const int keyIdx = midiNote - kManualBaseMidi;
        if (keyIdx < 0 || keyIdx >= kNumManualKeys) return;
        keyMatrix_.noteOffUpper(keyIdx);
    }

    void noteOnLower(int midiNote, float velocity) noexcept
    {
        const int keyIdx = midiNote - kManualBaseMidi;
        if (keyIdx < 0 || keyIdx >= kNumManualKeys) return;
        keyMatrix_.noteOnLower(keyIdx, velocity);
    }

    void noteOffLower(int midiNote) noexcept
    {
        const int keyIdx = midiNote - kManualBaseMidi;
        if (keyIdx < 0 || keyIdx >= kNumManualKeys) return;
        keyMatrix_.noteOffLower(keyIdx);
    }

    void noteOnPedal(int midiNote, float velocity) noexcept
    {
        const int pedalIdx = midiNote - kPedalBaseMidi;
        if (pedalIdx < 0 || pedalIdx >= kNumPedalKeys) return;
        keyMatrix_.noteOnPedal(pedalIdx, velocity);
    }

    void noteOffPedal(int midiNote) noexcept
    {
        const int pedalIdx = midiNote - kPedalBaseMidi;
        if (pedalIdx < 0 || pedalIdx >= kNumPedalKeys) return;
        keyMatrix_.noteOffPedal(pedalIdx);
    }

    void allNotesOff() noexcept
    {
        keyMatrix_.allNotesOff();
    }

    // Processes 1 audio sample across all 91 free-running tonewheels, key matrix, and percussion
    SampleOutput processSample(const Parameters& params) noexcept
    {
        constexpr double kTwoPi = 2.0 * std::numbers::pi;

        // 0. Hot-swap "Load Your Organ" console calibration table if changed
        if (params.organModel != activeModelIndex_)
        {
            activeModelIndex_  = params.organModel;
            targetCalibration_ = buildWheelCalibrationTable(activeModelIndex_);
        }
        const auto& organProf = getOrganProfile(activeModelIndex_);

        // 1. Advance all 91 continuously spinning tonewheels with per-wheel console calibration
        std::array<float, kNumTonewheels> rawWheels {};
        for (int w = 0; w < kNumTonewheels; ++w)
        {
            const size_t idx = static_cast<size_t>(w);
            wheelCalibration_[idx] += (targetCalibration_[idx] - wheelCalibration_[idx]) * calSmoothCoeff_;

            double p = phase_[idx] + phaseInc_[idx];
            if (p >= kTwoPi) p -= kTwoPi;
            phase_[idx] = p;

            const float s1 = static_cast<float>(std::sin(p));
            const float wheelGain = table_.taperGains[idx] * wheelCalibration_[idx];

            if (w < 12)
            {
                // Wheels 1..12 (C1..B1): Authentic complex pedal tonewheels
                // Blend fundamental + 2nd, 3rd, 4th, 5th harmonics via trig recurrence
                const float c1 = static_cast<float>(std::cos(p));
                const float s2 = 2.0f * s1 * c1;
                const float c2 = 2.0f * c1 * c1 - 1.0f;
                const float s3 = s2 * c1 + c2 * s1;
                const float c3 = c2 * c1 - s2 * s1;
                const float s4 = 2.0f * s2 * c2;
                const float s5 = s4 * c1 + (2.0f * c2 * c2 - 1.0f) * s1;
                (void) c3;

                rawWheels[idx] = (0.56f * s1 + 0.24f * s2 + 0.13f * s3 + 0.055f * s4 + 0.025f * s5)
                               * wheelGain;
            }
            else
            {
                rawWheels[idx] = s1 * wheelGain;
            }
        }

        // 2. Apply Crosstalk / Leakage matrix (scaled by console wiring-harness profile)
        std::array<float, kNumTonewheels> wheels {};
        const float leak = params.leakage * organProf.crosstalkScale;
        if (leak > 1e-4f)
        {
            const float bleedA = leak * 0.022f; // Bin partner (4 octaves away)
            const float bleedB = leak * 0.010f; // Adjacent wiring loom neighbor
            for (int w = 0; w < kNumTonewheels; ++w)
            {
                const size_t idx = static_cast<size_t>(w);
                wheels[idx] = rawWheels[idx]
                            + bleedA * rawWheels[static_cast<size_t>(table_.crosstalkNeighborA[idx])]
                            + bleedB * rawWheels[static_cast<size_t>(table_.crosstalkNeighborB[idx])];
            }
        }
        else
        {
            wheels = rawWheels;
        }

        // 3. Advance 9-contact key matrix & collect organic key click (scaled by console click character)
        const float clickSample = keyMatrix_.processContacts(params.keyClick * organProf.clickScale);

        // 4. Convert drawbar settings to gains (with authentic B-3 percussion interactions)
        std::array<float, kNumDrawbars> upperGains {};
        std::array<float, kNumDrawbars> lowerGains {};
        for (int db = 0; db < kNumDrawbars; ++db)
        {
            upperGains[static_cast<size_t>(db)] = drawbarValueToGain(params.upperDrawbars[static_cast<size_t>(db)]);
            lowerGains[static_cast<size_t>(db)] = drawbarValueToGain(params.lowerDrawbars[static_cast<size_t>(db)]);
        }

        // Authentic B-3: Percussion steals the Upper 1' drawbar (index 8)
        if (PercussionEngine::stealsFirstFootDrawbar(params.percussion))
            upperGains[8] = 0.0f;

        const float upperPercAtten = PercussionEngine::upperDrawbarAttenuation(params.percussion);
        const float percEnv        = percussion_.processSample(params.percussion);
        const int   percTapIdx     = params.percussion.thirdHarmonic ? 1 : 0;

        // 5. Sum Upper Manual & Percussion
        float upperSum = 0.0f;
        float percSum  = 0.0f;
        const auto& upperKeys = keyMatrix_.getUpperKeys();
        for (int k = 0; k < kNumManualKeys; ++k)
        {
            const auto& key = upperKeys[static_cast<size_t>(k)];
            if (!key.active) continue;

            const auto& wheelMap = table_.manualWheelIndex[static_cast<size_t>(k)];
            for (int db = 0; db < kNumDrawbars; ++db)
            {
                const float g = upperGains[static_cast<size_t>(db)];
                if (g <= 0.0f) continue;
                const float c = key.contacts[static_cast<size_t>(db)].level;
                if (c <= 0.0f) continue;
                upperSum += wheels[static_cast<size_t>(wheelMap[static_cast<size_t>(db)])] * g * c;
            }

            if (percEnv > 0.0f && key.isDown)
            {
                const int pw = table_.percussionWheelIndex[static_cast<size_t>(k)][static_cast<size_t>(percTapIdx)];
                // Gate percussion via 8' contact closure for natural attack alignment
                percSum += wheels[static_cast<size_t>(pw)] * key.contacts[2].level;
            }
        }
        upperSum *= upperPercAtten;
        percSum  *= percEnv * 0.95f;

        // 6. Sum Lower Manual
        float lowerSum = 0.0f;
        const auto& lowerKeys = keyMatrix_.getLowerKeys();
        for (int k = 0; k < kNumManualKeys; ++k)
        {
            const auto& key = lowerKeys[static_cast<size_t>(k)];
            if (!key.active) continue;

            const auto& wheelMap = table_.manualWheelIndex[static_cast<size_t>(k)];
            for (int db = 0; db < kNumDrawbars; ++db)
            {
                const float g = lowerGains[static_cast<size_t>(db)];
                if (g <= 0.0f) continue;
                const float c = key.contacts[static_cast<size_t>(db)].level;
                if (c <= 0.0f) continue;
                lowerSum += wheels[static_cast<size_t>(wheelMap[static_cast<size_t>(db)])] * g * c;
            }
        }

        // 7. Sum 25-Note Pedalboard (16' and 8' with complex pedal richness)
        float pedalSum = 0.0f;
        const float ped16 = drawbarValueToGain(params.pedalDrawbars[0]);
        const float ped8  = drawbarValueToGain(params.pedalDrawbars[1]);
        if (ped16 > 0.0f || ped8 > 0.0f)
        {
            const auto& pedalKeys = keyMatrix_.getPedalKeys();
            for (int p = 0; p < kNumPedalKeys; ++p)
            {
                const auto& pk = pedalKeys[static_cast<size_t>(p)];
                if (!pk.active || pk.level <= 0.0f) continue;

                const int w16 = table_.pedalWheelIndex[static_cast<size_t>(p)][0];
                const int w8  = table_.pedalWheelIndex[static_cast<size_t>(p)][1];

                // For 16' pedal notes above B1 (w16 >= 12), add subtle 2nd/3rd harmonic warmth
                // from the upper wheels so the pedal voice stays cohesive across all 25 notes.
                float sig16 = wheels[static_cast<size_t>(w16)];
                if (w16 >= 12 && w16 + 19 < kNumTonewheels)
                {
                    sig16 = 0.72f * sig16
                          + 0.20f * wheels[static_cast<size_t>(w16 + 12)]
                          + 0.08f * wheels[static_cast<size_t>(w16 + 19)];
                }

                // 8' pedal tap with mild 2nd harmonic warmth
                float sig8 = wheels[static_cast<size_t>(w8)];
                if (w8 + 12 < kNumTonewheels)
                    sig8 = 0.85f * sig8 + 0.15f * wheels[static_cast<size_t>(w8 + 12)];

                pedalSum += (sig16 * ped16 + sig8 * ped8) * pk.level * 1.45f;
            }
        }

        // Smart Idle Generator Gate:
        // Keeps all 91 tonewheel phases continuously spinning, wakes in 0.4ms on any key/pedal,
        // holds 1.2s after release, then fades continuous generator leakage smoothly to 0 over 400ms.
        const bool anyNotePlaying = (keyMatrix_.getUpperHeldCount() > 0)
                                 || (keyMatrix_.getLowerHeldCount() > 0)
                                 || (keyMatrix_.getPedalHeldCount() > 0);
        if (anyNotePlaying || !params.muteWhenIdle)
        {
            idleHoldCounter_ = idleHoldSamples_;
            idleGateGain_    = std::min(1.0f, idleGateGain_ + idleAttackStep_);
        }
        else
        {
            if (idleHoldCounter_ > 0)
                --idleHoldCounter_;
            else if (idleGateGain_ > 0.0f)
                idleGateGain_ = std::max(0.0f, idleGateGain_ - idleReleaseStep_);
        }

        // Subtle generator-wide leakage floor (distant spinning wheels hum/shimmer)
        float generatorFloor = 0.0f;
        if (leak > 1e-4f && idleGateGain_ > 1e-4f)
        {
            // Sum representative mid/high wheels at authentic whisper level, gated by idleGateGain_
            generatorFloor = (rawWheels[24] + rawWheels[36] + rawWheels[48] + rawWheels[55] + rawWheels[67])
                           * (leak * 0.0012f * idleGateGain_);
        }

        // Normalize summing bus so a full 888888888 chord sits cleanly before the AO-28 preamp
        constexpr float kBusScale = 0.145f;
        upperSum *= kBusScale;
        lowerSum *= kBusScale;
        percSum  *= kBusScale * 1.35f;
        pedalSum *= kBusScale * 1.15f;

        // Route Upper/Lower independently to Scanner V/C or Dry Bypass (authentic B-3 routing)
        SampleOutput out;
        if (params.upperVibratoOn)
            out.toVibrato += upperSum + clickSample * 0.65f;
        else
            out.dryBypass += upperSum + clickSample * 0.65f;

        if (params.lowerVibratoOn)
            out.toVibrato += lowerSum;
        else
            out.dryBypass += lowerSum;

        // Percussion, Pedals, and generator floor bypass the scanner (just like the real B-3)
        out.dryBypass += percSum + pedalSum + generatorFloor + clickSample * 0.35f;

        return out;
    }

    const TonewheelTable& getTable() const noexcept { return table_; }
    const KeyMatrix&      getKeyMatrix() const noexcept { return keyMatrix_; }
    double                getWheelPhase(int wheelIndex) const noexcept
    {
        return phase_[static_cast<size_t>(wheelIndex)];
    }

private:
    double sr_ = 44100.0;
    bool phasesInitialized_ = false;

    TonewheelTable table_ {};
    KeyMatrix      keyMatrix_ {};
    PercussionEngine percussion_ {};

    std::array<double, kNumTonewheels> phase_ {};
    std::array<double, kNumTonewheels> phaseInc_ {};

    int activeModelIndex_ = 1;
    float calSmoothCoeff_ = 0.001f;
    std::array<float, kNumTonewheels> wheelCalibration_ {};
    std::array<float, kNumTonewheels> targetCalibration_ {};

    float idleGateGain_    = 0.0f;
    float idleAttackStep_  = 0.05f;
    float idleReleaseStep_ = 0.00005f;
    int   idleHoldSamples_ = 52920;
    int   idleHoldCounter_ = 0;
};

} // namespace GreenOnions::DSP
