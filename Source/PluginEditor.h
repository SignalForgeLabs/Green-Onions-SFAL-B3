#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "UI/VintageLookAndFeel.h"
#include "UI/DualDrawbarDeck.h"
#include "UI/TiltRockerSwitch.h"
#include "UI/PresetOctaveComponent.h"
#include "UI/HalfMoonSwitch.h"
#include "UI/SwellPedalComponent.h"
#include "UI/LeslieVisualizerComponent.h"
#include "UI/StageMicLayerComponent.h"
#include "UI/OrganKeyboard.h"

#include <memory>

namespace GreenOnions
{

class GreenOnionsAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit GreenOnionsAudioProcessorEditor(GreenOnionsAudioProcessor&);
    ~GreenOnionsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override {}
    void resized() override;

    static constexpr int kBaseWidth        = 1280;
    static constexpr int kFullHeight       = 740;
    static constexpr int kLiveRackHeight   = 440;

private:
    void timerCallback() override;
    void applyWindowScaleAndMode();

    struct LabeledKnob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

        void init(juce::Component& parent, juce::AudioProcessorValueTreeState& apvts,
                  const char* paramId, const juce::String& text);
        void layout(juce::Rectangle<int> bounds);
    };

    // Inner fixed-coordinate canvas scaled proportionally via AffineTransform::scale
    class ConsoleCanvas final : public juce::Component
    {
    public:
        explicit ConsoleCanvas(GreenOnionsAudioProcessorEditor& ownerRef) : owner_(ownerRef) {}
        void paint(juce::Graphics& g) override { owner_.paintCanvas(g, getLocalBounds()); }
        void resized() override                { owner_.layoutCanvas(getLocalBounds()); }

    private:
        GreenOnionsAudioProcessorEditor& owner_;
    };

    void paintCanvas(juce::Graphics& g, juce::Rectangle<int> bounds);
    void layoutCanvas(juce::Rectangle<int> bounds);

    GreenOnionsAudioProcessor& processor_;
    UI::VintageLookAndFeel lookAndFeel_;
    ConsoleCanvas canvas_ { *this };
    juce::TooltipWindow tooltipWindow_ { this, 500 };

    bool  isLiveRackMode_ = false;
    float zoomScale_      = 1.0f;

    // Header
    juce::ComboBox organModelBox_;
    juce::Label    organModelLabel_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> organModelAttachment_;
    juce::ComboBox presetBox_;
    juce::Label    presetLabel_;
    juce::TextButton stageViewToggleBtn_ { "STAGE MICS" };
    juce::TextButton liveRackToggleBtn_  { "LIVE RACK" };
    juce::ComboBox   zoomBox_;
    LabeledKnob masterKnob_;
    std::unique_ptr<UI::TiltRockerSwitch> splitSwitch_;

    // Unified 38-Drawbar B-3 Deck (Upper A, Upper B, Pedals, Lower A, Lower B)
    std::unique_ptr<UI::DualDrawbarDeck> dualDrawbarDeck_;

    // Aged Bakelite Ivory Tilt-Rocker Switches (Percussion & Vibrato/Chorus)
    std::unique_ptr<UI::TiltRockerSwitch> percOnSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> percSoftSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> percFastSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> percThirdSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> upperVCSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> lowerVCSwitch_;
    juce::ComboBox vcModeBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> vcModeAttachment_;

    // Tonewheel, Preamp, Spring Reverb, Leslie knobs
    LabeledKnob keyClickKnob_;
    LabeledKnob leakageKnob_;
    LabeledKnob driveKnob_;
    LabeledKnob swellKnob_;
    std::unique_ptr<UI::TiltRockerSwitch> idleMuteSwitch_;
    std::unique_ptr<UI::TiltRockerSwitch> marshallConsoleSwitch_;
    LabeledKnob reverbMixKnob_;
    LabeledKnob reverbDecayKnob_;
    LabeledKnob micSpreadKnob_;
    LabeledKnob micDistKnob_;
    LabeledKnob hornBalKnob_;

    // Leslie 122/147 controls & 60 fps cutaway rotor chamber visualizer
    std::unique_ptr<UI::TiltRockerSwitch> leslieOnSwitch_;
    std::unique_ptr<UI::HalfMoonSwitch>   halfMoonSwitch_;
    UI::LeslieVisualizerComponent         leslieVisualizer_;

    // Reverse-Color Preset Octaves (C..B) + Interactive Dual Manuals & 25-Note Pedalboard
    // + Always-visible Lower Half-Moon Leslie Switch & Expression Swell Pedal
    std::unique_ptr<UI::PresetOctaveComponent> upperPresetOctave_;
    std::unique_ptr<UI::PresetOctaveComponent> lowerPresetOctave_;
    std::unique_ptr<UI::OrganKeyboard> upperKeyboard_;
    std::unique_ptr<UI::OrganKeyboard> lowerKeyboard_;
    std::unique_ptr<UI::OrganKeyboard> pedalKeyboard_;
    std::unique_ptr<UI::HalfMoonSwitch>      lowerHalfMoonSwitch_;
    std::unique_ptr<UI::SwellPedalComponent> swellPedal_;

    // Interactive Maple Stage & Draggable Stereo Mic Layer (toggled via STAGE MICS button)
    UI::StageMicLayerComponent stageMicLayer_;

    // Cached LED telemetry for paint()
    int upperHeld_ = 0;
    int lowerHeld_ = 0;
    int pedalHeld_ = 0;

    // Layout rects for section framing in paintCanvas()
    juce::Rectangle<int> percVcFrame_, preampFrame_, reverbFrame_, leslieFrame_;
    juce::Rectangle<int> ledArea_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GreenOnionsAudioProcessorEditor)
};

} // namespace GreenOnions
