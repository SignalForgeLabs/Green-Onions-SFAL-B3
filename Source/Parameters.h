#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <vector>

namespace GreenOnions::Params
{

inline constexpr std::array<const char*, 9> kDrawbarLabels = {
    "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'"
};

inline constexpr std::array<const char*, 9> kUpperDrawbarIds = {
    "upper_db_0", "upper_db_1", "upper_db_2", "upper_db_3", "upper_db_4",
    "upper_db_5", "upper_db_6", "upper_db_7", "upper_db_8"
};

inline constexpr std::array<const char*, 9> kUpperBDrawbarIds = {
    "upper_b_db_0", "upper_b_db_1", "upper_b_db_2", "upper_b_db_3", "upper_b_db_4",
    "upper_b_db_5", "upper_b_db_6", "upper_b_db_7", "upper_b_db_8"
};

inline constexpr std::array<const char*, 9> kLowerDrawbarIds = {
    "lower_db_0", "lower_db_1", "lower_db_2", "lower_db_3", "lower_db_4",
    "lower_db_5", "lower_db_6", "lower_db_7", "lower_db_8"
};

inline constexpr std::array<const char*, 9> kLowerBDrawbarIds = {
    "lower_b_db_0", "lower_b_db_1", "lower_b_db_2", "lower_b_db_3", "lower_b_db_4",
    "lower_b_db_5", "lower_b_db_6", "lower_b_db_7", "lower_b_db_8"
};

inline constexpr std::array<const char*, 2> kPedalDrawbarIds = {
    "pedal_db_0", "pedal_db_1"
};

// Reverse-Color Preset Keys (0 = C Cancel, 1..9 = C#..A Presets, 10 = Bb Set B, 11 = B Set A)
inline constexpr const char* kUpperPresetKey = "upper_preset_key";
inline constexpr const char* kLowerPresetKey = "lower_preset_key";

// Percussion
inline constexpr const char* kPercOn    = "perc_on";
inline constexpr const char* kPercSoft  = "perc_soft";
inline constexpr const char* kPercFast  = "perc_fast";
inline constexpr const char* kPercThird = "perc_third";

// Scanner Vibrato / Chorus
inline constexpr const char* kVCMode    = "vc_mode";
inline constexpr const char* kUpperVCOn = "upper_vc_on";
inline constexpr const char* kLowerVCOn = "lower_vc_on";

// Tonewheel & Key Click
inline constexpr const char* kKeyClick          = "key_click";
inline constexpr const char* kLeakage           = "leakage";
inline constexpr const char* kIdleGeneratorMute = "idle_generator_mute";

// AO-28 Preamp & Expression Swell
inline constexpr const char* kSwell     = "swell";
inline constexpr const char* kDrive     = "drive";

// Accutronics Spring Reverb
inline constexpr const char* kReverbMix   = "reverb_mix";
inline constexpr const char* kReverbDecay = "reverb_decay";

// Leslie 122 / 147
inline constexpr const char* kLeslieOn           = "leslie_on";
inline constexpr const char* kLeslieCabinetModel = "leslie_cabinet_model";
inline constexpr const char* kLeslieSpeed        = "leslie_speed";
inline constexpr const char* kMicSpread    = "mic_spread";
inline constexpr const char* kMicDistance  = "mic_distance";
inline constexpr const char* kHornBalance  = "horn_balance";

// Master & Routing
inline constexpr const char* kMasterGain   = "master_gain";
inline constexpr const char* kMidiSplit    = "midi_split";
inline constexpr const char* kUpperMidiCh  = "midi_ch_upper";
inline constexpr const char* kLowerMidiCh  = "midi_ch_lower";
inline constexpr const char* kPedalMidiCh  = "midi_ch_pedal";
inline constexpr const char* kOrganModel   = "organ_model";

// Interactive Stage & Draggable Stereo Mic Placement
inline constexpr const char* kStagePerspective = "stage_view_perspective";
inline constexpr const char* kMicStereoLink    = "mic_stereo_link";
inline constexpr const char* kMicLX            = "mic_l_x";
inline constexpr const char* kMicLY            = "mic_l_y";
inline constexpr const char* kMicRX            = "mic_r_x";
inline constexpr const char* kMicRY            = "mic_r_y";
inline constexpr const char* kMicLType         = "mic_l_type";
inline constexpr const char* kMicRType         = "mic_r_type";

// Parallel Marshall 1962 "Bluesbreaker" 2x12 Combo
inline constexpr const char* kMarshallOn       = "marshall_on";
inline constexpr const char* kMarshallGain     = "marshall_gain";
inline constexpr const char* kMarshallTone     = "marshall_tone";
inline constexpr const char* kMarshallMix      = "marshall_mix";
inline constexpr const char* kAmpSm57X         = "amp_sm57_x";
inline constexpr const char* kAmpSm57Y         = "amp_sm57_y";
inline constexpr const char* kAmpC414X         = "amp_c414_x";
inline constexpr const char* kAmpC414Y         = "amp_c414_y";

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Default patch: "Green Onions" (Booker T. style: 88 8800 000 upper A, 00 8880 000 lower A)
    constexpr std::array<float, 9> kDefaultUpperA = { 8, 8, 8, 8, 0, 0, 0, 0, 0 };
    constexpr std::array<float, 9> kDefaultUpperB = { 8, 0, 6, 6, 0, 0, 0, 0, 8 };
    constexpr std::array<float, 9> kDefaultLowerA = { 0, 0, 8, 8, 8, 0, 0, 0, 0 };
    constexpr std::array<float, 9> kDefaultLowerB = { 7, 0, 7, 0, 0, 0, 0, 0, 0 };
    constexpr std::array<float, 2> kDefaultPedal  = { 7, 4 };

