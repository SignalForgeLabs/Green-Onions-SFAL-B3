#include "DSP/TonewheelEngine.h"
#include "DSP/ScannerVibrato.h"
#include "DSP/PreampSwell.h"
#include "DSP/SpringReverb.h"
#include "DSP/Leslie122.h"
#include "DSP/LeslieMicEQ.h"
#include "DSP/MarshallBluesbreaker.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace GreenOnions::DSP;

static bool approxEqual(double a, double b, double tol)
{
    return std::abs(a - b) <= tol;
}

int main()
{
    std::cout << "Running Green Onions (Classic ToneWheels) DSP Tests...\n";

    // 1. Gear Ratios & Foldback Table
    {
        TonewheelTable tbl;
        // Wheel 45 (0-indexed) = A4 (C1=0, C2=12, C3=24, C4=36, A4=36+9=45)
        assert(approxEqual(tbl.frequencies[45], 440.0, 1e-6));
        // C4 = wheel 36 = 320 * 85 / 104 = 261.5384615 Hz
        assert(approxEqual(tbl.frequencies[36], 320.0 * 85.0 / 104.0, 1e-6));
        // Bottom foldback: Key 0 (C2), 16' drawbar (idx 0) must fold up to wheel 12 (C2)
        assert(tbl.manualWheelIndex[0][0] == 12);
        // Key 12 (C3), 16' drawbar -> wheel 12 (C2), no foldback needed
        assert(tbl.manualWheelIndex[12][0] == 12);
        // Top foldback: Key 60 (C7, wheel 72), 1' drawbar (+36 -> 108) must fold down into [79..90] -> 84 (C8)
        assert(tbl.manualWheelIndex[60][8] == 84);
        assert(tbl.manualWheelIndex[60][8] < kNumTonewheels);
        // Pedals: C2 (idx 0) 16' -> wheel 0 (C1), 8' -> wheel 12 (C2)
        assert(tbl.pedalWheelIndex[0][0] == 0);
        assert(tbl.pedalWheelIndex[0][1] == 12);
        std::cout << "  [PASS] Gear ratios & foldback LUT\n";
    }

    // 2. Free-Running Tonewheels & Key Click
    {
        TonewheelEngine engine;
        engine.prepare(48000.0);
        TonewheelEngine::Parameters p;
        p.upperDrawbars = { 8, 8, 8, 8, 0, 0, 0, 0, 0 };

        // Run 500 samples idle -> phases must advance continuously
        const double ph0 = engine.getWheelPhase(36);
        for (int i = 0; i < 500; ++i)
            (void) engine.processSample(p);
        const double ph1 = engine.getWheelPhase(36);
        assert(std::abs(ph1 - ph0) > 1e-3);

        // Trigger note -> phase must NOT reset to 0
        engine.noteOnUpper(60, 0.9f);
        const double phAfterNote = engine.getWheelPhase(36);
        assert(approxEqual(phAfterNote, ph1, 1e-9));

        float peak = 0.0f;
        for (int i = 0; i < 2400; ++i)
        {
            auto out = engine.processSample(p);
            const float s = out.toVibrato + out.dryBypass;
            assert(std::isfinite(s));
            peak = std::max(peak, std::abs(s));
        }
        assert(peak > 0.05f);
        std::cout << "  [PASS] Free-running 91 tonewheels & key matrix (peak=" << peak << ")\n";
    }

    // 3. Monophonic Single-Trigger Percussion & 1' Drawbar Steal
    {
        PercussionEngine perc;
        perc.prepare(48000.0);
        PercussionEngine::Settings s;
        s.enabled = true;
        s.fast    = true;
        s.soft    = false;

        assert(PercussionEngine::stealsFirstFootDrawbar(s));
        assert(approxEqual(PercussionEngine::upperDrawbarAttenuation(s), 0.7071, 1e-3));

        perc.triggerSingle();
        const float startAmp = perc.processSample(s);
        assert(startAmp > 0.9f);

        // After ~150ms (7200 samples), fast decay should drop below 5%
        float endAmp = startAmp;
        for (int i = 0; i < 7200; ++i)
            endAmp = perc.processSample(s);
        assert(endAmp < 0.05f);

        // Verify legato does not re-trigger in KeyMatrix
        KeyMatrix km;
        km.prepare(48000.0);
        assert(km.noteOnUpper(24, 0.8f) == true);  // 0 -> 1 keys: triggers!
        assert(km.noteOnUpper(28, 0.8f) == false); // 1 -> 2 keys (legato): does NOT trigger!
        km.noteOffUpper(24);
        km.noteOffUpper(28);
        assert(km.noteOnUpper(31, 0.8f) == true);  // 0 -> 1 keys again: triggers!
        std::cout << "  [PASS] Single-trigger percussion & 1' drawbar steal\n";
    }

    // 4. Full Signal Chain Integration (Tonewheel -> Scanner C3 -> AO-28 -> Spring -> Leslie 122)
    {
        constexpr double sr = 48000.0;
        TonewheelEngine engine;
        ScannerVibrato  scanner;
        PreampSwell     preamp;
        SpringReverb    reverb;
        Leslie122       leslie;

        engine.prepare(sr);
        scanner.prepare(sr);
        preamp.prepare(sr);
        reverb.prepare(sr);
        leslie.prepare(sr);

        TonewheelEngine::Parameters tp;
        tp.upperDrawbars = { 8, 8, 8, 8, 0, 0, 0, 0, 0 }; // Classic Green Onions
        tp.lowerDrawbars = { 0, 0, 8, 8, 8, 0, 0, 0, 0 };
        tp.pedalDrawbars = { 7, 4 };
        tp.percussion.enabled = true;
        tp.percussion.thirdHarmonic = true;

        Leslie122::Parameters lp;
        lp.enabled     = true;
        lp.speed       = Leslie122::SpeedMode::Tremolo; // Ramp from Chorale -> Tremolo
        lp.micSpread   = 0.75f;
        lp.micDistance = 0.35f;

        // Play an F minor 7 chord + bass pedal F2
        engine.noteOnUpper(65, 0.9f);
        engine.noteOnUpper(68, 0.85f);
        engine.noteOnUpper(72, 0.85f);
        engine.noteOnLower(53, 0.75f);
        engine.noteOnPedal(41, 0.9f);

        float maxL = 0.0f, maxR = 0.0f, stereoDiff = 0.0f;
        float hornSpeedAt1s = 0.0f, drumSpeedAt1s = 0.0f;

        for (int i = 0; i < 96000; ++i) // 2.0 seconds
        {
            auto raw   = engine.processSample(tp);
            float vib  = scanner.processSample(raw.toVibrato, ScannerVibrato::Mode::C3);
            float mono = preamp.processSample(vib + raw.dryBypass, 0.85f, 0.45f);
            mono       = reverb.processSample(mono, 0.30f, 0.55f);
            auto st    = leslie.processSample(mono, lp);

            assert(std::isfinite(st.left) && std::isfinite(st.right));
            maxL = std::max(maxL, std::abs(st.left));
            maxR = std::max(maxR, std::abs(st.right));
            stereoDiff = std::max(stereoDiff, std::abs(st.left - st.right));

            if (i == 48000)
            {
                hornSpeedAt1s = leslie.getHornSpeedHz();
                drumSpeedAt1s = leslie.getDrumSpeedHz();
            }
        }

        // At 1.0 second into Tremolo ramp:
        // Horn (0.7s tau) should already be > 5.0 Hz, while heavy Bass Drum (4.5s tau) is still < 2.5 Hz!
        assert(hornSpeedAt1s > 4.8f);
        assert(drumSpeedAt1s < 2.5f);
        assert(maxL > 0.1f && maxR > 0.1f);
        assert(stereoDiff > 0.05f); // Genuine stereo rotary image!

        std::cout << "  [PASS] Full B-3 + Leslie 122 chain (1s horn=" << hornSpeedAt1s
                  << " Hz, drum=" << drumSpeedAt1s << " Hz, peakL=" << maxL
                  << ", stereoDiff=" << stereoDiff << ")\n";
    }

    // 5. "Load Your Organ" Console Voicing Profiles (Wax-Cap vs Red-Cap vs Sanctuary vs Chop)
    {
        const auto wax1955 = buildWheelCalibrationTable(0); // +2.8 dB low, -3.5 dB high
        const auto red1968 = buildWheelCalibrationTable(2); // -0.5 dB low, +3.2 dB high

        // Low tonewheels (wheel 0 = C1) must be hotter on 1955 Wax-Cap than 1968 Red-Cap
        assert(wax1955[0] > red1968[0] * 1.25f);
        // High tonewheels (wheel 90 = F#8) must be significantly brighter on 1968 Red-Cap than 1955 Wax-Cap
        assert(red1968[90] > wax1955[90] * 1.80f);
        // 1972 Road-Worn Chop must have higher crosstalk & preamp bias than 1962 Sanctuary C-3
        assert(getOrganProfile(4).crosstalkScale > getOrganProfile(3).crosstalkScale * 3.0f);
        assert(getOrganProfile(4).preampBiasBoost > getOrganProfile(3).preampBiasBoost * 2.0f);

        std::cout << "  [PASS] Load Your Organ 5 console voicing profiles (wax90=" << wax1955[90]
                  << ", red90=" << red1968[90] << ")\n";
    }

    // 6. Distance-Coupled Studio Mic Modeling (U47 / C414 / R-121 / SM57)
    {
        constexpr double sr = 48000.0;
        LeslieMicChannelEQ micClose, micFar, micC414, micR121;
        micClose.prepare(sr);
        micFar.prepare(sr);
        micC414.prepare(sr);
        micR121.prepare(sr);

        // 70 Hz bass tone: R-121 close-miked (dist=0.0) must have strong proximity boom vs far (dist=1.0)
        float rmsClose = 0.0f, rmsFar = 0.0f;
        for (int i = 0; i < 4800; ++i)
        {
            const float in = std::sin(2.0f * std::numbers::pi_v<float> * 70.0f * static_cast<float>(i) / static_cast<float>(sr));
            const float yClose = micClose.processSample(in, 2, 0.0f);
            const float yFar   = micFar.processSample(in,   2, 1.0f);
            if (i > 1000)
            {
                rmsClose += yClose * yClose;
                rmsFar   += yFar   * yFar;
            }
        }
        assert(rmsClose > rmsFar * 2.5f); // +6.5 dB proximity boost (~4.4x power)

        // 10 kHz air tone: AKG C414 (+3.5 dB air) must be much brighter than Royer R-121 (-4.5 dB ribbon)
        float rmsC414 = 0.0f, rmsR121 = 0.0f;
        for (int i = 0; i < 4800; ++i)
        {
            const float in = std::sin(2.0f * std::numbers::pi_v<float> * 10000.0f * static_cast<float>(i) / static_cast<float>(sr));
            const float yC = micC414.processSample(in, 1, 0.5f);
            const float yR = micR121.processSample(in, 2, 0.5f);
            if (i > 1000)
            {
                rmsC414 += yC * yC;
                rmsR121 += yR * yR;
            }
        }
        assert(rmsC414 > rmsR121 * 3.0f);

        std::cout << "  [PASS] Distance-coupled studio mic EQ (R121 prox ratio=" << std::sqrt(rmsClose / rmsFar)
                  << "x, C414/R121 air ratio=" << std::sqrt(rmsC414 / rmsR121) << "x)\n";
    }

    // 7. Leslie 122 vs Leslie 147 Dual-Cabinet Motor Ramp Physics
    {
        constexpr double sr = 48000.0;
        Leslie122 cab122, cab147;
        cab122.prepare(sr);
        cab147.prepare(sr);

        Leslie122::Parameters p122; p122.cabinetModel = 0; p122.speed = Leslie122::SpeedMode::Tremolo;
        Leslie122::Parameters p147; p147.cabinetModel = 1; p147.speed = Leslie122::SpeedMode::Tremolo;

        // Run 0.6 seconds into Tremolo spin-up: 147 (0.55s tau, 411 RPM) must outpace 122 (0.75s tau, 400 RPM)
        for (int i = 0; i < static_cast<int>(0.6 * sr); ++i)
        {
            (void) cab122.processSample(0.2f, p122);
            (void) cab147.processSample(0.2f, p147);
        }
        assert(cab147.getTelemetry().hornRpm > cab122.getTelemetry().hornRpm + 25.0f);
        assert(cab147.getTelemetry().drumRpm > cab122.getTelemetry().drumRpm + 5.0f);

        std::cout << "  [PASS] Leslie 122 vs 147 ramp physics (0.6s horn: 122="
                  << cab122.getTelemetry().hornRpm << " RPM, 147=" << cab147.getTelemetry().hornRpm << " RPM)\n";
    }

    // 8. Smart Idle Generator Gate (RUN vs QUIET)
    {
        constexpr double sr = 48000.0;
        TonewheelEngine eng;
        eng.prepare(sr);

        TonewheelEngine::Parameters p;
        p.leakage      = 0.8f;
        p.muteWhenIdle = true;

        // Idle from startup: generator floor must be exact 0.0f while wheel phases keep spinning
        const double phStart = eng.getWheelPhase(48);
        float maxIdleQuiet = 0.0f;
        for (int i = 0; i < 480; ++i)
        {
            auto o = eng.processSample(p);
            maxIdleQuiet = std::max(maxIdleQuiet, std::abs(o.toVibrato + o.dryBypass));
        }
        assert(maxIdleQuiet == 0.0f);
        assert(std::abs(eng.getWheelPhase(48) - phStart) > 1e-3);

        // Switch to Continuous Run (muteWhenIdle = false): vintage generator floor wakes up
        p.muteWhenIdle = false;
        float maxIdleRun = 0.0f;
        for (int i = 0; i < 2400; ++i)
        {
            auto o = eng.processSample(p);
            maxIdleRun = std::max(maxIdleRun, std::abs(o.toVibrato + o.dryBypass));
        }
        assert(maxIdleRun > 1e-4f);

        // Switch back to QUIET: holds 1.2s, fades over 0.4s -> after 1.7s must return to exact 0.0f
        p.muteWhenIdle = true;
        for (int i = 0; i < static_cast<int>(1.7 * sr); ++i)
            (void) eng.processSample(p);
        auto tail = eng.processSample(p);
        assert(std::abs(tail.toVibrato + tail.dryBypass) == 0.0f);

        std::cout << "  [PASS] Smart Idle Generator Gate (quiet=" << maxIdleQuiet
                  << ", continuousRun=" << maxIdleRun << ")\n";
    }

    // 9. Parallel Marshall 1962 Bluesbreaker 2x12 Combo Amp
    {
        constexpr double sr = 48000.0;
        MarshallBluesbreaker ampDark, ampBright;
        ampDark.prepare(sr);
        ampBright.prepare(sr);

        float rmsDark = 0.0f, rmsBright = 0.0f, peakBright = 0.0f;
        for (int i = 0; i < 4800; ++i)
        {
            // 2.8 kHz presence test tone at moderate drive
            const float in = 0.35f * std::sin(2.0f * std::numbers::pi_v<float> * 2800.0f * static_cast<float>(i) / static_cast<float>(sr));
            const float yD = ampDark.processSample(in, 0.45f, 0.05f);
            const float yB = ampBright.processSample(in, 0.45f, 0.95f);
            assert(std::isfinite(yD) && std::isfinite(yB));
            if (i > 1000)
            {
                rmsDark    += yD * yD;
                rmsBright  += yB * yB;
                peakBright  = std::max(peakBright, std::abs(yB));
            }
        }
        assert(rmsBright > rmsDark * 1.4f);
        assert(peakBright > 0.05f && peakBright < 1.2f);
        std::cout << "  [PASS] Marshall 1962 Bluesbreaker 2x12 (bright/dark ratio="
                  << std::sqrt(rmsBright / rmsDark) << "x, peak=" << peakBright << ")\n";
    }

    std::cout << "All DSP tests passed!\n";
    return 0;
}
