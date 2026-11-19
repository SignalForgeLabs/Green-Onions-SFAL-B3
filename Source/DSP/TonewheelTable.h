#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace GreenOnions::DSP
{

// Authentic Hammond B-3 constants
inline constexpr int kNumTonewheels    = 91;   // Wheel 1 (C1) .. Wheel 91 (F#8), 0-indexed [0..90]
inline constexpr int kNumManualKeys    = 61;   // C2 (MIDI 36) .. C7 (MIDI 96)
inline constexpr int kManualBaseMidi   = 36;   // C2
inline constexpr int kNumPedalKeys     = 25;   // C2 (MIDI 36) .. C4 (MIDI 60)
inline constexpr int kPedalBaseMidi    = 36;   // C2
inline constexpr int kNumDrawbars      = 9;    // 16', 5-1/3', 8', 4', 2-2/3', 2', 1-3/5', 1-1/3', 1'
inline constexpr int kNumPedalDrawbars = 2;    // 16', 8'

// Semitone offsets for the 9 Hammond drawbars relative to the 8' fundamental
inline constexpr std::array<int, kNumDrawbars> kDrawbarSemitoneOffsets = {
    -12, // 16'    (Sub-octave)
      7, // 5-1/3' (Sub-third / Quint: 3rd harmonic of 16')
      0, // 8'     (Fundamental)
     12, // 4'     (2nd harmonic)
     19, // 2-2/3' (3rd harmonic)
     24, // 2'     (4th harmonic)
     28, // 1-3/5' (5th harmonic)
     31, // 1-1/3' (6th harmonic)
     36  // 1'     (8th harmonic)
};

// Authentic Hammond B-3 synchronous gear tooth ratios (driven / driver)
// Base synchronous shaft reference for octave 4 is 320.0 Hz (20 rev/s * 16 teeth),
// yielding exact A4 = 320 * 11 / 8 = 440.000 Hz, with authentic slight mechanical
// detuning on the other 11 pitch classes.
struct GearRatio
{
    double numerator;
    double denominator;
};

inline constexpr std::array<GearRatio, 12> kHammondGearRatios = {{
    {  85.0, 104.0 }, // C  (-0.58 cents vs ET)
    {  71.0,  82.0 }, // C# (-0.68 cents)
    {  67.0,  73.0 }, // D  (+0.20 cents)
    {  35.0,  36.0 }, // D# (-0.09 cents)
    {  69.0,  67.0 }, // E  (-0.40 cents)
    { 108.0,  99.0 }, // F  (-0.68 cents)
    {  37.0,  32.0 }, // F# (+0.03 cents)
    {  49.0,  40.0 }, // G  (+0.02 cents)
    {  48.0,  37.0 }, // G# (-0.71 cents)
    {  11.0,   8.0 }, // A  ( 0.00 cents, exact 440.0 Hz)
    {  67.0,  46.0 }, // A# (-0.29 cents)
    {  54.0,  35.0 }  // B  (-0.59 cents)
}};

struct TonewheelTable
{
    // Frequency in Hz for each of the 91 tonewheels (0 = C1 .. 90 = F#8)
    std::array<double, kNumTonewheels> frequencies {};

    // Authentic Hammond generator taper gain per wheel (mild high-end roll-off & bass body)
    std::array<float, kNumTonewheels> taperGains {};

    // Foldback lookup table: manualWheelIndex[key 0..60][drawbar 0..8] -> wheel index [12..90]
    std::array<std::array<int, kNumDrawbars>, kNumManualKeys> manualWheelIndex {};

    // Percussion wheel lookup for Upper Manual: [key 0..60][0 = 2nd (4'), 1 = 3rd (2-2/3')]
    std::array<std::array<int, 2>, kNumManualKeys> percussionWheelIndex {};

    // Pedal wheel lookup: [pedalKey 0..24][0 = 16', 1 = 8'] -> wheel index [0..36]
    std::array<std::array<int, kNumPedalDrawbars>, kNumPedalKeys> pedalWheelIndex {};

    // Adjacent crosstalk pairs (physical bin neighbors on the tonewheel generator shaft)
    // On a B-3, wheels share shaft bins spaced by 48 wheels (4 octaves) and adjacent semitones.
    std::array<int, kNumTonewheels> crosstalkNeighborA {};
    std::array<int, kNumTonewheels> crosstalkNeighborB {};

    TonewheelTable()
    {
        initFrequencies();
        initFoldback();
        initCrosstalkMap();
    }

private:
    void initFrequencies()
    {
        // Wheel 0 = C1 (octave 1 -> 3 octaves below octave 4 reference 320 Hz -> 40 Hz base)
        for (int w = 0; w < kNumTonewheels; ++w)
        {
            const int pitchClass = w % 12;
            const int octave     = w / 12; // 0 = C1..B1, 1 = C2..B2, ..., 7 = C8..F#8
            const double baseHz  = 40.0 * static_cast<double>(1 << octave);
            const auto& ratio    = kHammondGearRatios[static_cast<size_t>(pitchClass)];
            frequencies[static_cast<size_t>(w)] = baseHz * (ratio.numerator / ratio.denominator);

            // Authentic B-3 generator taper:
            // Slight warmth below C3, flat through midrange, gentle roll-off above C6
            double gain = 1.0;
            if (w < 12)
                gain = 1.12; // Complex pedal wheels carry solid weight
            else if (w > 60)
                gain = 1.0 - 0.0085 * static_cast<double>(w - 60); // Gentle top-octave taper

            taperGains[static_cast<size_t>(w)] = static_cast<float>(gain);
        }
    }

    void initFoldback()
    {
        // Manual key 0 = C2 = Wheel 12 (0-indexed)
        for (int key = 0; key < kNumManualKeys; ++key)
        {
            const int fundamentalWheel = 12 + key; // C2 = 12 .. C7 = 72

            for (int db = 0; db < kNumDrawbars; ++db)
            {
                int w = fundamentalWheel + kDrawbarSemitoneOffsets[static_cast<size_t>(db)];

                // Bottom foldback: manuals never access pedal wheels 0..11
                while (w < 12)
                    w += 12;

                // Top foldback: highest wheel is index 90 (wheel #91, F#8)
                while (w >= kNumTonewheels)
                    w -= 12;

                manualWheelIndex[static_cast<size_t>(key)][static_cast<size_t>(db)] = w;
            }

            // Percussion taps: 2nd harmonic (4' = +12 st), 3rd harmonic (2-2/3' = +19 st)
            int w2 = fundamentalWheel + 12;
            while (w2 >= kNumTonewheels) w2 -= 12;
            percussionWheelIndex[static_cast<size_t>(key)][0] = w2;

            int w3 = fundamentalWheel + 19;
            while (w3 >= kNumTonewheels) w3 -= 12;
            percussionWheelIndex[static_cast<size_t>(key)][1] = w3;
        }

        // Pedals: 25 notes C2..C4
        for (int p = 0; p < kNumPedalKeys; ++p)
        {
            pedalWheelIndex[static_cast<size_t>(p)][0] = p;      // 16' -> C1..C3 (wheels 0..24)
            pedalWheelIndex[static_cast<size_t>(p)][1] = p + 12; // 8'  -> C2..C4 (wheels 12..36)
        }
    }

    void initCrosstalkMap()
    {
        for (int w = 0; w < kNumTonewheels; ++w)
        {
            // Bin partner (typically +48 or -48 wheels on the generator shaft)
            crosstalkNeighborA[static_cast<size_t>(w)] = (w + 48 < kNumTonewheels) ? (w + 48) : (w - 48);
            // Adjacent wiring loom neighbor
            crosstalkNeighborB[static_cast<size_t>(w)] = (w + 1 < kNumTonewheels) ? (w + 1) : (w - 1);
        }
    }
};

// Drawbar step 0..8 to linear amplitude (authentic Hammond ~3 dB per step taper)
inline float drawbarStepToGain(int step) noexcept
{
    static constexpr std::array<float, 9> kSteps = {
        0.0000f, // 0: Off
        0.0884f, // 1: -21 dB
        0.1250f, // 2: -18 dB
        0.1768f, // 3: -15 dB
        0.2500f, // 4: -12 dB
        0.3536f, // 5:  -9 dB
        0.5000f, // 6:  -6 dB
        0.7071f, // 7:  -3 dB
        1.0000f  // 8:   0 dB
    };
    const int clamped = (step < 0) ? 0 : (step > 8 ? 8 : step);
    return kSteps[static_cast<size_t>(clamped)];
}

// Continuous [0.0..8.0] drawbar value to linear gain (smoothly interpolated for DAW automation)
inline float drawbarValueToGain(float value) noexcept
{
    if (value <= 0.0f) return 0.0f;
    if (value >= 8.0f) return 1.0f;
    const int idx = static_cast<int>(value);
    const float frac = value - static_cast<float>(idx);
    const float g0 = drawbarStepToGain(idx);
    const float g1 = drawbarStepToGain(idx + 1);
    return g0 + frac * (g1 - g0);
}

} // namespace GreenOnions::DSP
