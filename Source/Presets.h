#pragma once

#include "Parameters.h"
#include <array>
#include <vector>

namespace GreenOnions::Presets
{

struct Preset
{
    const char* name;
    std::array<float, 9> upper;
    std::array<float, 9> lower;
    std::array<float, 2> pedal;
    bool  percOn;
    bool  percSoft;
    bool  percFast;
    bool  percThird;
    int   vcMode;         // 0=Off, 1=V1, 2=C1, 3=V2, 4=C2, 5=V3, 6=C3
    bool  upperVC;
    bool  lowerVC;
    float keyClick;
    float leakage;
    float drive;
    float reverbMix;
    float reverbDecay;
    bool  leslieOn;
    int   leslieSpeed;    // 0=Chorale, 1=Brake, 2=Tremolo
    int   organModel;     // 0=1955 Wax-Cap, 1=1959 Golden Era, 2=1968 Red-Cap, 3=1962 Sanctuary, 4=1972 Chop
    int   leslieCabModel; // 0=Leslie 122, 1=Leslie 147
    bool  marshallOn;
    float marshallGain;
    float marshallTone;
    float marshallMix;
};

inline const std::vector<Preset>& getFactoryPresets()
{
    static const std::vector<Preset> kPresets = {
        {
            "Green Onions (Booker T.)",
            { 8, 8, 8, 8, 0, 0, 0, 0, 0 },
            { 0, 0, 8, 8, 8, 0, 0, 0, 0 },
            { 7, 4 },
            false, false, true, true,
            6, true, false,       // C3 on Upper
            0.58f, 0.30f, 0.38f,  // Punchy click, vintage leakage, warm AO-28 grit
            0.24f, 0.50f,         // Classic Stax studio spring reverb
            true, 0,              // Leslie Chorale
            1, 0,                 // 1959 Golden Era B-3 + Leslie 122
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Jimmy Smith Jazz (888000000)",
            { 8, 8, 8, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 8, 3, 8, 0, 0, 0, 0 },
            { 8, 0 },
            true, false, true, true, // 3rd Harmonic, Fast, Normal
            6, true, false,          // C3 on Upper
            0.65f, 0.25f, 0.22f,
            0.16f, 0.42f,
            true, 0,
            0, 0,                    // 1955 Wax-Cap Jazz B-3 + Leslie 122
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Gospel Shout",
            { 8, 8, 8, 8, 8, 8, 8, 8, 8 },
            { 8, 4, 8, 6, 4, 2, 0, 0, 0 },
            { 8, 6 },
            true, false, true, false, // 2nd Harmonic, Fast, Normal
            6, true, true,            // C3 on both
            0.62f, 0.35f, 0.52f,
            0.32f, 0.62f,
            true, 2,                  // Leslie Tremolo
            1, 0,                     // 1959 Golden Era B-3 + Leslie 122
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Rock Legend Jon Lord",
            { 8, 6, 8, 8, 6, 8, 6, 8, 8 },
            { 8, 8, 8, 0, 0, 0, 0, 0, 0 },
            { 8, 5 },
            false, false, true, true,
            0, false, false,          // Raw straight tone into Leslie 147 + Marshall 2x12
            0.78f, 0.40f, 0.85f,      // Cranking AO-28 tube roar
            0.18f, 0.42f,
            true, 2,                  // Leslie Tremolo
            2, 1,                     // 1968 Red-Cap Rock B-3 + Leslie 147
            true, 0.72f, 0.68f, 0.65f // Parallel Marshall 1962 Bluesbreaker 2x12 engaged!
        },
        {
            "Whiter Shade of Pale",
            { 8, 0, 8, 8, 0, 0, 0, 0, 8 },
            { 0, 0, 8, 4, 4, 0, 0, 0, 0 },
            { 6, 4 },
            false, false, false, false,
            6, true, true,
            0.40f, 0.22f, 0.20f,
            0.42f, 0.70f,
            true, 0,
            1, 0,
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Mellow Comping",
            { 8, 0, 8, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 6, 4, 0, 0, 0, 0, 0 },
            { 6, 2 },
            true, true, false, false, // Soft, Slow, 2nd Harmonic
            4, true, false,           // C2
            0.35f, 0.20f, 0.14f,
            0.28f, 0.52f,
            true, 0,
            0, 0,
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Cathedral Full (Stationary)",
            { 8, 8, 8, 8, 8, 8, 8, 8, 8 },
            { 8, 6, 8, 6, 6, 4, 4, 4, 4 },
            { 8, 8 },
            false, false, true, false,
            5, true, true,            // V3
            0.30f, 0.18f, 0.15f,
            0.55f, 0.85f,
            true, 1,                  // Brake (stationary cabinet)
            3, 0,                     // 1962 Sanctuary C-3 + Leslie 122
            false, 0.65f, 0.60f, 0.50f
        },
        {
            "Squabblin' Funk",
            { 8, 0, 0, 0, 0, 0, 0, 0, 8 },
            { 0, 0, 8, 8, 0, 0, 0, 0, 0 },
            { 7, 5 },
            true, false, true, true,
            6, true, false,
            0.85f, 0.38f, 0.65f,
            0.18f, 0.38f,
            true, 2,
            4, 1,                     // 1972 Road-Worn Club Chop + Leslie 147
            false, 0.65f, 0.60f, 0.50f
        }
    };
    return kPresets;
}

inline void applyPreset(juce::AudioProcessorValueTreeState& apvts, int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    const auto& p = presets[static_cast<size_t>(index)];

    auto setParam = [&](const char* id, float plainValue)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(plainValue));
    };

    for (size_t i = 0; i < 9; ++i)
    {
        setParam(Params::kUpperDrawbarIds[i], p.upper[i]);
        setParam(Params::kLowerDrawbarIds[i], p.lower[i]);
    }
    setParam(Params::kPedalDrawbarIds[0], p.pedal[0]);
    setParam(Params::kPedalDrawbarIds[1], p.pedal[1]);

    setParam(Params::kPercOn,             p.percOn ? 1.0f : 0.0f);
    setParam(Params::kPercSoft,           p.percSoft ? 1.0f : 0.0f);
    setParam(Params::kPercFast,           p.percFast ? 1.0f : 0.0f);
    setParam(Params::kPercThird,          p.percThird ? 1.0f : 0.0f);
    setParam(Params::kVCMode,             static_cast<float>(p.vcMode));
    setParam(Params::kUpperVCOn,          p.upperVC ? 1.0f : 0.0f);
    setParam(Params::kLowerVCOn,          p.lowerVC ? 1.0f : 0.0f);
    setParam(Params::kKeyClick,           p.keyClick);
    setParam(Params::kLeakage,            p.leakage);
    setParam(Params::kDrive,              p.drive);
    setParam(Params::kReverbMix,          p.reverbMix);
    setParam(Params::kReverbDecay,        p.reverbDecay);
    setParam(Params::kLeslieOn,           p.leslieOn ? 1.0f : 0.0f);
    setParam(Params::kLeslieSpeed,        static_cast<float>(p.leslieSpeed));
    setParam(Params::kOrganModel,         static_cast<float>(p.organModel));
    setParam(Params::kLeslieCabinetModel, static_cast<float>(p.leslieCabModel));
    setParam(Params::kMarshallOn,         p.marshallOn ? 1.0f : 0.0f);
    setParam(Params::kMarshallGain,       p.marshallGain);
    setParam(Params::kMarshallTone,       p.marshallTone);
    setParam(Params::kMarshallMix,        p.marshallMix);
}

} // namespace GreenOnions::Presets
