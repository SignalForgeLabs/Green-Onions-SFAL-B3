#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include "Parameters.h"
#include "Presets.h"
#include "DSP/TonewheelEngine.h"
#include "DSP/ScannerVibrato.h"
#include "DSP/PreampSwell.h"
#include "DSP/SpringReverb.h"
#include "DSP/Leslie122.h"
#include "DSP/LeslieMicEQ.h"
#include "DSP/MarshallBluesbreaker.h"

#include <atomic>
#include <array>

namespace GreenOnions
{

class GreenOnionsAudioProcessor final : public juce::AudioProcessor
{
public:
    GreenOnionsAudioProcessor();
    ~GreenOnionsAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using juce::AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Green Onions"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram_; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts_; }

    // Thread-safe MIDI injection from on-screen Upper/Lower/Pedal keyboards
    void injectNoteOn(int channel, int midiNote, float velocity);
    void injectNoteOff(int channel, int midiNote);

    // Reverse-color preset key slot recall & snapshot (C#..A, keys 1..9)
    void saveManualPresetSlot(bool isUpper, int keyIndex);
    void recallManualPresetSlot(bool isUpper, int keyIndex);

    // Live UI telemetry (lock-free atomics)
    struct Telemetry
    {
        std::atomic<float> hornSpeedHz { DSP::Leslie122::kHornChoraleHz };
        std::atomic<float> drumSpeedHz { DSP::Leslie122::kDrumChoraleHz };
        std::atomic<float> hornAngle   { 0.0f };
        std::atomic<float> drumAngle   { 0.0f };
        std::atomic<int>   upperHeld   { 0 };
        std::atomic<int>   lowerHeld   { 0 };
        std::atomic<int>   pedalHeld   { 0 };
        std::atomic<float> meterL      { 0.0f };
        std::atomic<float> meterR      { 0.0f };
    };

    const Telemetry& getTelemetry() const noexcept { return telemetry_; }

    DSP::Leslie122::RotorTelemetry getLeslieTelemetry() const noexcept
    {
        return leslie122_.getTelemetry();
    }

private:
    void handleMidiMessage(const juce::MidiMessage& msg, bool splitMode);
    void setParamNormalizedFromAudioThread(const char* paramId, float plainValue);
    void appendPresetSlotsToXml(juce::XmlElement& parentXml) const;
    void restorePresetSlotsFromXml(const juce::XmlElement& parentXml);

    juce::AudioProcessorValueTreeState apvts_;
    int currentProgram_ = 0;

    // Cached raw parameter pointers for lock-free audio thread access
    std::array<std::atomic<float>*, 9> upperDbParams_ {};
    std::array<std::atomic<float>*, 9> upperBDbParams_ {};
    std::array<std::atomic<float>*, 9> lowerDbParams_ {};
    std::array<std::atomic<float>*, 9> lowerBDbParams_ {};
    std::array<std::atomic<float>*, 2> pedalDbParams_ {};

    std::atomic<float>* upperPresetKeyParam_ = nullptr;
    std::atomic<float>* lowerPresetKeyParam_ = nullptr;

    // 9 instant-recall reverse-color preset slots (C#..A) per manual
    std::array<std::array<float, 9>, 9> upperPresetSlots_ {};
    std::array<std::array<float, 9>, 9> lowerPresetSlots_ {};

    std::atomic<float>* percOnParam_      = nullptr;
    std::atomic<float>* percSoftParam_    = nullptr;
    std::atomic<float>* percFastParam_    = nullptr;
    std::atomic<float>* percThirdParam_   = nullptr;
    std::atomic<float>* vcModeParam_      = nullptr;
    std::atomic<float>* upperVCOnParam_   = nullptr;
    std::atomic<float>* lowerVCOnParam_   = nullptr;
    std::atomic<float>* keyClickParam_    = nullptr;
    std::atomic<float>* leakageParam_     = nullptr;
    std::atomic<float>* idleGenMuteParam_ = nullptr;
    std::atomic<float>* swellParam_       = nullptr;
    std::atomic<float>* driveParam_       = nullptr;
    std::atomic<float>* reverbMixParam_   = nullptr;
    std::atomic<float>* reverbDecayParam_ = nullptr;
    std::atomic<float>* leslieOnParam_       = nullptr;
    std::atomic<float>* leslieCabModelParam_ = nullptr;
    std::atomic<float>* leslieSpeedParam_    = nullptr;
    std::atomic<float>* micSpreadParam_   = nullptr;
    std::atomic<float>* micDistanceParam_ = nullptr;
    std::atomic<float>* hornBalanceParam_ = nullptr;
    std::atomic<float>* masterGainParam_  = nullptr;
    std::atomic<float>* midiSplitParam_   = nullptr;
    std::atomic<float>* organModelParam_  = nullptr;
    std::atomic<float>* micLTypeParam_    = nullptr;
    std::atomic<float>* micRTypeParam_    = nullptr;
    std::atomic<float>* micLXParam_       = nullptr;
    std::atomic<float>* micLYParam_       = nullptr;
    std::atomic<float>* micRXParam_       = nullptr;
    std::atomic<float>* micRYParam_       = nullptr;
    std::atomic<float>* marshallOnParam_   = nullptr;
    std::atomic<float>* marshallGainParam_ = nullptr;
    std::atomic<float>* marshallToneParam_ = nullptr;
    std::atomic<float>* marshallMixParam_  = nullptr;
    std::atomic<float>* ampSm57XParam_     = nullptr;
    std::atomic<float>* ampSm57YParam_     = nullptr;
    std::atomic<float>* ampC414XParam_     = nullptr;
    std::atomic<float>* ampC414YParam_     = nullptr;

    // DSP Chain
    DSP::TonewheelEngine      tonewheelEngine_;
    DSP::ScannerVibrato       scannerVibrato_;
    DSP::PreampSwell          preampSwell_;
    DSP::SpringReverb         springReverb_;
    DSP::Leslie122            leslie122_;
    DSP::LeslieMicChannelEQ   micEqL_;
    DSP::LeslieMicChannelEQ   micEqR_;
    DSP::MarshallBluesbreaker marshallAmp_;
    DSP::LeslieMicChannelEQ   ampMicSm57_; // Shure SM57 @ 1' in front of Bluesbreaker 2x12
    DSP::LeslieMicChannelEQ   ampMicC414_; // AKG C414  @ 3' in front of Bluesbreaker 2x12

    // Acoustic time-of-arrival delay line for Bluesbreaker SM57 & C414 movable stereo mic matrix
    std::array<float, 1024> ampMicDelayBuf_ {};
    int ampMicDelayWrite_ = 0;
    double currentSampleRate_ = 44100.0;

    // UI Keyboard MIDI collector
    juce::MidiMessageCollector uiMidiCollector_;
    bool sustainWasDown_ = false;

    Telemetry telemetry_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GreenOnionsAudioProcessor)
};

} // namespace GreenOnions
