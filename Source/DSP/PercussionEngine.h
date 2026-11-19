#pragma once

#include <cmath>

namespace GreenOnions::DSP
{

class PercussionEngine
{
public:
    struct Settings
    {
        bool enabled       = false;
        bool soft          = false; // false = Normal, true = Soft
        bool fast          = true;  // true = Fast (~150ms), false = Slow (~500ms)
        bool thirdHarmonic = false; // false = 2nd (4'), true = 3rd (2-2/3')
    };

    void prepare(double sampleRate) noexcept
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        // Exponential decay coefficients (time to -40 dB / ~1% amplitude)
        // Fast ~150 ms, Slow ~500 ms
        fastCoeff_ = std::exp(-4.60517f / static_cast<float>(0.150 * sr_));
        slowCoeff_ = std::exp(-4.60517f / static_cast<float>(0.500 * sr_));
        reset();
    }

    void reset() noexcept
    {
        envelope_ = 0.0f;
    }

    // Call ONLY when Upper Manual transitions from 0 held keys -> >= 1 held keys
    void triggerSingle() noexcept
    {
        envelope_ = 1.0f;
    }

    // Advance envelope by 1 sample and return current percussion gain multiplier
    float processSample(const Settings& s) noexcept
    {
        if (!s.enabled)
        {
            envelope_ *= fastCoeff_;
            if (envelope_ < 1e-6f) envelope_ = 0.0f;
            return 0.0f;
        }

        const float current = envelope_;
        const float coeff   = s.fast ? fastCoeff_ : slowCoeff_;
        envelope_ *= coeff;
        if (envelope_ < 1e-6f) envelope_ = 0.0f;

        // Normal is hot & punchy (~0.95), Soft is ~-6 dB (~0.48)
        const float level = s.soft ? 0.48f : 0.95f;
        return current * level;
    }

    // Authentic B-3 detail: engaging Percussion drops the Upper Manual 1' drawbar (busbar #9)
    static bool stealsFirstFootDrawbar(const Settings& s) noexcept
    {
        return s.enabled;
    }

    // Authentic B-3 detail: Normal Percussion dips Upper Manual drawbars by ~3 dB
    static float upperDrawbarAttenuation(const Settings& s) noexcept
    {
        return (s.enabled && !s.soft) ? 0.7071f : 1.0f;
    }

    float getCurrentEnvelope() const noexcept { return envelope_; }

private:
    double sr_        = 44100.0;
    float  fastCoeff_ = 0.999f;
    float  slowCoeff_ = 0.9997f;
    float  envelope_  = 0.0f;
};

} // namespace GreenOnions::DSP
