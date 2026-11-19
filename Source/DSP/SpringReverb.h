#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace GreenOnions::DSP
{

class SpringReverb
{
public:
    static constexpr int kNumSprings    = 3;
    static constexpr int kDispersionAPs = 4;

    void prepare(double sampleRate)
    {
        sr_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;

        // Input HPF (~140 Hz) to keep 16' pedal bass tight out of the spring tank
        inHpCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 140.0f / static_cast<float>(sr_));
        // Feedback damping LPF (~3.5 kHz) and output pickup LPF (~4.8 kHz)
        dampLpCoeff_ = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 3500.0f / static_cast<float>(sr_));
        outLpCoeff_  = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * 4800.0f / static_cast<float>(sr_));

        // 3 Accutronics-style spring delay lengths in milliseconds (mutually prime ratios)
        constexpr std::array<double, kNumSprings> kSpringMs = { 29.7, 37.1, 43.3 };
        // Short dispersion all-pass lengths in ms per spring
        constexpr std::array<std::array<double, kDispersionAPs>, kNumSprings> kApMs = {{
            { 1.7, 2.3, 3.1, 4.7 },
            { 1.9, 2.9, 3.7, 5.1 },
            { 2.1, 3.3, 4.1, 5.3 }
        }};

        for (int s = 0; s < kNumSprings; ++s)
        {
            const size_t idx = static_cast<size_t>(s);
            const int len = std::max(8, static_cast<int>(kSpringMs[idx] * 0.001 * sr_));
            springs_[idx].delayBuf.assign(static_cast<size_t>(len), 0.0f);
            springs_[idx].writePos = 0;
            springs_[idx].dampState = 0.0f;

            for (int a = 0; a < kDispersionAPs; ++a)
            {
                const int apLen = std::max(4, static_cast<int>(kApMs[idx][static_cast<size_t>(a)] * 0.001 * sr_));
                springs_[idx].ap[static_cast<size_t>(a)].buf.assign(static_cast<size_t>(apLen), 0.0f);
                springs_[idx].ap[static_cast<size_t>(a)].pos = 0;
                // Slightly different feedback gains per AP stage for authentic spring chirp
                springs_[idx].ap[static_cast<size_t>(a)].gain = 0.52f + 0.04f * static_cast<float>(a);
            }
        }

        // Cross-coupling shimmer all-pass at output
        const int postLen = std::max(8, static_cast<int>(0.0073 * sr_));
        postAp_.buf.assign(static_cast<size_t>(postLen), 0.0f);
        postAp_.pos  = 0;
        postAp_.gain = 0.55f;

        reset();
    }

    void reset() noexcept
    {
        inHpState_  = 0.0f;
        outLpState_ = 0.0f;
        for (auto& s : springs_)
        {
            std::fill(s.delayBuf.begin(), s.delayBuf.end(), 0.0f);
            s.writePos  = 0;
            s.dampState = 0.0f;
            for (auto& ap : s.ap)
            {
                std::fill(ap.buf.begin(), ap.buf.end(), 0.0f);
                ap.pos = 0;
            }
        }
        std::fill(postAp_.buf.begin(), postAp_.buf.end(), 0.0f);
        postAp_.pos = 0;
    }

    float processSample(float input, float mix, float decay) noexcept
    {
        if (mix <= 1e-4f)
            return input;

        // Input high-pass to emulate spring driver coil
        inHpState_ += (input - inHpState_) * inHpCoeff_;
        const float driveIn = input - inHpState_;

        // Feedback gain mapped smoothly from decay [0..1] -> [0.35 .. 0.84]
        const float clampedDecay = (decay < 0.0f) ? 0.0f : (decay > 1.0f ? 1.0f : decay);
        const float fbGain = 0.35f + clampedDecay * 0.49f;

        float wetSum = 0.0f;
        for (int s = 0; s < kNumSprings; ++s)
        {
            auto& sp = springs_[static_cast<size_t>(s)];
            const float delayed = sp.delayBuf[static_cast<size_t>(sp.writePos)];

            // Damping LPF inside the spring feedback loop
            sp.dampState += (delayed - sp.dampState) * dampLpCoeff_;

            // Feed input + damped feedback into the 4-stage dispersive all-pass chain
            float x = driveIn * 0.42f + sp.dampState * fbGain;
            // Subtle transducer soft limit prevents runaway resonances
            x = std::tanh(x);

            for (int a = 0; a < kDispersionAPs; ++a)
                x = sp.ap[static_cast<size_t>(a)].process(x);

            sp.delayBuf[static_cast<size_t>(sp.writePos)] = x;
            if (++sp.writePos >= static_cast<int>(sp.delayBuf.size()))
                sp.writePos = 0;

            // Alternate polarity slightly across the 3 springs for lush density
            wetSum += (s == 1) ? -delayed : delayed;
        }

        // Output pickup diffusion + warmth LPF
        float wet = postAp_.process(wetSum * 0.55f);
        outLpState_ += (wet - outLpState_) * outLpCoeff_;
        wet = outLpState_;

        const float m = (mix < 0.0f) ? 0.0f : (mix > 1.0f ? 1.0f : mix);
        return input * (1.0f - m * 0.35f) + wet * (m * 0.85f);
    }

private:
    struct AllPass
    {
        std::vector<float> buf;
        int   pos  = 0;
        float gain = 0.5f;

        float process(float in) noexcept
        {
            const float delayed = buf[static_cast<size_t>(pos)];
            const float v       = in - gain * delayed;
            buf[static_cast<size_t>(pos)] = v;
            if (++pos >= static_cast<int>(buf.size()))
                pos = 0;
            return delayed + gain * v;
        }
    };

    struct SpringLine
    {
        std::vector<float> delayBuf;
        int   writePos  = 0;
        float dampState = 0.0f;
        std::array<AllPass, kDispersionAPs> ap {};
    };

    double sr_          = 44100.0;
    float  inHpCoeff_   = 0.02f;
    float  dampLpCoeff_ = 0.35f;
    float  outLpCoeff_  = 0.45f;
    float  inHpState_   = 0.0f;
    float  outLpState_  = 0.0f;

    std::array<SpringLine, kNumSprings> springs_ {};
    AllPass postAp_ {};
};

} // namespace GreenOnions::DSP
