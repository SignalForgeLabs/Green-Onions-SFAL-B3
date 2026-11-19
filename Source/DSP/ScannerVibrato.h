#pragma once

#include <array>
#include <cmath>
#include <numbers>

namespace GreenOnions::DSP
{

class ScannerVibrato
{
public:
    enum class Mode : int
    {
        Off = 0,
        V1  = 1,
        C1  = 2,
        V2  = 3,
        C2  = 4,
        V3  = 5,
        C3  = 6
    };

    static constexpr int kNumStages = 18;

    void prepare(double sampleRate) noexcept
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        // Authentic Hammond B-3 scanner rotates at ~6.9 Hz (414 RPM)
        lfoPhaseInc_ = (2.0 * std::numbers::pi * 6.90) / sr_;
        // Line-box gentle warmth low-pass (~6.5 kHz, modeling LC line-box inductance)
        lpCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 6500.0f / static_cast<float>(sr_));
        reset();
    }

    void reset() noexcept
    {
        apX1_.fill(0.0f);
        apY1_.fill(0.0f);
        lpState_ = 0.0f;
    }

    float processSample(float input, Mode mode) noexcept
    {
        // Always keep scanner LFO spinning continuously (mechanical shaft driven by main motor)
        lfoPhase_ += lfoPhaseInc_;
        if (lfoPhase_ >= 2.0 * std::numbers::pi)
            lfoPhase_ -= 2.0 * std::numbers::pi;

        if (mode == Mode::Off)
            return input;

        // Tri-phase capacitive scanner waveform:
        // Subtle 3rd-harmonic shaping mimics the mechanical scanner plates sweeping across the 9 taps
        const float lfo = static_cast<float>(0.88 * std::sin(lfoPhase_) + 0.12 * std::sin(3.0 * lfoPhase_));

        // Select modulation depth & wet/dry blend for V1-V3 / C1-C3
        float depth = 0.0f;
        bool isChorus = false;
        switch (mode)
        {
            case Mode::Off: return input;
            case Mode::V1: depth = 0.24f; isChorus = false; break;
            case Mode::C1: depth = 0.24f; isChorus = true;  break;
            case Mode::V2: depth = 0.48f; isChorus = false; break;
            case Mode::C2: depth = 0.48f; isChorus = true;  break;
            case Mode::V3: depth = 0.78f; isChorus = false; break;
            case Mode::C3: depth = 0.78f; isChorus = true;  break;
            default: return input;
        }

        // Sweep the 18-stage all-pass break frequency around 3400 Hz
        // Sample-rate invariant group delay modulation
        const double centerFreq = 3400.0;
        const double modFreq    = centerFreq * std::pow(2.0, -static_cast<double>(lfo * depth * 0.62f));
        const double clampedF   = (modFreq < 500.0) ? 500.0 : (modFreq > sr_ * 0.42 ? sr_ * 0.42 : modFreq);
        const double t          = std::tan(std::numbers::pi * clampedF / sr_);
        const float  a          = static_cast<float>((t - 1.0) / (t + 1.0));

        // Cascade 18 first-order all-pass stages
        float sig = input;
        for (int s = 0; s < kNumStages; ++s)
        {
            const size_t idx = static_cast<size_t>(s);
            const float x = sig;
            const float y = a * (x - apY1_[idx]) + apX1_[idx];
            apX1_[idx] = x;
            apY1_[idx] = y;
            sig = y;
        }

        // Subtle LC line-box high-frequency warmth
        lpState_ += (sig - lpState_) * lpCoeff_;
        const float wet = lpState_;

        if (isChorus)
            return 0.54f * input + 0.54f * wet;

        return wet;
    }

private:
    double sr_          = 44100.0;
    double lfoPhase_    = 0.0;
    double lfoPhaseInc_ = 0.0;
    float  lpCoeff_     = 0.5f;
    float  lpState_     = 0.0f;

    std::array<float, kNumStages> apX1_ {};
    std::array<float, kNumStages> apY1_ {};
};

} // namespace GreenOnions::DSP
