#include "PluginEditor.h"
#include "Presets.h"

namespace GreenOnions
{

void GreenOnionsAudioProcessorEditor::LabeledKnob::init(
    juce::Component& parent,
    juce::AudioProcessorValueTreeState& apvts,
    const char* paramId,
    const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    parent.addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, UI::Palette::creamDim);
    parent.addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
}

void GreenOnionsAudioProcessorEditor::LabeledKnob::layout(juce::Rectangle<int> bounds)
{
    label.setBounds(bounds.removeFromBottom(15));
    slider.setBounds(bounds);
}

GreenOnionsAudioProcessorEditor::GreenOnionsAudioProcessorEditor(GreenOnionsAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor_(p),
      leslieVisualizer_(p.getAPVTS(), [&p] { return p.getLeslieTelemetry(); }),
      stageMicLayer_(p.getAPVTS(), [&p] { return p.getLeslieTelemetry(); })
{
    setLookAndFeel(&lookAndFeel_);
    canvas_.setLookAndFeel(&lookAndFeel_);
    addAndMakeVisible(canvas_);

    auto& apvts = processor_.getAPVTS();

    // Header: "Load Your Organ" Console Voicing Selector, Registration Presets, View Toggles, Zoom, Split, Master
    organModelLabel_.setText("CONSOLE", juce::dontSendNotification);
    organModelLabel_.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    organModelLabel_.setColour(juce::Label::textColourId, UI::Palette::amberGlow);
    organModelLabel_.setJustificationType(juce::Justification::centredLeft);
    canvas_.addAndMakeVisible(organModelLabel_);

    organModelBox_.addItemList({
        "1955 Wax-Cap Jazz B-3",
        "1959 Golden Era B-3",
        "1968 Red-Cap Rock B-3",
        "1962 Sanctuary C-3",
        "1972 Road-Worn Club Chop"
    }, 1);
    organModelBox_.setColour(juce::ComboBox::outlineColourId, UI::Palette::amber);
    canvas_.addAndMakeVisible(organModelBox_);
    organModelAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::kOrganModel, organModelBox_);

    presetLabel_.setText("REGISTRATION", juce::dontSendNotification);
    presetLabel_.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    presetLabel_.setColour(juce::Label::textColourId, UI::Palette::creamDim);
    presetLabel_.setJustificationType(juce::Justification::centredLeft);
    canvas_.addAndMakeVisible(presetLabel_);

    presetBox_.onChange = [this]()
    {
        const int id = presetBox_.getSelectedId();
        if (id >= kUserPresetIdBase)
        {
            const int userIdx = id - kUserPresetIdBase;
            if (userIdx >= 0 && userIdx < userPresetNames_.size())
                processor_.loadUserPreset(userPresetNames_[userIdx]);
        }
        else if (id >= 1)
        {
            processor_.setCurrentProgram(id - 1);
        }
        deletePresetBtn_.setEnabled(processor_.isCurrentPresetUser());
    };
    canvas_.addAndMakeVisible(presetBox_);

    auto styleActionBtn = [](juce::TextButton& b, bool isAccent)
    {
        b.setColour(juce::TextButton::buttonColourId,  isAccent ? juce::Colour(0xff3a2a16) : juce::Colour(0xff231b1a));
        b.setColour(juce::TextButton::textColourOffId, isAccent ? juce::Colour(0xffffcf87) : juce::Colour(0xffe0a899));
    };

    styleActionBtn(savePresetBtn_, true);
    savePresetBtn_.setTooltip("Save current registration, drawbars, amp/mic & MIDI settings as a User Preset");
    savePresetBtn_.onClick = [this]() { promptSaveUserPreset(); };
    canvas_.addAndMakeVisible(savePresetBtn_);

    styleActionBtn(deletePresetBtn_, false);
    deletePresetBtn_.setTooltip("Delete selected User Preset");
    deletePresetBtn_.onClick = [this]() { promptDeleteUserPreset(); };
    canvas_.addAndMakeVisible(deletePresetBtn_);

    refreshPresetList();

    auto styleHeaderBtn = [](juce::TextButton& b)
    {
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff18191d));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a351e));
        b.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xfff0e4ce));
        b.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xffffcf87));
    };

    // STAGE MICS view toggle button
    styleHeaderBtn(stageViewToggleBtn_);
    stageViewToggleBtn_.onClick = [this]()
    {
        const bool showStage = stageViewToggleBtn_.getToggleState();
        stageMicLayer_.setVisible(showStage);
        stageViewToggleBtn_.setButtonText(showStage ? "B-3 CONSOLE" : "STAGE MICS");
        if (showStage)
            stageMicLayer_.toFront(false);
    };
    canvas_.addAndMakeVisible(stageViewToggleBtn_);

    // LIVE RACK view toggle button (collapses keyboards & pedalboard for compact gigging rack)
    styleHeaderBtn(liveRackToggleBtn_);
    liveRackToggleBtn_.onClick = [this]()
    {
        isLiveRackMode_ = liveRackToggleBtn_.getToggleState();
        liveRackToggleBtn_.setButtonText(isLiveRackMode_ ? "FULL CONSOLE" : "LIVE RACK");
        applyWindowScaleAndMode();
    };
    canvas_.addAndMakeVisible(liveRackToggleBtn_);

    // Proportional Zoom Selector (75% .. 175%)
    zoomBox_.addItemList({ "75%", "100%", "125%", "150%", "175%" }, 1);
    zoomBox_.setSelectedId(2, juce::dontSendNotification); // 100%
    zoomBox_.onChange = [this]()
    {
        static constexpr std::array<float, 5> kScales = { 0.75f, 1.00f, 1.25f, 1.50f, 1.75f };
        const int idx = juce::jlimit(0, 4, zoomBox_.getSelectedId() - 1);
        zoomScale_ = kScales[static_cast<size_t>(idx)];
        applyWindowScaleAndMode();
    };
    canvas_.addAndMakeVisible(zoomBox_);

    masterKnob_.init(canvas_, apvts, Params::kMasterGain, "MASTER");
    splitSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kMidiSplit), "MIDI", "3-CH", "SPLIT", true);
    canvas_.addAndMakeVisible(*splitSwitch_);

    // Unified 38-Drawbar Deck (Upper A + Upper B + Pedals + Lower A + Lower B)
    dualDrawbarDeck_ = std::make_unique<UI::DualDrawbarDeck>(apvts);
    canvas_.addAndMakeVisible(*dualDrawbarDeck_);

    // Aged Bakelite Ivory Tilt-Rocker Switches (Percussion & Vibrato/Chorus)
    percOnSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kPercOn), "PERC", "OFF", "ON", true);
    percSoftSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kPercSoft), "VOL", "NORM", "SOFT", false);
    percFastSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kPercFast), "DECAY", "SLOW", "FAST", false);
    percThirdSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kPercThird), "HARM", "2ND", "3RD", false);
    upperVCSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kUpperVCOn), "UP V/C", "OFF", "ON", true);
    lowerVCSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kLowerVCOn), "LO V/C", "OFF", "ON", true);

    canvas_.addAndMakeVisible(*percOnSwitch_);
    canvas_.addAndMakeVisible(*percSoftSwitch_);
    canvas_.addAndMakeVisible(*percFastSwitch_);
    canvas_.addAndMakeVisible(*percThirdSwitch_);
    canvas_.addAndMakeVisible(*upperVCSwitch_);
    canvas_.addAndMakeVisible(*lowerVCSwitch_);

    vcModeBox_.addItemList({ "Off", "V1", "C1", "V2", "C2", "V3", "C3" }, 1);
    canvas_.addAndMakeVisible(vcModeBox_);
    vcModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, Params::kVCMode, vcModeBox_);

    // Tonewheel, AO-28 Preamp, Spring Reverb, Leslie 122/147 Knobs
    keyClickKnob_.init(canvas_, apvts, Params::kKeyClick,       "KEY CLICK");
    leakageKnob_.init(canvas_, apvts,  Params::kLeakage,        "LEAKAGE");
    driveKnob_.init(canvas_, apvts,    Params::kDrive,          "AO-28 DRIVE");
    swellKnob_.init(canvas_, apvts,    Params::kSwell,          "SWELL");
    idleMuteSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kIdleGeneratorMute), "IDLE", "RUN", "QUIET", true);
    canvas_.addAndMakeVisible(*idleMuteSwitch_);

    marshallConsoleSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kMarshallOn), "1962", "2x12", "AMP", true);
    canvas_.addAndMakeVisible(*marshallConsoleSwitch_);
    reverbMixKnob_.init(canvas_, apvts,   Params::kReverbMix,   "SPRING MIX");
    reverbDecayKnob_.init(canvas_, apvts, Params::kReverbDecay, "DECAY");
    micSpreadKnob_.init(canvas_, apvts,   Params::kMicSpread,   "MIC SPREAD");
    micDistKnob_.init(canvas_, apvts,     Params::kMicDistance, "DISTANCE");
    hornBalKnob_.init(canvas_, apvts,     Params::kHornBalance, "HORN/DRUM");

    leslieOnSwitch_ = std::make_unique<UI::TiltRockerSwitch>(
        *apvts.getParameter(Params::kLeslieOn), "LESLIE", "BYP", "ON", true);
    canvas_.addAndMakeVisible(*leslieOnSwitch_);

    halfMoonSwitch_ = std::make_unique<UI::HalfMoonSwitch>(
        *apvts.getParameter(Params::kLeslieSpeed));
    canvas_.addAndMakeVisible(*halfMoonSwitch_);
    canvas_.addAndMakeVisible(leslieVisualizer_);

    // Reverse-Color Preset Octaves (C..B)
    upperPresetOctave_ = std::make_unique<UI::PresetOctaveComponent>(
        *apvts.getParameter(Params::kUpperPresetKey), "UPPER PRESETS");
    upperPresetOctave_->onPresetKeyLatched = [this](int keyIdx, bool saveSlot)
    {
        if (saveSlot)
            processor_.saveManualPresetSlot(true, keyIdx);
        else if (keyIdx >= 1 && keyIdx <= 9)
            processor_.recallManualPresetSlot(true, keyIdx);
    };
    canvas_.addAndMakeVisible(*upperPresetOctave_);

    lowerPresetOctave_ = std::make_unique<UI::PresetOctaveComponent>(
        *apvts.getParameter(Params::kLowerPresetKey), "LOWER PRESETS");
    lowerPresetOctave_->onPresetKeyLatched = [this](int keyIdx, bool saveSlot)
    {
        if (saveSlot)
            processor_.saveManualPresetSlot(false, keyIdx);
        else if (keyIdx >= 1 && keyIdx <= 9)
            processor_.recallManualPresetSlot(false, keyIdx);
    };
    canvas_.addAndMakeVisible(*lowerPresetOctave_);

    // On-screen interactive manuals & 25-note pedalboard
    auto onNoteOn  = [this](int ch, int note, float vel) { processor_.injectNoteOn(ch, note, vel); };
    auto onNoteOff = [this](int ch, int note)            { processor_.injectNoteOff(ch, note); };

    upperKeyboard_ = std::make_unique<UI::OrganKeyboard>(36, 61, 1, false, onNoteOn, onNoteOff);
    lowerKeyboard_ = std::make_unique<UI::OrganKeyboard>(36, 61, 2, false, onNoteOn, onNoteOff);
    pedalKeyboard_ = std::make_unique<UI::OrganKeyboard>(36, 25, 3, true,  onNoteOn, onNoteOff);

    canvas_.addAndMakeVisible(*upperKeyboard_);
    canvas_.addAndMakeVisible(*lowerKeyboard_);
    canvas_.addAndMakeVisible(*pedalKeyboard_);

    // Always-visible Performance Controls flanking the 25-Note Pedalboard (active in both Console & Stage Views)
    lowerHalfMoonSwitch_ = std::make_unique<UI::HalfMoonSwitch>(
        *apvts.getParameter(Params::kLeslieSpeed));
    swellPedal_ = std::make_unique<UI::SwellPedalComponent>(
        *apvts.getParameter(Params::kSwell));
    canvas_.addAndMakeVisible(*lowerHalfMoonSwitch_);
    canvas_.addAndMakeVisible(*swellPedal_);

    // Selectable MIDI Channel Routing (Omni, Ch 1..16) for Upper, Lower & Pedals
    {
        juce::StringArray chItems { "Omni" };
        for (int ch = 1; ch <= 16; ++ch)
            chItems.add("Ch " + juce::String(ch));

        auto initMidiCh = [&](juce::ComboBox& box, juce::Label& lbl, const juce::String& title,
                              const char* paramId, const juce::String& tip,
                              std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
        {
            lbl.setText(title, juce::dontSendNotification);
            lbl.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            lbl.setColour(juce::Label::textColourId, UI::Palette::creamDim);
            lbl.setJustificationType(juce::Justification::centred);
            canvas_.addAndMakeVisible(lbl);

            box.addItemList(chItems, 1);
            box.setTooltip(tip);
            canvas_.addAndMakeVisible(box);
            att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, box);
        };

        initMidiCh(upperMidiChBox_, upperMidiChLabel_, "UPPER", Params::kUpperMidiCh,
                   "Upper Manual MIDI Channel (Omni receives all channels)", upperMidiChAttachment_);
        initMidiCh(lowerMidiChBox_, lowerMidiChLabel_, "LOWER", Params::kLowerMidiCh,
                   "Lower Manual MIDI Channel (Omni receives all channels)", lowerMidiChAttachment_);
        initMidiCh(pedalMidiChBox_, pedalMidiChLabel_, "PEDAL", Params::kPedalMidiCh,
                   "Pedalboard MIDI Channel (Omni receives all channels)", pedalMidiChAttachment_);
    }

    // Interactive Maple Stage overlay (hidden by default; toggled via STAGE MICS header button)
    canvas_.addChildComponent(stageMicLayer_);

    setResizable(true, true);
    applyWindowScaleAndMode();

    startTimerHz(30);
}

