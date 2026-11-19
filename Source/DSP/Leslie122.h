#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <numbers>
#include <vector>

namespace GreenOnions::DSP
{

class Leslie122
{
public:
    enum class SpeedMode : int
    {
        Chorale = 0, // Slow (~40 RPM horn, ~34 RPM drum)
        Brake   = 1, // Stopped (0 RPM)
        Tremolo = 2  // Fast (~400 RPM horn, ~342 RPM drum)
    };

    struct Parameters
    {
        bool      enabled      = true;
        int       cabinetModel = 0;    // 0 = Leslie 122 (Studio/Jazz), 1 = Leslie 147 (Rock/Stage)
        SpeedMode speed        = SpeedMode::Chorale;
        float     micSpread    = 0.72f; // [0..1] Stereo mic angle (narrow -> 180 deg)
        float     micDistance  = 0.35f; // [0..1] Close-miked punch -> roomy cabinet reflections
        float     hornBalance  = 0.50f; // [0..1] Drum-heavy -> balanced (0.5) -> Horn-forward
    };

    struct StereoSample
    {
        float left  = 0.0f;
        float right = 0.0f;
    };

    struct RotorTelemetry
    {
        float hornAngleRad = 0.0f;
        float drumAngleRad = 0.0f;
        float hornRpm      = 40.0f;
        float drumRpm      = 34.0f;
    };

    RotorTelemetry getTelemetry() const noexcept
    {
        return {
            telemetryHornAngle_.load(std::memory_order_relaxed),
            telemetryDrumAngle_.load(std::memory_order_relaxed),
            telemetryHornRpm_.load(std::memory_order_relaxed),
            telemetryDrumRpm_.load(std::memory_order_relaxed)
        };
    }

    void prepare(double sampleRate)
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;

        // 4th-order Linkwitz-Riley crossover at 800 Hz (two cascaded 2nd-order Butterworth stages)
        computeButterworthCoeffs(800.0, false, lpCoeffs_);
        computeButterworthCoeffs(800.0, true,  hpCoeffs_);

        // Delay lines for Doppler modulation (max ~15 ms buffer, power-of-two size)
        const int minSamples = static_cast<int>(0.020 * sr_) + 64;
        delaySize_ = 1;
        while (delaySize_ < minSamples)
            delaySize_ <<= 1;
        delayMask_ = delaySize_ - 1;

        hornDelayBuf_.assign(static_cast<size_t>(delaySize_), 0.0f);
        drumDelayBuf_.assign(static_cast<size_t>(delaySize_), 0.0f);

        // Short cabinet early-reflection buffers (~3.1 ms L, ~4.3 ms R)
        reflLenL_ = std::max(8, static_cast<int>(0.0031 * sr_));
        reflLenR_ = std::max(8, static_cast<int>(0.0043 * sr_));
        reflBufL_.assign(static_cast<size_t>(reflLenL_), 0.0f);
        reflBufR_.assign(static_cast<size_t>(reflLenR_), 0.0f);

        // Independent rotor inertia time constants for Leslie 122 vs Leslie 147
        // 122: Horn 0.75s up / 1.15s down, Drum 4.20s up / 5.60s down
        hornAccel122_ = 1.0f - std::exp(-1.0f / static_cast<float>(0.75 * sr_));
        hornDecel122_ = 1.0f - std::exp(-1.0f / static_cast<float>(1.15 * sr_));
        drumAccel122_ = 1.0f - std::exp(-1.0f / static_cast<float>(4.20 * sr_));
        drumDecel122_ = 1.0f - std::exp(-1.0f / static_cast<float>(5.60 * sr_));

        // 147: Snappier belt tension — Horn 0.55s up / 0.85s down, Drum 3.40s up / 4.50s down
        hornAccel147_ = 1.0f - std::exp(-1.0f / static_cast<float>(0.55 * sr_));
        hornDecel147_ = 1.0f - std::exp(-1.0f / static_cast<float>(0.85 * sr_));
        drumAccel147_ = 1.0f - std::exp(-1.0f / static_cast<float>(3.40 * sr_));
        drumDecel147_ = 1.0f - std::exp(-1.0f / static_cast<float>(4.50 * sr_));

