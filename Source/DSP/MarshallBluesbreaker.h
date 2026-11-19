#pragma once

#include <cmath>
#include <numbers>

namespace GreenOnions::DSP
{

// Parallel Marshall 1962 "Bluesbreaker" 2x12 Combo Amplifier
// Models JTM45 bright-channel tone stack, GZ34 rectifier sag, dual-stage 12AX7 + asymmetric KT66
// push-pull saturation, and open-back 2x12 Celestion G12 Alnico speaker cabinet response.
// Pure C++20 zero-allocation audio-thread DSP.
class MarshallBluesbreaker
{
public:
    void prepare(double sampleRate) noexcept
    {
        fs_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;

        // Tighten sub-75Hz mud before tube distortion (classic JTM45 coupling cap)
        inputHpCoeffs_ = makeHighPass(fs_, 75.0, 0.707);

        // Open-back 2x12 Celestion Alnico cabinet resonance (115 Hz) & speaker rolloff (5.4 kHz)
        cabResCoeffs_     = makePeak(fs_, 115.0, 1.35, +3.2);
        cabRolloffCoeffs_ = makeLowPass(fs_, 5400.0, 0.85);

        reset();
    }

    void reset() noexcept
    {
        inputHpState_    = {};
        midPushState_    = {};
        presenceState_   = {};
        cabResState_     = {};
        cabRolloffState_ = {};
        sagEnvelope_     = 0.0f;
        cachedTone_      = -1.0f;
    }

    // monoIn: Pre-Leslie AO-28 preamp signal
    // drive:  0.0 .. 1.0 (JTM45 / KT66 gain)
    // tone:   0.0 .. 1.0 (Marshall Presence / Treble)
    float processSample(float monoIn, float drive, float tone) noexcept
    {
        const float d = (drive < 0.0f) ? 0.0f : (drive > 1.0f ? 1.0f : drive);
        const float t = (tone  < 0.0f) ? 0.0f : (tone  > 1.0f ? 1.0f : tone);

        if (std::abs(t - cachedTone_) > 0.005f)
        {
            cachedTone_ = t;
            const double midDb  = 1.5 + static_cast<double>(t) * 4.5;  // +1.5 to +6.0 dB @ 850 Hz
            const double presDb = -2.0 + static_cast<double>(t) * 6.5; // -2.0 to +4.5 dB @ 2.8 kHz

            midPushCoeffs_  = makePeak(fs_, 850.0, 0.95, midDb);
            presenceCoeffs_ = makeHighShelf(fs_, 2800.0, 0.707, presDb);
        }

        // 1. Input Coupling & Marshall Tone Stack
        float sig = processBiquad(monoIn, inputHpCoeffs_,  inputHpState_);
        sig       = processBiquad(sig,    midPushCoeffs_,  midPushState_);
        sig       = processBiquad(sig,    presenceCoeffs_, presenceState_);

        // 2. GZ34 Rectifier Sag & Dual-Stage 12AX7 -> Asymmetric KT66 Saturation
        const float absIn    = std::abs(sig);
        const float sagCoeff = (absIn > sagEnvelope_) ? 0.002f : 0.00015f;
        sagEnvelope_ += (absIn - sagEnvelope_) * sagCoeff;

        const float preGain = (1.6f + d * 11.5f) / (1.0f + sagEnvelope_ * 1.4f);
        const float stage1  = std::tanh(sig * preGain);

        // Asymmetric KT66 push-pull stage for singing harmonic sustain
        const float stage2 = (stage1 >= 0.0f)
            ? std::tanh(stage1 * 1.45f)
            : std::tanh(stage1 * 1.15f) * 1.06f;

        // 3. Open-Back 2x12 Celestion Alnico Cabinet Filter
        float cabOut = processBiquad(stage2, cabResCoeffs_,     cabResState_);
        cabOut       = processBiquad(cabOut, cabRolloffCoeffs_, cabRolloffState_);

        // Level-compensated output
        return cabOut * (0.55f / (1.0f + d * 0.35f));
    }

private:
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

    static BiquadCoeffs makeHighPass(double sr, double fc, double q) noexcept
    {
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * q);
        const double a0    = 1.0 + alpha;
        BiquadCoeffs c;
        c.b0 = static_cast<float>(((1.0 + cosW0) * 0.5) / a0);
        c.b1 = static_cast<float>(-(1.0 + cosW0) / a0);
        c.b2 = c.b0;
        c.a1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.a2 = static_cast<float>((1.0 - alpha) / a0);
        return c;
    }

    static BiquadCoeffs makeLowPass(double sr, double fc, double q) noexcept
    {
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * q);
        const double a0    = 1.0 + alpha;
        BiquadCoeffs c;
        c.b0 = static_cast<float>(((1.0 - cosW0) * 0.5) / a0);
        c.b1 = static_cast<float>((1.0 - cosW0) / a0);
        c.b2 = c.b0;
        c.a1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.a2 = static_cast<float>((1.0 - alpha) / a0);
        return c;
    }

    static BiquadCoeffs makePeak(double sr, double fc, double q, double gainDb) noexcept
    {
        const double A     = std::pow(10.0, gainDb / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * q);
        const double a0    = 1.0 + alpha / A;
        BiquadCoeffs c;
        c.b0 = static_cast<float>((1.0 + alpha * A) / a0);
        c.b1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.b2 = static_cast<float>((1.0 - alpha * A) / a0);
        c.a1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.a2 = static_cast<float>((1.0 - alpha / A) / a0);
        return c;
    }

    static BiquadCoeffs makeHighShelf(double sr, double fc, double q, double gainDb) noexcept
    {
        const double A     = std::pow(10.0, gainDb / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr * 0.45) / sr;
        const double cosW0 = std::cos(w0);
        const double alpha = std::sin(w0) / (2.0 * q);
        const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;
        const double a0    = (A + 1.0) - (A - 1.0) * cosW0 + twoSqrtAAlpha;
        BiquadCoeffs c;
        c.b0 = static_cast<float>((A * ((A + 1.0) + (A - 1.0) * cosW0 + twoSqrtAAlpha)) / a0);
        c.b1 = static_cast<float>((-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0)) / a0);
        c.b2 = static_cast<float>((A * ((A + 1.0) + (A - 1.0) * cosW0 - twoSqrtAAlpha)) / a0);
        c.a1 = static_cast<float>((2.0 * ((A - 1.0) - (A + 1.0) * cosW0)) / a0);
        c.a2 = static_cast<float>(((A + 1.0) - (A - 1.0) * cosW0 - twoSqrtAAlpha) / a0);
        return c;
    }

    double fs_         = 44100.0;
    float  cachedTone_ = -1.0f;
    float  sagEnvelope_ = 0.0f;

    BiquadCoeffs inputHpCoeffs_ {}, midPushCoeffs_ {}, presenceCoeffs_ {}, cabResCoeffs_ {}, cabRolloffCoeffs_ {};
    BiquadState  inputHpState_  {}, midPushState_  {}, presenceState_  {}, cabResState_  {}, cabRolloffState_  {};
};

} // namespace GreenOnions::DSP
