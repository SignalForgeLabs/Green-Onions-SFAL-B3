#pragma once

#include "TonewheelTable.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace GreenOnions::DSP
{

class KeyMatrix
{
public:
    struct Contact
    {
        float level       = 0.0f; // Current contact closure [0..1]
        float target      = 0.0f; // 1.0 when key down, 0.0 when key up
        int   delayLeft   = 0;    // Stagger samples remaining before contact moves
        bool  clickArmed  = false;
        float clickAmp    = 0.0f;
    };

    struct ManualKey
    {
        bool  isDown   = false;
        bool  active   = false;
        float velocity = 0.0f;
        std::array<Contact, kNumDrawbars> contacts {};
    };

    struct PedalKey
    {
        bool  isDown    = false;
        bool  active    = false;
        float velocity  = 0.0f;
        float level     = 0.0f;
        float target    = 0.0f;
        int   delayLeft = 0;
    };

    void prepare(double sampleRate) noexcept
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        // ~0.35 ms contact rise/fall time constant
        contactCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(0.00035 * sr_));
        pedalCoeff_   = 1.0f - std::exp(-1.0f / static_cast<float>(0.0025 * sr_));

        // 2nd-order SVF bandpass at ~2.85 kHz, Q = 1.35 for organic key-click coloration
        const double fc = 2850.0;
        const double q  = 1.35;
        bpG_ = static_cast<float>(std::tan(std::numbers::pi * fc / sr_));
        bpK_ = static_cast<float>(1.0 / q);
        bpA1_ = 1.0f / (1.0f + bpG_ * (bpG_ + bpK_));
        bpA2_ = bpG_ * bpA1_;
        bpA3_ = bpG_ * bpA2_;

        reset();
    }

    void reset() noexcept
    {
        for (auto& k : upperKeys_) { k = ManualKey{}; }
        for (auto& k : lowerKeys_) { k = ManualKey{}; }
        for (auto& p : pedalKeys_) { p = PedalKey{}; }
        upperHeldCount_ = 0;
        lowerHeldCount_ = 0;
        pedalHeldCount_ = 0;
        bpIc1_ = 0.0f;
        bpIc2_ = 0.0f;
    }

    // Returns true if this note-on transitioned upper manual from 0 -> 1 held keys (for single-trigger percussion)
    bool noteOnUpper(int keyIndex, float velocity) noexcept
    {
        if (keyIndex < 0 || keyIndex >= kNumManualKeys) return false;
        const bool wasZero = (upperHeldCount_ == 0);
        auto& k = upperKeys_[static_cast<size_t>(keyIndex)];
        if (!k.isDown)
        {
            k.isDown = true;
            ++upperHeldCount_;
        }
        triggerManualKeyDown(k, velocity);
        return wasZero;
    }

    void noteOffUpper(int keyIndex) noexcept
    {
        if (keyIndex < 0 || keyIndex >= kNumManualKeys) return;
        auto& k = upperKeys_[static_cast<size_t>(keyIndex)];
        if (k.isDown)
        {
            k.isDown = false;
            if (upperHeldCount_ > 0) --upperHeldCount_;
        }
        triggerManualKeyUp(k);
    }

    void noteOnLower(int keyIndex, float velocity) noexcept
    {
        if (keyIndex < 0 || keyIndex >= kNumManualKeys) return;
        auto& k = lowerKeys_[static_cast<size_t>(keyIndex)];
        if (!k.isDown)
        {
            k.isDown = true;
            ++lowerHeldCount_;
        }
        triggerManualKeyDown(k, velocity);
    }

    void noteOffLower(int keyIndex) noexcept
    {
        if (keyIndex < 0 || keyIndex >= kNumManualKeys) return;
        auto& k = lowerKeys_[static_cast<size_t>(keyIndex)];
        if (k.isDown)
        {
            k.isDown = false;
            if (lowerHeldCount_ > 0) --lowerHeldCount_;
        }
        triggerManualKeyUp(k);
    }

    void noteOnPedal(int pedalIndex, float velocity) noexcept
    {
        if (pedalIndex < 0 || pedalIndex >= kNumPedalKeys) return;
        auto& p = pedalKeys_[static_cast<size_t>(pedalIndex)];
        if (!p.isDown)
        {
            p.isDown = true;
            ++pedalHeldCount_;
        }
        p.velocity  = velocity;
        p.active    = true;
        p.target    = 1.0f;
        p.delayLeft = static_cast<int>(nextRand01() * 0.0012 * sr_);
    }

    void noteOffPedal(int pedalIndex) noexcept
    {
        if (pedalIndex < 0 || pedalIndex >= kNumPedalKeys) return;
        auto& p = pedalKeys_[static_cast<size_t>(pedalIndex)];
        if (p.isDown)
        {
            p.isDown = false;
            if (pedalHeldCount_ > 0) --pedalHeldCount_;
        }
        p.target    = 0.0f;
        p.delayLeft = static_cast<int>(nextRand01() * 0.0008 * sr_);
    }

    void allNotesOff() noexcept
    {
        for (int i = 0; i < kNumManualKeys; ++i)
        {
            noteOffUpper(i);
            noteOffLower(i);
        }
        for (int i = 0; i < kNumPedalKeys; ++i)
            noteOffPedal(i);
    }

    // Advances all contact states by 1 sample and returns the filtered organic key-click sample
    float processContacts(float keyClickAmount) noexcept
    {
        float rawClick = 0.0f;

        auto updateManual = [&](std::array<ManualKey, kNumManualKeys>& keys)
        {
            for (auto& k : keys)
            {
                if (!k.active) continue;
                bool anyAlive = k.isDown;
                for (auto& c : k.contacts)
                {
                    if (c.delayLeft > 0)
                    {
                        --c.delayLeft;
                        anyAlive = true;
                        continue;
                    }
                    if (c.clickArmed)
                    {
                        c.clickArmed = false;
                        rawClick += c.clickAmp;
                    }
                    const float diff = c.target - c.level;
                    if (std::abs(diff) > 1e-5f)
                    {
                        c.level += diff * contactCoeff_;
                        anyAlive = true;
                    }
                    else
                    {
                        c.level = c.target;
                        if (c.level > 0.0f) anyAlive = true;
                    }
                }
                k.active = anyAlive;
            }
        };

        updateManual(upperKeys_);
        updateManual(lowerKeys_);

        for (auto& p : pedalKeys_)
        {
            if (!p.active) continue;
            if (p.delayLeft > 0)
            {
                --p.delayLeft;
                continue;
            }
            const float diff = p.target - p.level;
            if (std::abs(diff) > 1e-5f)
                p.level += diff * pedalCoeff_;
            else
                p.level = p.target;

            p.active = p.isDown || (p.level > 0.0f) || (p.delayLeft > 0);
        }

        // Bandpass filter (~2.85 kHz) for organic key click transient
        const float v3 = rawClick - bpIc2_;
        const float v1 = bpA1_ * bpIc1_ + bpA2_ * v3;
        const float v2 = bpIc2_ + bpA2_ * bpIc1_ + bpA3_ * v3;
        bpIc1_ = 2.0f * v1 - bpIc1_;
        bpIc2_ = 2.0f * v2 - bpIc2_;

        // Blend bandpass body + tiny uncolored transient edge
        const float clickOut = (v1 * 1.15f + rawClick * 0.12f) * keyClickAmount;
        return clickOut;
    }

    const std::array<ManualKey, kNumManualKeys>& getUpperKeys() const noexcept { return upperKeys_; }
    const std::array<ManualKey, kNumManualKeys>& getLowerKeys() const noexcept { return lowerKeys_; }
    const std::array<PedalKey, kNumPedalKeys>&   getPedalKeys() const noexcept { return pedalKeys_; }

    int getUpperHeldCount() const noexcept { return upperHeldCount_; }
    int getLowerHeldCount() const noexcept { return lowerHeldCount_; }
    int getPedalHeldCount() const noexcept { return pedalHeldCount_; }