        cachedModel_ = -1;
        updateCabinetVoicing(0);

        reset();
    }

    void reset() noexcept
    {
        lp1_ = lp2_ = hp1_ = hp2_ = BiquadState{};
        hornVoiceState_ = BiquadState{};
        drumVoiceState_ = BiquadState{};
        std::fill(hornDelayBuf_.begin(), hornDelayBuf_.end(), 0.0f);
        std::fill(drumDelayBuf_.begin(), drumDelayBuf_.end(), 0.0f);
        std::fill(reflBufL_.begin(), reflBufL_.end(), 0.0f);
        std::fill(reflBufR_.begin(), reflBufR_.end(), 0.0f);
        delayWritePos_ = 0;
        reflPosL_ = 0;
        reflPosR_ = 0;

        hornBeamLpL_ = 0.0f;
        hornBeamLpR_ = 0.0f;

        hornSpeedHz_ = kHornChoraleHz;
        drumSpeedHz_ = kDrumChoraleHz;
        hornAngle_   = 0.0;
        drumAngle_   = 1.1; // Natural offset between independent shafts

        telemetryHornAngle_.store(static_cast<float>(hornAngle_), std::memory_order_relaxed);
        telemetryDrumAngle_.store(static_cast<float>(drumAngle_), std::memory_order_relaxed);
        telemetryHornRpm_.store(hornSpeedHz_ * 60.0f, std::memory_order_relaxed);
        telemetryDrumRpm_.store(drumSpeedHz_ * 60.0f, std::memory_order_relaxed);
    }