GreenOnionsAudioProcessorEditor::~GreenOnionsAudioProcessorEditor()
{
    stopTimer();
    if (presetDialog_ != nullptr)
    {
        presetDialog_->exitModalState(0);
        presetDialog_->setLookAndFeel(nullptr);
        presetDialog_.reset();
    }
    canvas_.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void GreenOnionsAudioProcessorEditor::refreshPresetList()
{
    presetBox_.clear(juce::dontSendNotification);

    presetBox_.addSectionHeading("FACTORY REGISTRATIONS");
    const auto& presets = Presets::getFactoryPresets();
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
        presetBox_.addItem(presets[static_cast<size_t>(i)].name, i + 1);

    userPresetNames_ = processor_.getUserPresetNames();
    if (!userPresetNames_.isEmpty())
    {
        presetBox_.addSeparator();
        presetBox_.addSectionHeading("USER PRESETS");
        for (int i = 0; i < userPresetNames_.size(); ++i)
            presetBox_.addItem(userPresetNames_[i], kUserPresetIdBase + i);
    }

    int selectedId = processor_.getCurrentProgram() + 1;
    if (processor_.isCurrentPresetUser())
    {
        const int idx = userPresetNames_.indexOf(processor_.getCurrentPresetName());
        if (idx >= 0)
            selectedId = kUserPresetIdBase + idx;
    }

    presetBox_.setSelectedId(selectedId, juce::dontSendNotification);
    deletePresetBtn_.setEnabled(processor_.isCurrentPresetUser());
}

void GreenOnionsAudioProcessorEditor::promptSaveUserPreset()
{
    if (presetDialog_ != nullptr)
    {
        presetDialog_->exitModalState(0);
        presetDialog_->setLookAndFeel(nullptr);
        presetDialog_.reset();
    }

    const juce::String initialName = processor_.isCurrentPresetUser()
        ? processor_.getCurrentPresetName()
        : "My Registration";

    presetDialog_ = std::make_unique<juce::AlertWindow>(
        "Save User Preset",
        "Enter a name for this registration (saved to your Green Onions User Presets folder):",
        juce::MessageBoxIconType::NoIcon,
        this);
    presetDialog_->setLookAndFeel(&lookAndFeel_);
    presetDialog_->addTextEditor("presetName", initialName, "Preset Name:");
    presetDialog_->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
    presetDialog_->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<GreenOnionsAudioProcessorEditor> safeThis(this);
    presetDialog_->enterModalState(
        true,
        juce::ModalCallbackFunction::create([safeThis](int result)
        {
            if (safeThis == nullptr || safeThis->presetDialog_ == nullptr)
                return;

            if (result == 1)
            {
                const auto name = safeThis->presetDialog_->getTextEditorContents("presetName").trim();
                if (name.isNotEmpty() && safeThis->processor_.saveUserPreset(name))
                    safeThis->refreshPresetList();
            }

            safeThis->presetDialog_->setLookAndFeel(nullptr);
            safeThis->presetDialog_.reset();
        }),
        false);
}

void GreenOnionsAudioProcessorEditor::promptDeleteUserPreset()
{
    if (!processor_.isCurrentPresetUser())
        return;

    if (presetDialog_ != nullptr)
    {
        presetDialog_->exitModalState(0);
        presetDialog_->setLookAndFeel(nullptr);
        presetDialog_.reset();
    }

    const juce::String targetName = processor_.getCurrentPresetName();
    presetDialog_ = std::make_unique<juce::AlertWindow>(
        "Delete User Preset",
        "Delete user preset \"" + targetName + "\"?",
        juce::MessageBoxIconType::WarningIcon,
        this);
    presetDialog_->setLookAndFeel(&lookAndFeel_);
    presetDialog_->addButton("Delete", 1, juce::KeyPress(juce::KeyPress::returnKey));
    presetDialog_->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<GreenOnionsAudioProcessorEditor> safeThis(this);
    presetDialog_->enterModalState(
        true,
        juce::ModalCallbackFunction::create([safeThis, targetName](int result)
        {
            if (safeThis == nullptr || safeThis->presetDialog_ == nullptr)
                return;

            if (result == 1 && safeThis->processor_.deleteUserPreset(targetName))
                safeThis->refreshPresetList();

            safeThis->presetDialog_->setLookAndFeel(nullptr);
            safeThis->presetDialog_.reset();
        }),
        false);
}

void GreenOnionsAudioProcessorEditor::applyWindowScaleAndMode()
{
    const int baseH = isLiveRackMode_ ? kLiveRackHeight : kFullHeight;
    const double aspect = static_cast<double>(kBaseWidth) / static_cast<double>(baseH);

    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio(aspect);
        c->setSizeLimits(static_cast<int>(kBaseWidth * 0.75f), static_cast<int>(baseH * 0.75f),
                         static_cast<int>(kBaseWidth * 1.75f), static_cast<int>(baseH * 1.75f));
    }

    setSize(static_cast<int>(std::round(kBaseWidth * zoomScale_)),
            static_cast<int>(std::round(baseH * zoomScale_)));
}

