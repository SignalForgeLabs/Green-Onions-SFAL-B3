#pragma once

#include <array>
#include <cmath>
#include <numbers>

namespace GreenOnions::DSP
{

// Zero-allocation 3-band cascaded biquad EQ modeling 4 iconic Leslie studio microphones
// with distance-coupled inverse-square proximity effect.
class LeslieMicChannelEQ
{
public:
    void prepare(double sampleRate) noexcept
    {
        fs_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        reset();
        forceUpdate_ = true;
    }

    void reset() noexcept
    {
        proxState_ = {};
        presState_ = {};
        airState_  = {};
    }

    // micType: 0 = Neumann U47, 1 = AKG C414, 2 = Royer R-121, 3 = Shure SM57
    // radialDistNorm: 0.0 (close-miked louvers) .. 1.0 (distant room mic)
    float processSample(float input, int micType, float radialDistNorm) noexcept
    {
        const int clampedType = (micType < 0) ? 0 : (micType > 3 ? 3 : micType);
        const float clampedDist = (radialDistNorm < 0.0f) ? 0.0f : (radialDistNorm > 1.0f ? 1.0f : radialDistNorm);

        if (forceUpdate_ || clampedType != cachedType_ || std::abs(clampedDist - cachedDist_) > 0.005f)
        {
            cachedType_  = clampedType;
            cachedDist_  = clampedDist;
            forceUpdate_ = false;
            updateCoefficients();
        }

        float sig = processBiquad(input, proxCoeffs_, proxState_);
        sig       = processBiquad(sig,   presCoeffs_, presState_);
        return      processBiquad(sig,   airCoeffs_,  airState_);
    }

private:
    struct MicCurveSpec
    {
        float proxFreq;
        float maxProxBoostDb;
        float presFreq;
        float presGainDb;
        float presQ;
        float airFreq;
        float airGainDb;
    };

    static constexpr std::array<MicCurveSpec, 4> kMicCurves = {{
        // 0: Neumann U47 (Warm tube low-end, rich 2.2 kHz body, gentle top rolloff)
        { 110.0f, 4.5f, 2200.0f, +1.5f, 0.9f, 9500.0f, -1.5f },
        // 1: AKG C414 (Controlled low-end, flat mids, crisp 9 kHz air)
        { 100.0f, 2.5f, 2500.0f,  0.0f, 0.8f, 9000.0f, +3.5f },
        // 2: Royer R-121 Ribbon (Heavy figure-8 proximity boom, smooth 3.5 kHz dip, dark ribbon top)
        {  95.0f, 6.5f, 3500.0f, -2.0f, 1.1f, 8000.0f, -4.5f },
        // 3: Shure SM57 (Tight 140 Hz punch, aggressive 3.2 kHz rock horn bite, steep dynamic rolloff)
        { 140.0f, 3.0f, 3200.0f, +4.0f, 1.3f, 7000.0f, -5.5f }
    }};

    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
    };

    static float processBiquad(float in, const BiquadCoeffs& c, BiquadState& s) noexcept
    {
        const float out = c.b0 * in + s.z1;
        s.z1 = c.b1 * in - c.a1 * out + s.z2;
        s.z2 = c.b2 * in - c.a2 * out;
        return out;
    }

    static BiquadCoeffs makeLowShelf(double sr, float fc, float q, float gainDb) noexcept
    {
        const double A     = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(static_cast<double>(fc), sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * static_cast<double>(q));
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;

        const double a0 = (A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha;
        BiquadCoeffs c;
        c.b0 = static_cast<float>((A * ((A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha)) / a0);
        c.b1 = static_cast<float>((2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0)) / a0);
        c.b2 = static_cast<float>((A * ((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha)) / a0);
        c.a1 = static_cast<float>((-2.0 * ((A - 1.0) + (A + 1.0) * cosW0)) / a0);
        c.a2 = static_cast<float>(((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
        return c;
    }

    static BiquadCoeffs makeHighShelf(double sr, float fc, float q, float gainDb) noexcept
    {
        const double A     = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(static_cast<double>(fc), sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * static_cast<double>(q));
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;

        const double a0 = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
        BiquadCoeffs c;
        c.b0 = static_cast<float>((A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha)) / a0);
        c.b1 = static_cast<float>((-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0)) / a0);
        c.b2 = static_cast<float>((A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha)) / a0);
        c.a1 = static_cast<float>((2.0 * ((A - 1.0) - (A + 1.0) * cosW0)) / a0);
        c.a2 = static_cast<float>(((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
        return c;
    }

    static BiquadCoeffs makePeakFilter(double sr, float fc, float q, float gainDb) noexcept
    {
        const double A     = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(static_cast<double>(fc), sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * static_cast<double>(q));

        const double a0 = 1.0 + alpha / A;
        BiquadCoeffs c;
        c.b0 = static_cast<float>((1.0 + alpha * A) / a0);
        c.b1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.b2 = static_cast<float>((1.0 - alpha * A) / a0);
        c.a1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.a2 = static_cast<float>((1.0 - alpha / A) / a0);
        return c;
    }

    void updateCoefficients() noexcept
    {
        const auto& spec = kMicCurves[static_cast<size_t>(cachedType_)];

        // Proximity effect follows an inverse-square rise as the mic approaches the cabinet louvers
        const float closeness = 1.0f - cachedDist_;
        const float proxDb    = spec.maxProxBoostDb * (closeness * closeness);

        proxCoeffs_ = makeLowShelf(fs_,   spec.proxFreq, 0.707f,    proxDb);
        presCoeffs_ = makePeakFilter(fs_, spec.presFreq, spec.presQ, spec.presGainDb);
        airCoeffs_  = makeHighShelf(fs_,  spec.airFreq,  0.707f,    spec.airGainDb);
    }

    double fs_         = 44100.0;
    int    cachedType_ = -1;
    float  cachedDist_ = -1.0f;
    bool   forceUpdate_ = true;

    BiquadCoeffs proxCoeffs_ {}, presCoeffs_ {}, airCoeffs_ {};
    BiquadState  proxState_  {}, presState_  {}, airState_  {};
};

} // namespace GreenOnions::DSP
