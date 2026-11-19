#pragma once

#include <cmath>
#include <numbers>

namespace GreenOnions::DSP
{

class PreampSwell
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;

        // ~15 ms zipper-free smoothing for expression pedal and drive
        smoothCoeff_ = 1.0f - std::exp(-1.0f / static_cast<float>(0.015 * sr_));

        // 180 Hz low-shelf split for authentic B-3 swell-box loudness contour
        bassSplitCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 180.0f / static_cast<float>(sr_));

        // 20 Hz DC blocker after asymmetric tube bias
        dcBlockCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 20.0f / static_cast<float>(sr_));

        // Gentle tube-amp transformer HF roll-off (~9.5 kHz)
        postLpCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 9500.0f / static_cast<float>(sr_));

        reset();
    }

    void reset() noexcept
    {
        swellSmooth_   = 0.85f;
        driveSmooth_   = 0.25f;
        bassLpState_   = 0.0f;
        dcState1_      = 0.0f;
        dcState2_      = 0.0f;
        postLpState_   = 0.0f;
    }

    // swell: [0..1] (CC 11 Expression Pedal), drive: [0..1] (AO-28 / 122 tube saturation),
    // preampBiasBoost: console voicing multiplier (0.65 = clean C-3 .. 1.60 = gritty road-worn chop)
    float processSample(float input, float swell, float drive, float preampBiasBoost = 1.0f) noexcept
    {
        swellSmooth_ += (swell - swellSmooth_) * smoothCoeff_;
        driveSmooth_ += (drive - driveSmooth_) * smoothCoeff_;

        // 1. Variable-Capacitor Swell Pedal Loudness Contour
        // Real B-3 swell pedal spans roughly -26 dB to 0 dB, boosting bass relative to mids at low volumes.
        const float s = (swellSmooth_ < 0.0f) ? 0.0f : (swellSmooth_ > 1.0f ? 1.0f : swellSmooth_);
        const float midHighGain = 0.055f + 0.945f * (s * s);          // Exponential open-up of mids/highs
        const float bassBoost   = 1.0f + (1.0f - s) * 1.45f;          // Up to ~+7.8 dB relative bass warmth at heel-down
        const float bassGain    = midHighGain * bassBoost;

        bassLpState_ += (input - bassLpState_) * bassSplitCoeff_;
        const float lowBand  = bassLpState_;
        const float highBand = input - lowBand;

        float sig = lowBand * bassGain + highBand * midHighGain;

        // 2. AO-28 Tube Preamp Stage 1 (6AU6 / 12AX7 warm asymmetric triode)
        const float effDrive = driveSmooth_ * preampBiasBoost;
        const float preGain  = 1.15f + effDrive * 5.2f;
        const float driven1  = sig * preGain;
        // Subtle asymmetric bias creates rich 2nd-harmonic tube warmth
        const float bias1    = 0.08f * effDrive;
        float stage1 = std::tanh(driven1 + bias1) - std::tanh(bias1);

        // Remove induced DC offset
        dcState1_ += (stage1 - dcState1_) * dcBlockCoeff_;
        stage1 -= dcState1_;

        // 3. Stage 2 (12AX7 driver / Leslie 122 6550 push-pull warmth)
        const float drive2 = 1.0f + effDrive * 2.4f;
        const float x2     = stage1 * drive2;
        // Soft asymmetric clipping curve
        float stage2 = (x2 >= 0.0f)
            ? std::tanh(x2 * 1.05f)
            : std::tanh(x2 * 0.92f) * 1.04f;

        dcState2_ += (stage2 - dcState2_) * dcBlockCoeff_;
        stage2 -= dcState2_;

        // Output transformer gentle HF smoothing + level compensation
        postLpState_ += (stage2 - postLpState_) * postLpCoeff_;
        const float makeup = 1.0f / (1.0f + effDrive * 0.55f);

        return postLpState_ * makeup;
    }

private:
    double sr_             = 44100.0;
    float  smoothCoeff_    = 0.01f;
    float  bassSplitCoeff_ = 0.02f;
    float  dcBlockCoeff_   = 0.002f;
    float  postLpCoeff_    = 0.5f;

    float  swellSmooth_    = 0.85f;
    float  driveSmooth_    = 0.25f;
    float  bassLpState_    = 0.0f;
    float  dcState1_       = 0.0f;
    float  dcState2_       = 0.0f;
    float  postLpState_    = 0.0f;
};

} // namespace GreenOnions::DSP