void GreenOnionsAudioProcessorEditor::resized()
{
    const int baseH = isLiveRackMode_ ? kLiveRackHeight : kFullHeight;
    const float sx  = static_cast<float>(getWidth())  / static_cast<float>(kBaseWidth);
    const float sy  = static_cast<float>(getHeight()) / static_cast<float>(baseH);
    const float s   = juce::jmin(sx, sy);

    canvas_.setTransform(juce::AffineTransform::scale(s));
    canvas_.setBounds(0, 0, kBaseWidth, baseH);
}

void GreenOnionsAudioProcessorEditor::timerCallback()
{
    const auto& t = processor_.getTelemetry();
    const int u = t.upperHeld.load(std::memory_order_relaxed);
    const int l = t.lowerHeld.load(std::memory_order_relaxed);
    const int p = t.pedalHeld.load(std::memory_order_relaxed);
    if (u != upperHeld_ || l != lowerHeld_ || p != pedalHeld_)
    {
        upperHeld_ = u;
        lowerHeld_ = l;
        pedalHeld_ = p;
        canvas_.repaint(ledArea_);
    }
}

void GreenOnionsAudioProcessorEditor::paintCanvas(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    constexpr int railW = 18;

    // 1. Cached Procedural American Black Walnut Cabinet Body
    g.drawImageAt(lookAndFeel_.getWalnutTexture(bounds.getWidth(), bounds.getHeight()), 0, 0);

    // 2. Cached Procedural Brushed Dark Anodized Aluminum Console Deck
    const auto deckBounds = bounds.reduced(railW, 0);
    g.drawImageAt(lookAndFeel_.getAnodizedMetalTexture(deckBounds.getWidth(), deckBounds.getHeight()),
                  deckBounds.getX(), deckBounds.getY());

    // Inner walnut rail shadows
    g.setColour(juce::Colours::black.withAlpha(0.65f));
    g.drawVerticalLine(railW, 0.0f, static_cast<float>(bounds.getHeight()));
    g.drawVerticalLine(bounds.getRight() - railW - 1, 0.0f, static_cast<float>(bounds.getHeight()));

    // Top Header Banner
    const auto header = bounds.reduced(railW, 0).withHeight(52);
    juce::ColourGradient hdrGrad(juce::Colour(0xff26201c), 0.0f, 0.0f,
                                 juce::Colour(0xff141210), 0.0f, 52.0f, false);
    g.setGradientFill(hdrGrad);
    g.fillRect(header);
    g.setColour(UI::Palette::amber.withAlpha(0.45f));
    g.drawHorizontalLine(header.getBottom() - 1, static_cast<float>(header.getX()), static_cast<float>(header.getRight()));

    // Brand Nameplate
    g.setColour(UI::Palette::amberGlow);
    g.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    g.drawText("GREEN ONIONS", header.getX() + 12, 5, 200, 24, juce::Justification::centredLeft, false);

    g.setColour(UI::Palette::creamDim);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("SIGNAL FORGE AUDIO LABS  •  B-3 TONEWHEELS",
               header.getX() + 12, 30, 265, 14, juce::Justification::centredLeft, false);

    // MIDI Channel Activity LEDs
    auto drawLed = [&](juce::Rectangle<int> r, const juce::String& lbl, bool active)
    {
        const float cx = static_cast<float>(r.getCentreX());
        const float cy = static_cast<float>(r.getY() + 10);
        g.setColour(active ? UI::Palette::greenAccent : juce::Colour(0xff2b3029));
        g.fillEllipse(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
        if (active)
        {
            g.setColour(UI::Palette::greenAccent.withAlpha(0.35f));
            g.fillEllipse(cx - 7.5f, cy - 7.5f, 15.0f, 15.0f);
        }
        g.setColour(UI::Palette::creamDim);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText(lbl, r.withTop(r.getY() + 17), juce::Justification::centred, false);
    };

    const int ledW = ledArea_.getWidth() / 3;
    drawLed(ledArea_.withWidth(ledW),                                    "UP",  upperHeld_ > 0);
    drawLed(ledArea_.withX(ledArea_.getX() + ledW).withWidth(ledW),      "LO",  lowerHeld_ > 0);
    drawLed(ledArea_.withX(ledArea_.getX() + 2 * ledW).withWidth(ledW),  "PED", pedalHeld_ > 0);

    // Draw brushed dark anodized module panels
    auto drawSection = [&](juce::Rectangle<int> r, const juce::String& title)
    {
        UI::VintageLookAndFeel::drawAnodizedPanel(g, r.toFloat(), 5.0f);

        g.setColour(UI::Palette::amber);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(title, r.withHeight(18).reduced(8, 2), juce::Justification::centredLeft, false);
    };

    drawSection(percVcFrame_,  "HARMONIC PERCUSSION & SCANNER V/C");
    drawSection(preampFrame_,  "TONEWHEEL & AO-28 PREAMP");
    drawSection(reverbFrame_,  "REVERB & 2x12 AMP");
    drawSection(leslieFrame_,  "LESLIE 122 / 147 ROTARY SPEAKER (CC1 / CC64)");
    if (!midiChFrame_.isEmpty())
        drawSection(midiChFrame_, "MIDI CHANNELS");
}

void GreenOnionsAudioProcessorEditor::layoutCanvas(juce::Rectangle<int> bounds)
{
    constexpr int railW = 18;
    auto area = bounds.reduced(railW, 0);

    // 1. Header Bar (52px)
    auto header = area.removeFromTop(52).reduced(8, 4);
    header.removeFromLeft(270); // Title nameplate
    masterKnob_.layout(header.removeFromRight(50));
    header.removeFromRight(5);
    splitSwitch_->setBounds(header.removeFromRight(48));
    header.removeFromRight(6);
    zoomBox_.setBounds(header.removeFromRight(64).reduced(0, 8));
    header.removeFromRight(6);
    liveRackToggleBtn_.setBounds(header.removeFromRight(84).reduced(0, 8));
    header.removeFromRight(6);
    stageViewToggleBtn_.setBounds(header.removeFromRight(84).reduced(0, 8));
    header.removeFromRight(6);
    ledArea_ = header.removeFromRight(92);
    header.removeFromRight(6);

    const int consoleW = std::min(205, std::max(140, (header.getWidth() - 6) * 42 / 100));
    auto consoleCol = header.removeFromLeft(consoleW);
    organModelLabel_.setBounds(consoleCol.removeFromTop(12));
    organModelBox_.setBounds(consoleCol.reduced(0, 2));

    header.removeFromLeft(6);
    auto presetCol = header;
    presetLabel_.setBounds(presetCol.removeFromTop(12));
    auto presetRow = presetCol.reduced(0, 2);
    deletePresetBtn_.setBounds(presetRow.removeFromRight(32));
    presetRow.removeFromRight(3);
    savePresetBtn_.setBounds(presetRow.removeFromRight(42));
    presetRow.removeFromRight(4);
    presetBox_.setBounds(presetRow);

    const int stageTopY = area.getY();
    area.reduce(8, 6);

    // 2. Unified 38-Drawbar B-3 Deck
    const int drawbarH = 198;
    dualDrawbarDeck_->setBounds(area.removeFromTop(drawbarH));

    area.removeFromTop(6);

    // 3. Control Deck
    const int ctrlH = 172;
    auto ctrlRow = area.removeFromTop(ctrlH);
    const int stageBottomY = ctrlRow.getBottom() + 6;

    // Position the Interactive Maple Stage overlay over the upper deck ONLY,
    // keeping both 61-key manuals, reverse-color preset octaves, and 25-note pedalboard
    // 100% visible and playable while adjusting stage mics & amps!
    stageMicLayer_.setBounds(railW, stageTopY, bounds.getWidth() - 2 * railW, stageBottomY - stageTopY);
    const int cw = ctrlRow.getWidth();

    percVcFrame_ = ctrlRow.removeFromLeft(static_cast<int>(cw * 0.28f));
    ctrlRow.removeFromLeft(8);
    preampFrame_ = ctrlRow.removeFromLeft(static_cast<int>(cw * 0.24f));
    ctrlRow.removeFromLeft(8);
    reverbFrame_ = ctrlRow.removeFromLeft(static_cast<int>(cw * 0.14f));
    ctrlRow.removeFromLeft(8);
    leslieFrame_ = ctrlRow;

    // Layout Percussion & V/C Tilt-Rocker Switches
    {
        auto inner = percVcFrame_.withTrimmedTop(20).reduced(6, 4);
        auto topR  = inner.removeFromTop(inner.getHeight() / 2);
        const int swW = topR.getWidth() / 4;
        percOnSwitch_->setBounds(topR.removeFromLeft(swW));
        percSoftSwitch_->setBounds(topR.removeFromLeft(swW));
        percFastSwitch_->setBounds(topR.removeFromLeft(swW));
        percThirdSwitch_->setBounds(topR);

        inner.removeFromTop(3);
        upperVCSwitch_->setBounds(inner.removeFromLeft(swW));
        lowerVCSwitch_->setBounds(inner.removeFromLeft(swW));
        vcModeBox_.setBounds(inner.reduced(6, 15));
    }

    // Layout Tonewheel & AO-28 Preamp (Left: 2x2 knobs; Right: classic RUN / QUIET Idle Gate rocker)
    {
        auto inner = preampFrame_.withTrimmedTop(20).reduced(6, 4);
        auto rightCol = inner.removeFromRight(52);
        idleMuteSwitch_->setBounds(rightCol.withSizeKeepingCentre(48, 72));
        inner.removeFromRight(4);

        auto topR  = inner.removeFromTop(inner.getHeight() / 2);
        keyClickKnob_.layout(topR.removeFromLeft(topR.getWidth() / 2));
        leakageKnob_.layout(topR);
        driveKnob_.layout(inner.removeFromLeft(inner.getWidth() / 2));
        swellKnob_.layout(inner);
    }

    // Layout Spring Reverb & Parallel Marshall 1962 2x12 Quick Switch
    {
        auto inner = reverbFrame_.withTrimmedTop(20).reduced(6, 4);
        auto rightCol = inner.removeFromRight(48);
        marshallConsoleSwitch_->setBounds(rightCol.withSizeKeepingCentre(46, 72));
        inner.removeFromRight(4);
        reverbMixKnob_.layout(inner.removeFromTop(inner.getHeight() / 2));
        reverbDecayKnob_.layout(inner);
    }

    // Layout Leslie 122/147 (Left: ON + Half-Moon + 3 Mic/Balance Knobs; Right: 60 fps Top-Down Rotor Chamber)
    {
        auto inner = leslieFrame_.withTrimmedTop(20).reduced(6, 4);
        const int visW = std::max(145, static_cast<int>(inner.getWidth() * 0.44f));
        leslieVisualizer_.setBounds(inner.removeFromRight(visW));
        inner.removeFromRight(6);

        auto topR = inner.removeFromTop(static_cast<int>(inner.getHeight() * 0.52f));
        leslieOnSwitch_->setBounds(topR.removeFromLeft(46));
        topR.removeFromLeft(4);
        halfMoonSwitch_->setBounds(topR);

        inner.removeFromTop(4);
        const int kw = inner.getWidth() / 3;
        micSpreadKnob_.layout(inner.removeFromLeft(kw));
        micDistKnob_.layout(inner.removeFromLeft(kw));
        hornBalKnob_.layout(inner);
    }

    // 4. Reverse-Color Preset Octaves + Dual 61-Key Manuals + 25-Note Pedalboard
    //    + Flanking Half-Moon Leslie Switch (left) & Expression Swell Pedal (right)
    const bool showKeyboards = !isLiveRackMode_;
    upperPresetOctave_->setVisible(showKeyboards);
    lowerPresetOctave_->setVisible(showKeyboards);
    upperKeyboard_->setVisible(showKeyboards);
    lowerKeyboard_->setVisible(showKeyboards);
    pedalKeyboard_->setVisible(showKeyboards);
    lowerHalfMoonSwitch_->setVisible(showKeyboards);
    swellPedal_->setVisible(showKeyboards);
    upperMidiChBox_.setVisible(showKeyboards);
    lowerMidiChBox_.setVisible(showKeyboards);
    pedalMidiChBox_.setVisible(showKeyboards);
    upperMidiChLabel_.setVisible(showKeyboards);
    lowerMidiChLabel_.setVisible(showKeyboards);
    pedalMidiChLabel_.setVisible(showKeyboards);

    if (showKeyboards)
    {
        area.removeFromTop(6);
        const int kbTotalH      = area.getHeight();
        const int manualH       = static_cast<int>(kbTotalH * 0.35f);
        const int presetOctaveW = std::max(148, static_cast<int>(area.getWidth() * 0.165f));

        auto upperRow = area.removeFromTop(manualH);
        upperPresetOctave_->setBounds(upperRow.removeFromLeft(presetOctaveW));
        upperRow.removeFromLeft(6);
        upperKeyboard_->setBounds(upperRow);

        area.removeFromTop(4);

        auto lowerRow = area.removeFromTop(manualH);
        lowerPresetOctave_->setBounds(lowerRow.removeFromLeft(presetOctaveW));
        lowerRow.removeFromLeft(6);
        lowerKeyboard_->setBounds(lowerRow);

        area.removeFromTop(4);
        auto pedalRow = area;
        const int sideW = 175;
        lowerHalfMoonSwitch_->setBounds(pedalRow.removeFromLeft(sideW).reduced(8, 4));
        swellPedal_->setBounds(pedalRow.removeFromRight(sideW).reduced(8, 2));

        pedalRow.removeFromRight(6);
        midiChFrame_ = pedalRow.removeFromRight(234).reduced(0, 2);
        {
            auto inner = midiChFrame_.withTrimmedTop(19).reduced(6, 4);
            const int gap = 5;
            const int colW = (inner.getWidth() - 2 * gap) / 3;

            auto layoutCol = [&](juce::Rectangle<int> col, juce::Label& lbl, juce::ComboBox& box)
            {
                lbl.setBounds(col.removeFromTop(14));
                col.removeFromTop(2);
                box.setBounds(col.withSizeKeepingCentre(col.getWidth(), std::min(24, col.getHeight())));
            };

            layoutCol(inner.removeFromLeft(colW), upperMidiChLabel_, upperMidiChBox_);
            inner.removeFromLeft(gap);
            layoutCol(inner.removeFromLeft(colW), lowerMidiChLabel_, lowerMidiChBox_);
            inner.removeFromLeft(gap);
            layoutCol(inner,                      pedalMidiChLabel_, pedalMidiChBox_);
        }

        pedalKeyboard_->setBounds(pedalRow.reduced(10, 0));
    }
    else
    {
        midiChFrame_ = {};
    }
}

} // namespace GreenOnions