    StereoSample processSample(float monoIn, const Parameters& params) noexcept
    {
        if (!params.enabled)
            return { monoIn, monoIn };

        const int cabModel = (params.cabinetModel == 1) ? 1 : 0;
        if (cabModel != cachedModel_)
            updateCabinetVoicing(cabModel);

        const bool is147 = (cabModel == 1);

        // 1. Update dual-rotor inertia physics (147 ramps faster & spins slightly faster in Tremolo)
        float targetHornHz = kHornChoraleHz;
        float targetDrumHz = kDrumChoraleHz;
        if (params.speed == SpeedMode::Tremolo)
        {
            targetHornHz = is147 ? kHorn147TremoloHz : kHornTremoloHz;
            targetDrumHz = is147 ? kDrum147TremoloHz : kDrumTremoloHz;
        }
        else if (params.speed == SpeedMode::Brake)
        {
            targetHornHz = 0.0f;
            targetDrumHz = 0.0f;
        }

        const float hornCoeff = (targetHornHz > hornSpeedHz_)
            ? (is147 ? hornAccel147_ : hornAccel122_)
            : (is147 ? hornDecel147_ : hornDecel122_);
        const float drumCoeff = (targetDrumHz > drumSpeedHz_)
            ? (is147 ? drumAccel147_ : drumAccel122_)
            : (is147 ? drumDecel147_ : drumDecel122_);

        hornSpeedHz_ += (targetHornHz - hornSpeedHz_) * hornCoeff;
        drumSpeedHz_ += (targetDrumHz - drumSpeedHz_) * drumCoeff;

        constexpr double kTwoPi = 2.0 * std::numbers::pi;
        hornAngle_ += (static_cast<double>(hornSpeedHz_) * kTwoPi) / sr_;
        if (hornAngle_ >= kTwoPi) hornAngle_ -= kTwoPi;

        drumAngle_ += (static_cast<double>(drumSpeedHz_) * kTwoPi) / sr_;
        if (drumAngle_ >= kTwoPi) drumAngle_ -= kTwoPi;

        // 2. 6550 Tube Power Amp Stage (147 runs hotter with earlier grit than 122)
        const float ampGain  = is147 ? 1.42f : 1.05f;
        const float drivenIn = std::tanh(monoIn * ampGain) / (is147 ? 1.18f : 1.02f);

        // 3. 4th-Order Linkwitz-Riley Crossover at 800 Hz + 122/147 Cabinet Horn & Woofer Voicing
        float low  = processBiquad(drivenIn, lpCoeffs_, lp1_);
        low        = processBiquad(low,      lpCoeffs_, lp2_);
        low        = processBiquad(low,      drumVoiceCoeffs_, drumVoiceState_);

        float high = processBiquad(drivenIn, hpCoeffs_, hp1_);
        high       = processBiquad(high,     hpCoeffs_, hp2_);
        high       = processBiquad(high,     hornVoiceCoeffs_, hornVoiceState_);

        // Write split bands into Doppler delay lines
        hornDelayBuf_[static_cast<size_t>(delayWritePos_)] = high;
        drumDelayBuf_[static_cast<size_t>(delayWritePos_)] = low;

        // 3. Virtual Stereo Microphone Geometry
        // micSpread [0..1] -> half-angle from 0.25 rad (~28 deg total) to pi/2 (180 deg total)
        const double micHalfAngle = 0.25 + static_cast<double>(params.micSpread) * (0.5 * std::numbers::pi - 0.25);
        const double hAngleL = hornAngle_ - micHalfAngle;
        const double hAngleR = hornAngle_ + micHalfAngle;
        const double dAngleL = drumAngle_ - micHalfAngle * 0.85;
        const double dAngleR = drumAngle_ + micHalfAngle * 0.85;

        const float cosHL = static_cast<float>(std::cos(hAngleL));
        const float cosHR = static_cast<float>(std::cos(hAngleR));
        const float cosDL = static_cast<float>(std::cos(dAngleL));
        const float cosDR = static_cast<float>(std::cos(dAngleR));

        // Distance factor: closer mics = deeper AM & Doppler; farther mics = smoother + more cabinet reflection
        const float dist = (params.micDistance < 0.0f) ? 0.0f : (params.micDistance > 1.0f ? 1.0f : params.micDistance);
        const float proximity = 1.0f - 0.45f * dist;

        // 4. Horn Doppler Shift (physical radius ~0.19m / 343 m/s = ~0.55 ms peak excursion)
        const float baseHornDelaySamples = static_cast<float>(0.0028 * sr_);
        const float hornDopplerMax       = static_cast<float>(0.00062 * sr_) * proximity;
        const float hornDelayL           = baseHornDelaySamples - cosHL * hornDopplerMax;
        const float hornDelayR           = baseHornDelaySamples - cosHR * hornDopplerMax;

        float hornL = readDelayLinear(hornDelayBuf_, hornDelayL);
        float hornR = readDelayLinear(hornDelayBuf_, hornDelayR);

        // Horn directional cardioid AM + moving off-axis wooden baffle low-pass filter
        // cos = +1 (on-axis): bright (~7.8 kHz) & loud; cos = -1 (off-axis): dark (~1.4 kHz) & softer
        const float hornAmL = 0.62f + 0.38f * proximity * cosHL;
        const float hornAmR = 0.62f + 0.38f * proximity * cosHR;

        const float fcL = 1400.0f + (0.5f * (cosHL + 1.0f)) * 6400.0f;
        const float fcR = 1400.0f + (0.5f * (cosHR + 1.0f)) * 6400.0f;
        const float lpAlphaL = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * fcL / static_cast<float>(sr_));
        const float lpAlphaR = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * fcR / static_cast<float>(sr_));

        hornBeamLpL_ += (hornL * hornAmL - hornBeamLpL_) * lpAlphaL;
        hornBeamLpR_ += (hornR * hornAmR - hornBeamLpR_) * lpAlphaR;
        hornL = hornBeamLpL_;
        hornR = hornBeamLpR_;

        // 5. Bass Drum Doppler Shift (physical radius ~0.14m -> ~0.41 ms) & softer cardioid AM
        const float baseDrumDelaySamples = static_cast<float>(0.0022 * sr_);
        const float drumDopplerMax       = static_cast<float>(0.00042 * sr_) * proximity;
        const float drumDelayL           = baseDrumDelaySamples - cosDL * drumDopplerMax;
        const float drumDelayR           = baseDrumDelaySamples - cosDR * drumDopplerMax;

        const float drumAmL = 0.78f + 0.22f * proximity * cosDL;
        const float drumAmR = 0.78f + 0.22f * proximity * cosDR;

        const float drumL = readDelayLinear(drumDelayBuf_, drumDelayL) * drumAmL;
        const float drumR = readDelayLinear(drumDelayBuf_, drumDelayR) * drumAmR;

        // Advance shared delay write index
        delayWritePos_ = (delayWritePos_ + 1) & delayMask_;

        // 6. Horn / Drum Balance
        const float bal = (params.hornBalance < 0.0f) ? 0.0f : (params.hornBalance > 1.0f ? 1.0f : params.hornBalance);
        const float drumGain = 1.30f * (1.0f - 0.55f * bal);
        const float hornGain = 0.72f + 0.95f * bal;

        float outL = drumL * drumGain + hornL * hornGain;
        float outR = drumR * drumGain + hornR * hornGain;

        // 7. Cabinet Early Reflections (wooden Leslie 122 enclosure warmth & room blend)
        const float reflL = reflBufL_[static_cast<size_t>(reflPosL_)];
        const float reflR = reflBufR_[static_cast<size_t>(reflPosR_)];
        reflBufL_[static_cast<size_t>(reflPosL_)] = outR * 0.45f + outL * 0.25f;
        reflBufR_[static_cast<size_t>(reflPosR_)] = outL * 0.45f + outR * 0.25f;
        if (++reflPosL_ >= reflLenL_) reflPosL_ = 0;
        if (++reflPosR_ >= reflLenR_) reflPosR_ = 0;

        const float reflMix = 0.12f + 0.28f * dist;
        outL = outL * (1.0f - reflMix * 0.35f) + reflL * reflMix;
        outR = outR * (1.0f - reflMix * 0.35f) + reflR * reflMix;

        // Publish lock-free instantaneous rotor angles & RPMs for 60 Hz UI visualizer
        telemetryHornAngle_.store(static_cast<float>(hornAngle_), std::memory_order_relaxed);
        telemetryDrumAngle_.store(static_cast<float>(drumAngle_), std::memory_order_relaxed);
        telemetryHornRpm_.store(hornSpeedHz_ * 60.0f, std::memory_order_relaxed);
        telemetryDrumRpm_.store(drumSpeedHz_ * 60.0f, std::memory_order_relaxed);

        return { outL, outR };
    }

    // Live telemetry for the UI RotorMeter
    float getHornSpeedHz() const noexcept { return hornSpeedHz_; }
    float getDrumSpeedHz() const noexcept { return drumSpeedHz_; }
    float getHornAngle()   const noexcept { return static_cast<float>(hornAngle_); }
    float getDrumAngle()   const noexcept { return static_cast<float>(drumAngle_); }

    static constexpr float kHornChoraleHz    = 40.0f / 60.0f;  // ~0.667 Hz
    static constexpr float kHornTremoloHz    = 400.0f / 60.0f; // ~6.667 Hz (Leslie 122)
    static constexpr float kHorn147TremoloHz = 411.0f / 60.0f; // ~6.850 Hz (Leslie 147)
    static constexpr float kDrumChoraleHz    = 34.0f / 60.0f;  // ~0.567 Hz
    static constexpr float kDrumTremoloHz    = 342.0f / 60.0f; // ~5.700 Hz (Leslie 122)
    static constexpr float kDrum147TremoloHz = 351.0f / 60.0f; // ~5.850 Hz (Leslie 147)