private:
    void triggerManualKeyDown(ManualKey& k, float velocity) noexcept
    {
        k.velocity = velocity;
        k.active   = true;
        // Harder strikes close the 9 palladium contacts slightly tighter & punchier
        const float velScale = 0.55f + 0.45f * velocity;
        const double maxStaggerSec = 0.0025 - 0.0009 * static_cast<double>(velocity); // 0.2ms .. 1.6-2.5ms
        const double minStaggerSec = 0.0002;

        for (auto& c : k.contacts)
        {
            const double r = nextRand01();
            const double delaySec = minStaggerSec + r * (maxStaggerSec - minStaggerSec);
            c.delayLeft  = static_cast<int>(delaySec * sr_);
            c.target     = 1.0f;
            c.clickArmed = true;
            // Randomized polarity + DC thump component for organic multi-contact crackle
            const float sign = (nextRand01() > 0.35) ? 1.0f : -0.65f;
            c.clickAmp   = sign * (0.045f + 0.045f * static_cast<float>(nextRand01())) * velScale;
        }
    }

    void triggerManualKeyUp(ManualKey& k) noexcept
    {
        for (auto& c : k.contacts)
        {
            const double delaySec = 0.0001 + nextRand01() * 0.0011;
            c.delayLeft  = static_cast<int>(delaySec * sr_);
            c.target     = 0.0f;
            c.clickArmed = true;
            // Subtle release click (~25% of attack click)
            const float sign = (nextRand01() > 0.5) ? -1.0f : 1.0f;
            c.clickAmp   = sign * 0.014f * static_cast<float>(nextRand01());
        }
    }

    double nextRand01() noexcept
    {
        // Fast xorshift32
        uint32_t x = rngState_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        rngState_ = x;
        return static_cast<double>(x) * (1.0 / 4294967296.0);
    }

    double sr_ = 44100.0;
    float contactCoeff_ = 0.1f;
    float pedalCoeff_   = 0.02f;

    // Key-click bandpass SVF state
    float bpG_   = 0.0f;
    float bpK_   = 0.7f;
    float bpA1_  = 0.0f;
    float bpA2_  = 0.0f;
    float bpA3_  = 0.0f;
    float bpIc1_ = 0.0f;
    float bpIc2_ = 0.0f;

    std::array<ManualKey, kNumManualKeys> upperKeys_ {};
    std::array<ManualKey, kNumManualKeys> lowerKeys_ {};
    std::array<PedalKey,  kNumPedalKeys>  pedalKeys_ {};

    int upperHeldCount_ = 0;
    int lowerHeldCount_ = 0;
    int pedalHeldCount_ = 0;

    uint32_t rngState_ = 0x1962B301u;
};

} // namespace GreenOnions::DSP