    for (size_t i = 0; i < 9; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { kUpperDrawbarIds[i], 1 },
            juce::String("Upper A ") + kDrawbarLabels[i],
            juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f),
            kDefaultUpperA[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { kUpperBDrawbarIds[i], 1 },
            juce::String("Upper B ") + kDrawbarLabels[i],
            juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f),
            kDefaultUpperB[i]));
    }

    for (size_t i = 0; i < 9; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { kLowerDrawbarIds[i], 1 },
            juce::String("Lower A ") + kDrawbarLabels[i],
            juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f),
            kDefaultLowerA[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { kLowerBDrawbarIds[i], 1 },
            juce::String("Lower B ") + kDrawbarLabels[i],
            juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f),
            kDefaultLowerB[i]));
    }

    // Reverse-color preset key per manual (0=Cancel, 1..9=C#..A slots, 10=Bb Set B, 11=B Set A)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { kUpperPresetKey, 1 }, "Upper Preset Key", 0, 11, 11));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { kLowerPresetKey, 1 }, "Lower Preset Key", 0, 11, 11));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kPedalDrawbarIds[0], 1 }, "Pedal 16'",
        juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f), kDefaultPedal[0]));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kPedalDrawbarIds[1], 1 }, "Pedal 8'",
        juce::NormalisableRange<float>(0.0f, 8.0f, 1.0f), kDefaultPedal[1]));

    // Percussion
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kPercOn, 1 }, "Percussion On", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kPercSoft, 1 }, "Percussion Soft", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kPercFast, 1 }, "Percussion Fast", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kPercThird, 1 }, "Percussion 3rd", true));

    // Scanner Vibrato / Chorus
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kVCMode, 1 }, "Vibrato / Chorus",
        juce::StringArray { "Off", "V1", "C1", "V2", "C2", "V3", "C3" }, 6)); // Default C3
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kUpperVCOn, 1 }, "Upper Vibrato/Chorus", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kLowerVCOn, 1 }, "Lower Vibrato/Chorus", false));

    // Tonewheel / Key Click
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kKeyClick, 1 }, "Key Click",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kLeakage, 1 }, "Tonewheel Leakage",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.28f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kIdleGeneratorMute, 1 }, "Mute Idle Generator", true));

    // AO-28 Preamp & Expression Swell
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kSwell, 1 }, "Expression Swell",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.005f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kDrive, 1 }, "AO-28 Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));

    // Accutronics Spring Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kReverbMix, 1 }, "Spring Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.24f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kReverbDecay, 1 }, "Spring Reverb Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f));

    // Leslie 122 / 147
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kLeslieOn, 1 }, "Leslie On", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kLeslieCabinetModel, 1 }, "Leslie Cabinet Model",
        juce::StringArray { "Leslie 122", "Leslie 147" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kLeslieSpeed, 1 }, "Leslie Speed",
        juce::StringArray { "Chorale", "Brake", "Tremolo" }, 0)); // Default Chorale
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicSpread, 1 }, "Leslie Mic Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicDistance, 1 }, "Leslie Mic Distance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kHornBalance, 1 }, "Horn / Drum Balance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f));

    // Master & MIDI
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMasterGain, 1 }, "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.5f, 0.01f), 0.90f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kMidiSplit, 1 }, "Single-KB Split Mode", false));

    juce::StringArray midiChannelChoices;
    midiChannelChoices.add("Omni");
    for (int ch = 1; ch <= 16; ++ch)
        midiChannelChoices.add("Ch " + juce::String(ch));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kUpperMidiCh, 1 }, "Upper Manual MIDI Ch", midiChannelChoices, 1)); // Default Ch 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kLowerMidiCh, 1 }, "Lower Manual MIDI Ch", midiChannelChoices, 2)); // Default Ch 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kPedalMidiCh, 1 }, "Pedalboard MIDI Ch",   midiChannelChoices, 3)); // Default Ch 3
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kOrganModel, 1 }, "Load Your Organ",
        juce::StringArray {
            "1955 Wax-Cap Jazz B-3",
            "1959 Golden Era B-3",
            "1968 Red-Cap Rock B-3",
            "1962 Sanctuary C-3",
            "1972 Road-Worn Club Chop"
        }, 1));

    // Interactive Stage & Draggable Stereo Mics
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kStagePerspective, 1 }, "Stage Perspective (0=Top, 1=Stage)", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kMicStereoLink, 1 }, "Stereo Mic Link", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicLX, 1 }, "Left Mic X", 0.0f, 1.0f, 0.56f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicLY, 1 }, "Left Mic Y", 0.0f, 1.0f, 0.36f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicRX, 1 }, "Right Mic X", 0.0f, 1.0f, 0.78f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMicRY, 1 }, "Right Mic Y", 0.0f, 1.0f, 0.36f));

    const juce::StringArray micModels {
        "Neumann U47",
        "AKG C414",
        "Royer R-121",
        "Shure SM57"
    };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kMicLType, 1 }, "Left Mic Model", micModels, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { kMicRType, 1 }, "Right Mic Model", micModels, 0));

    // Parallel Marshall 1962 "Bluesbreaker" 2x12 Combo
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { kMarshallOn, 1 }, "Marshall 2x12 On", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMarshallGain, 1 }, "Marshall Drive", 0.0f, 1.0f, 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMarshallTone, 1 }, "Marshall Presence", 0.0f, 1.0f, 0.60f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kMarshallMix, 1 }, "Marshall / Leslie Blend", 0.0f, 1.0f, 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kAmpSm57X, 1 }, "Bluesbreaker SM57 X", 0.0f, 1.0f, 0.30f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kAmpSm57Y, 1 }, "Bluesbreaker SM57 Y", 0.0f, 1.0f, 0.62f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kAmpC414X, 1 }, "Bluesbreaker C414 X", 0.0f, 1.0f, 0.38f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { kAmpC414Y, 1 }, "Bluesbreaker C414 Y", 0.0f, 1.0f, 0.78f));

    return { params.begin(), params.end() };
}

} // namespace GreenOnions::Params