private:
    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
    };

    void updateCabinetVoicing(int model) noexcept
    {
        cachedModel_ = model;
        if (model == 1) // Leslie 147: Punchy 2.8 kHz horn bite + tight 95 Hz bass thump
        {
            hornVoiceCoeffs_ = makePeakCoeffs(2800.0, 1.1, +2.6);
            drumVoiceCoeffs_ = makePeakCoeffs(95.0,   1.2, +2.2);
        }
        else // Leslie 122: Warm studio horn smoothness (-1.5 dB @ 4.5 kHz) + 160 Hz wooden cabinet resonance
        {
            hornVoiceCoeffs_ = makeHighShelfCoeffs(4500.0, 0.707, -1.5);
            drumVoiceCoeffs_ = makePeakCoeffs(160.0,       0.9,   +1.8);
        }
    }

    BiquadCoeffs makePeakCoeffs(double fc, double q, double gainDb) const noexcept
    {
        const double A     = std::pow(10.0, gainDb / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr_ * 0.45) / sr_;
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

    BiquadCoeffs makeHighShelfCoeffs(double fc, double q, double gainDb) const noexcept
    {
        const double A     = std::pow(10.0, gainDb / 40.0);
        const double w0    = 2.0 * std::numbers::pi * std::min(fc, sr_ * 0.45) / sr_;
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

    void computeButterworthCoeffs(double fc, bool highPass, BiquadCoeffs& c) const noexcept
    {
        const double w0    = 2.0 * std::numbers::pi * fc / sr_;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / std::numbers::sqrt2; // Q = 1 / sqrt(2)

        const double a0 = 1.0 + alpha;
        if (!highPass)
        {
            c.b0 = static_cast<float>(((1.0 - cosW0) * 0.5) / a0);
            c.b1 = static_cast<float>((1.0 - cosW0) / a0);
            c.b2 = c.b0;
        }
        else
        {
            c.b0 = static_cast<float>(((1.0 + cosW0) * 0.5) / a0);
            c.b1 = static_cast<float>(-(1.0 + cosW0) / a0);
            c.b2 = c.b0;
        }
        c.a1 = static_cast<float>((-2.0 * cosW0) / a0);
        c.a2 = static_cast<float>((1.0 - alpha) / a0);
    }

    static float processBiquad(float in, const BiquadCoeffs& c, BiquadState& s) noexcept
    {
        const float out = c.b0 * in + s.z1;
        s.z1 = c.b1 * in - c.a1 * out + s.z2;
        s.z2 = c.b2 * in - c.a2 * out;
        return out;
    }

    float readDelayLinear(const std::vector<float>& buf, float delaySamples) const noexcept
    {
        const float readPos = static_cast<float>(delayWritePos_) - delaySamples;
        int i0 = static_cast<int>(std::floor(readPos));
        const float frac = readPos - static_cast<float>(i0);
        i0 &= delayMask_;
        const int i1 = (i0 + 1) & delayMask_;
        return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>(i1)] * frac;
    }

    double sr_ = 44100.0;

    BiquadCoeffs lpCoeffs_ {}, hpCoeffs_ {}, hornVoiceCoeffs_ {}, drumVoiceCoeffs_ {};
    BiquadState  lp1_ {}, lp2_ {}, hp1_ {}, hp2_ {}, hornVoiceState_ {}, drumVoiceState_ {};
    int          cachedModel_ = -1;

    std::vector<float> hornDelayBuf_;
    std::vector<float> drumDelayBuf_;
    int delaySize_     = 1024;
    int delayMask_     = 1023;
    int delayWritePos_ = 0;

    std::vector<float> reflBufL_;
    std::vector<float> reflBufR_;
    int reflLenL_ = 128;
    int reflLenR_ = 192;
    int reflPosL_ = 0;
    int reflPosR_ = 0;

    float hornBeamLpL_ = 0.0f;
    float hornBeamLpR_ = 0.0f;

    float hornAccel122_ = 0.0001f;
    float hornDecel122_ = 0.0001f;
    float drumAccel122_ = 0.00001f;
    float drumDecel122_ = 0.00001f;
    float hornAccel147_ = 0.0001f;
    float hornDecel147_ = 0.0001f;
    float drumAccel147_ = 0.00001f;
    float drumDecel147_ = 0.00001f;

    float  hornSpeedHz_ = kHornChoraleHz;
    float  drumSpeedHz_ = kDrumChoraleHz;
    double hornAngle_   = 0.0;
    double drumAngle_   = 1.1;

    std::atomic<float> telemetryHornAngle_ { 0.0f };
    std::atomic<float> telemetryDrumAngle_ { 1.1f };
    std::atomic<float> telemetryHornRpm_   { 40.0f };
    std::atomic<float> telemetryDrumRpm_   { 34.0f };
};

} // namespace GreenOnions::DSP
