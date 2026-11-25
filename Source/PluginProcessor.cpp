#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace GreenOnions
{

GreenOnionsAudioProcessor::GreenOnionsAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "GREEN_ONIONS_STATE", Params::createParameterLayout())
{
    for (size_t i = 0; i < 9; ++i)
    {
        upperDbParams_[i]  = apvts_.getRawParameterValue(Params::kUpperDrawbarIds[i]);
        upperBDbParams_[i] = apvts_.getRawParameterValue(Params::kUpperBDrawbarIds[i]);
        lowerDbParams_[i]  = apvts_.getRawParameterValue(Params::kLowerDrawbarIds[i]);
        lowerBDbParams_[i] = apvts_.getRawParameterValue(Params::kLowerBDrawbarIds[i]);
    }
    pedalDbParams_[0] = apvts_.getRawParameterValue(Params::kPedalDrawbarIds[0]);
    pedalDbParams_[1] = apvts_.getRawParameterValue(Params::kPedalDrawbarIds[1]);

    upperPresetKeyParam_ = apvts_.getRawParameterValue(Params::kUpperPresetKey);
    lowerPresetKeyParam_ = apvts_.getRawParameterValue(Params::kLowerPresetKey);

    // 9 Classic Hammond B-3 Reverse-Color Preset Key Registrations (C#..A)
    upperPresetSlots_ = {{
        { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, // C# (1): Jimmy Smith Jazz 888
        { 8, 8, 8, 8, 0, 0, 0, 0, 0 }, // D  (2): Green Onions
        { 8, 0, 8, 8, 0, 0, 0, 0, 8 }, // D# (3): Whiter Shade
        { 8, 8, 8, 8, 8, 8, 8, 8, 8 }, // E  (4): Full Gospel / All Out
        { 0, 0, 8, 8, 8, 0, 0, 0, 0 }, // F  (5): Warm Comping
        { 8, 8, 8, 8, 6, 0, 0, 0, 0 }, // F# (6): Jon Lord Rock
        { 8, 0, 0, 0, 0, 0, 0, 0, 8 }, // G  (7): Squabblin' Funk
        { 8, 4, 8, 8, 4, 8, 4, 4, 8 }, // G# (8): Bright Cathedral
        { 8, 0, 4, 0, 2, 0, 1, 0, 0 }  // A  (9): Soft Flute
    }};
    lowerPresetSlots_ = {{
        { 0, 0, 8, 3, 8, 0, 0, 0, 0 }, // C# (1): Jazz Comp
        { 0, 0, 8, 8, 8, 0, 0, 0, 0 }, // D  (2): Soul Comp
        { 8, 0, 8, 0, 0, 0, 0, 0, 0 }, // D# (3): Mellow 16'+8'
        { 8, 4, 8, 6, 4, 2, 0, 0, 0 }, // E  (4): Full Gospel Lower
        { 0, 0, 6, 4, 0, 0, 0, 0, 0 }, // F  (5): Soft Pad
        { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, // F# (6): Rock Chords
        { 0, 0, 8, 8, 0, 0, 0, 0, 0 }, // G  (7): Funk Stabs
        { 8, 6, 8, 6, 6, 4, 4, 4, 4 }, // G# (8): Cathedral Accomp
        { 0, 0, 4, 4, 2, 0, 0, 0, 0 }  // A  (9): Whisper Comp
    }};

    percOnParam_      = apvts_.getRawParameterValue(Params::kPercOn);
    percSoftParam_    = apvts_.getRawParameterValue(Params::kPercSoft);
    percFastParam_    = apvts_.getRawParameterValue(Params::kPercFast);
    percThirdParam_   = apvts_.getRawParameterValue(Params::kPercThird);
    vcModeParam_      = apvts_.getRawParameterValue(Params::kVCMode);
    upperVCOnParam_   = apvts_.getRawParameterValue(Params::kUpperVCOn);
    lowerVCOnParam_   = apvts_.getRawParameterValue(Params::kLowerVCOn);
    keyClickParam_    = apvts_.getRawParameterValue(Params::kKeyClick);
    leakageParam_     = apvts_.getRawParameterValue(Params::kLeakage);
    idleGenMuteParam_ = apvts_.getRawParameterValue(Params::kIdleGeneratorMute);
    swellParam_       = apvts_.getRawParameterValue(Params::kSwell);
    driveParam_       = apvts_.getRawParameterValue(Params::kDrive);
    reverbMixParam_   = apvts_.getRawParameterValue(Params::kReverbMix);
    reverbDecayParam_ = apvts_.getRawParameterValue(Params::kReverbDecay);
    leslieOnParam_       = apvts_.getRawParameterValue(Params::kLeslieOn);
    leslieCabModelParam_ = apvts_.getRawParameterValue(Params::kLeslieCabinetModel);
    leslieSpeedParam_    = apvts_.getRawParameterValue(Params::kLeslieSpeed);
    micSpreadParam_   = apvts_.getRawParameterValue(Params::kMicSpread);
    micDistanceParam_ = apvts_.getRawParameterValue(Params::kMicDistance);
    hornBalanceParam_ = apvts_.getRawParameterValue(Params::kHornBalance);
    masterGainParam_  = apvts_.getRawParameterValue(Params::kMasterGain);
    midiSplitParam_   = apvts_.getRawParameterValue(Params::kMidiSplit);
    upperMidiChParam_ = apvts_.getRawParameterValue(Params::kUpperMidiCh);
    lowerMidiChParam_ = apvts_.getRawParameterValue(Params::kLowerMidiCh);
    pedalMidiChParam_ = apvts_.getRawParameterValue(Params::kPedalMidiCh);
    organModelParam_  = apvts_.getRawParameterValue(Params::kOrganModel);
    micLTypeParam_    = apvts_.getRawParameterValue(Params::kMicLType);
    micRTypeParam_    = apvts_.getRawParameterValue(Params::kMicRType);
    micLXParam_       = apvts_.getRawParameterValue(Params::kMicLX);
    micLYParam_       = apvts_.getRawParameterValue(Params::kMicLY);
    micRXParam_       = apvts_.getRawParameterValue(Params::kMicRX);
    micRYParam_       = apvts_.getRawParameterValue(Params::kMicRY);
    marshallOnParam_   = apvts_.getRawParameterValue(Params::kMarshallOn);
    marshallGainParam_ = apvts_.getRawParameterValue(Params::kMarshallGain);
    marshallToneParam_ = apvts_.getRawParameterValue(Params::kMarshallTone);
    marshallMixParam_  = apvts_.getRawParameterValue(Params::kMarshallMix);
    ampSm57XParam_     = apvts_.getRawParameterValue(Params::kAmpSm57X);
    ampSm57YParam_     = apvts_.getRawParameterValue(Params::kAmpSm57Y);
    ampC414XParam_     = apvts_.getRawParameterValue(Params::kAmpC414X);
    ampC414YParam_     = apvts_.getRawParameterValue(Params::kAmpC414Y);
}

void GreenOnionsAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    tonewheelEngine_.prepare(sampleRate);
    scannerVibrato_.prepare(sampleRate);
    preampSwell_.prepare(sampleRate);
    springReverb_.prepare(sampleRate);
    leslie122_.prepare(sampleRate);
    micEqL_.prepare(sampleRate);
    micEqR_.prepare(sampleRate);
    marshallAmp_.prepare(sampleRate);
    ampMicSm57_.prepare(sampleRate);
    ampMicC414_.prepare(sampleRate);

    // Acoustic arrival delay buffer for movable Bluesbreaker SM57 & C414 mics
    currentSampleRate_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
    ampMicDelayBuf_.fill(0.0f);
    ampMicDelayWrite_ = 0;

    uiMidiCollector_.reset(sampleRate);
    sustainWasDown_ = false;
}

void GreenOnionsAudioProcessor::releaseResources()
{
    tonewheelEngine_.reset();
    scannerVibrato_.reset();
    preampSwell_.reset();
    springReverb_.reset();
    leslie122_.reset();
}

bool GreenOnionsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void GreenOnionsAudioProcessor::injectNoteOn(int manualIndex, int midiNote, float velocity)
{
    // Route on-screen keyboard clicks using the manual's configured MIDI channel (or manualIndex if Omni)
    int ch = manualIndex;
    if (manualIndex == 1 && upperMidiChParam_)      { const int c = static_cast<int>(upperMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 1; }
    else if (manualIndex == 2 && lowerMidiChParam_) { const int c = static_cast<int>(lowerMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 2; }
    else if (manualIndex == 3 && pedalMidiChParam_) { const int c = static_cast<int>(pedalMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 3; }

    auto msg = juce::MidiMessage::noteOn(ch, midiNote, velocity);
    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiMidiCollector_.addMessageToQueue(msg);
}

void GreenOnionsAudioProcessor::injectNoteOff(int manualIndex, int midiNote)
{
    int ch = manualIndex;
    if (manualIndex == 1 && upperMidiChParam_)      { const int c = static_cast<int>(upperMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 1; }
    else if (manualIndex == 2 && lowerMidiChParam_) { const int c = static_cast<int>(lowerMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 2; }
    else if (manualIndex == 3 && pedalMidiChParam_) { const int c = static_cast<int>(pedalMidiChParam_->load() + 0.5f); ch = (c > 0) ? c : 3; }

    auto msg = juce::MidiMessage::noteOff(ch, midiNote);
    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiMidiCollector_.addMessageToQueue(msg);
}

void GreenOnionsAudioProcessor::saveManualPresetSlot(bool isUpper, int keyIndex)
{
    if (keyIndex < 1 || keyIndex > 9) return;
    const size_t slot = static_cast<size_t>(keyIndex - 1);
    const int curKey  = static_cast<int>((isUpper ? upperPresetKeyParam_ : lowerPresetKeyParam_)->load() + 0.5f);
    const bool useB   = (curKey == 10);

    auto& dst = isUpper ? upperPresetSlots_[slot] : lowerPresetSlots_[slot];
    const auto& src = isUpper ? (useB ? upperBDbParams_ : upperDbParams_)
                              : (useB ? lowerBDbParams_ : lowerDbParams_);
    for (size_t i = 0; i < 9; ++i)
        dst[i] = src[i]->load();
}

void GreenOnionsAudioProcessor::recallManualPresetSlot(bool isUpper, int keyIndex)
{
    if (keyIndex < 1 || keyIndex > 9) return;
    const auto& src = isUpper ? upperPresetSlots_[static_cast<size_t>(keyIndex - 1)]
                              : lowerPresetSlots_[static_cast<size_t>(keyIndex - 1)];
    const auto& ids = isUpper ? Params::kUpperDrawbarIds : Params::kLowerDrawbarIds;
    for (size_t i = 0; i < 9; ++i)
        setParamNormalizedFromAudioThread(ids[i], src[i]);
}

void GreenOnionsAudioProcessor::setParamNormalizedFromAudioThread(const char* paramId, float plainValue)
{
    if (auto* p = apvts_.getParameter(paramId))
        p->setValueNotifyingHost(p->convertTo0to1(plainValue));
}

void GreenOnionsAudioProcessor::handleMidiMessage(const juce::MidiMessage& msg, bool splitMode,
                                                  int upperCh, int lowerCh, int pedalCh)
{
    const int ch = msg.getChannel(); // 1..16
    auto matches = [](int incoming, int configured) noexcept -> bool
    {
        return configured == 0 || incoming == configured;
    };

    const bool isUpper = matches(ch, upperCh);
    const bool isLower = matches(ch, lowerCh);
    const bool isPedal = matches(ch, pedalCh);

    if (msg.isNoteOn())
    {
        const int note  = msg.getNoteNumber();
        const float vel = msg.getFloatVelocity();

        // Authentic B-3 Reverse-Color Preset Octave via MIDI C1..B1 (MIDI 24..35)
        if ((isUpper || isLower) && note >= 24 && note <= 35)
        {
            const int presetKey = note - 24;
            if (isUpper)
            {
                setParamNormalizedFromAudioThread(Params::kUpperPresetKey, static_cast<float>(presetKey));
                if (presetKey >= 1 && presetKey <= 9)
                    recallManualPresetSlot(true, presetKey);
            }
            if (isLower)
            {
                setParamNormalizedFromAudioThread(Params::kLowerPresetKey, static_cast<float>(presetKey));
                if (presetKey >= 1 && presetKey <= 9)
                    recallManualPresetSlot(false, presetKey);
            }
            return;
        }

        if (splitMode && isUpper)
        {
            if (note < 48)
                tonewheelEngine_.noteOnPedal(note, vel);
            else if (note < 60)
                tonewheelEngine_.noteOnLower(note, vel);
            else
                tonewheelEngine_.noteOnUpper(note, vel);
        }
        else
        {
            if (isUpper) tonewheelEngine_.noteOnUpper(note, vel);
            if (isLower) tonewheelEngine_.noteOnLower(note, vel);
            if (isPedal) tonewheelEngine_.noteOnPedal(note, vel);
        }
    }
    else if (msg.isNoteOff())
    {
        const int note = msg.getNoteNumber();

        // Ignore Note-Off on C1..B1 (MIDI 24..35) so the mechanical preset key stays latched down
        if ((isUpper || isLower) && note >= 24 && note <= 35)
            return;

        if (splitMode && isUpper)
        {
            if (note < 48)
                tonewheelEngine_.noteOffPedal(note);
            else if (note < 60)
                tonewheelEngine_.noteOffLower(note);
            else
                tonewheelEngine_.noteOffUpper(note);
        }
        else
        {
            if (isUpper) tonewheelEngine_.noteOffUpper(note);
            if (isLower) tonewheelEngine_.noteOffLower(note);
            if (isPedal) tonewheelEngine_.noteOffPedal(note);
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        tonewheelEngine_.allNotesOff();
    }
    else if (msg.isController())
    {
        const int cc  = msg.getControllerNumber();
        const int val = msg.getControllerValue(); // 0..127

        if (cc == 1) // Mod Wheel -> Leslie Speed (Chorale / Brake / Tremolo)
        {
            const float mode = (val < 43) ? 0.0f : (val < 85 ? 1.0f : 2.0f);
            setParamNormalizedFromAudioThread(Params::kLeslieSpeed, mode);
        }
        else if (cc == 64) // Sustain Pedal -> Toggle Leslie Chorale <-> Tremolo on press
        {
            const bool isDown = (val >= 64);
            if (isDown && !sustainWasDown_)
            {
                const int cur = static_cast<int>(leslieSpeedParam_->load() + 0.5f);
                const float next = (cur == 2) ? 0.0f : 2.0f;
                setParamNormalizedFromAudioThread(Params::kLeslieSpeed, next);
            }
            sustainWasDown_ = isDown;
        }
        else if (cc == 11) // Expression / Swell Pedal
        {
            setParamNormalizedFromAudioThread(Params::kSwell, static_cast<float>(val) / 127.0f);
        }
        else if (cc >= 12 && cc <= 20) // Upper Drawbars 16'..1'
        {
            const size_t idx = static_cast<size_t>(cc - 12);
            const float step = std::round((static_cast<float>(val) / 127.0f) * 8.0f);
            setParamNormalizedFromAudioThread(Params::kUpperDrawbarIds[idx], step);
        }
        else if (cc >= 21 && cc <= 29) // Lower Drawbars 16'..1'
        {
            const size_t idx = static_cast<size_t>(cc - 21);
            const float step = std::round((static_cast<float>(val) / 127.0f) * 8.0f);
            setParamNormalizedFromAudioThread(Params::kLowerDrawbarIds[idx], step);
        }
        else if (cc == 30 || cc == 31) // Pedal Drawbars 16', 8'
        {
            const size_t idx = static_cast<size_t>(cc - 30);
            const float step = std::round((static_cast<float>(val) / 127.0f) * 8.0f);
            setParamNormalizedFromAudioThread(Params::kPedalDrawbarIds[idx], step);
        }
    }
}

void GreenOnionsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    buffer.clear();

    // Merge on-screen keyboard MIDI events into the block's MIDI buffer
    uiMidiCollector_.removeNextBlockOfMessages(midiMessages, numSamples);

    // Snapshot parameters once per block (resolving Reverse-Color Preset Keys: 0=Cancel, 1..9=Slot, 10=Set B, 11=Set A)
    DSP::TonewheelEngine::Parameters twParams;
    const int upperKey = static_cast<int>(upperPresetKeyParam_->load() + 0.5f);
    const int lowerKey = static_cast<int>(lowerPresetKeyParam_->load() + 0.5f);

    auto resolveDrawbars = [](int key,
                              const std::array<std::atomic<float>*, 9>& setA,
                              const std::array<std::atomic<float>*, 9>& setB,
                              std::array<float, 9>& out)
    {
        if (key == 0) // C1 (Cancel)
        {
            out.fill(0.0f);
        }
        else if (key == 10) // Bb1 (Set B)
        {
            for (size_t i = 0; i < 9; ++i) out[i] = setB[i]->load();
        }
        else // B1 (11) or C#1..A1 (1..9, recalled into Set A for live tweaking)
        {
            for (size_t i = 0; i < 9; ++i) out[i] = setA[i]->load();
        }
    };

    resolveDrawbars(upperKey, upperDbParams_, upperBDbParams_, twParams.upperDrawbars);
    resolveDrawbars(lowerKey, lowerDbParams_, lowerBDbParams_, twParams.lowerDrawbars);

    twParams.pedalDrawbars[0] = pedalDbParams_[0]->load();
    twParams.pedalDrawbars[1] = pedalDbParams_[1]->load();

    twParams.percussion.enabled       = percOnParam_->load() >= 0.5f;
    twParams.percussion.soft          = percSoftParam_->load() >= 0.5f;
    twParams.percussion.fast          = percFastParam_->load() >= 0.5f;
    twParams.percussion.thirdHarmonic = percThirdParam_->load() >= 0.5f;
    twParams.keyClick                 = keyClickParam_->load();
    twParams.leakage                  = leakageParam_->load();
    twParams.upperVibratoOn           = upperVCOnParam_->load() >= 0.5f;
    twParams.lowerVibratoOn           = lowerVCOnParam_->load() >= 0.5f;
    twParams.organModel               = static_cast<int>(organModelParam_->load() + 0.5f);
    twParams.muteWhenIdle             = idleGenMuteParam_->load() >= 0.5f;

    const float preampBiasBoost = DSP::getOrganProfile(twParams.organModel).preampBiasBoost;

    const auto vcMode = static_cast<DSP::ScannerVibrato::Mode>(
        static_cast<int>(vcModeParam_->load() + 0.5f));

    const float swell       = swellParam_->load();
    const float drive       = driveParam_->load();
    const float reverbMix   = reverbMixParam_->load();
    const float reverbDecay = reverbDecayParam_->load();

    DSP::Leslie122::Parameters leslieParams;
    leslieParams.enabled      = leslieOnParam_->load() >= 0.5f;
    leslieParams.cabinetModel = static_cast<int>(leslieCabModelParam_->load() + 0.5f);
    leslieParams.speed        = static_cast<DSP::Leslie122::SpeedMode>(
        static_cast<int>(leslieSpeedParam_->load() + 0.5f));
    leslieParams.micSpread   = micSpreadParam_->load();
    leslieParams.micDistance = micDistanceParam_->load();
    leslieParams.hornBalance = hornBalanceParam_->load();

    const float masterGain = masterGainParam_->load();
    const bool  splitMode  = midiSplitParam_->load() >= 0.5f;
    const int   upperCh    = static_cast<int>(upperMidiChParam_->load() + 0.5f);
    const int   lowerCh    = static_cast<int>(lowerMidiChParam_->load() + 0.5f);
    const int   pedalCh    = static_cast<int>(pedalMidiChParam_->load() + 0.5f);

    // Independent Left & Right studio mic radial distances & capsule models
    auto calcMicDistNorm = [](float mx, float my) noexcept -> float
    {
        const float dx = (mx - 0.67f) * 1200.0f;
        const float dy = (my - 0.54f) * 680.0f;
        const float pxDist = std::hypot(dx, dy);
        return juce::jlimit(0.0f, 1.0f, (pxDist - 65.0f) / 195.0f);
    };
    const float micDistL = calcMicDistNorm(micLXParam_->load(), micLYParam_->load());
    const float micDistR = calcMicDistNorm(micRXParam_->load(), micRYParam_->load());
    const int   micTypeL = static_cast<int>(micLTypeParam_->load() + 0.5f);
    const int   micTypeR = static_cast<int>(micRTypeParam_->load() + 0.5f);

    const bool  marshallOn   = marshallOnParam_->load() >= 0.5f;
    const float marshallGain = marshallGainParam_->load();
    const float marshallTone = marshallToneParam_->load();
    const float marshallMix  = marshallMixParam_->load();

    // Live movable Bluesbreaker SM57 & C414 mic distances, acoustic arrival delays, and stereo pan
    const float sm57X = ampSm57XParam_->load();
    const float sm57Y = ampSm57YParam_->load();
    const float c414X = ampC414XParam_->load();
    const float c414Y = ampC414YParam_->load();

    constexpr float kAmpCenterX = 0.335f;
    constexpr float kAmpCenterY = 0.440f;
    auto calcAmpMicFeet = [](float mx, float my) noexcept -> float
    {
        const float dxPx = (mx - kAmpCenterX) * 1244.0f;
        const float dyPx = (my - kAmpCenterY) * 388.0f;
        const float px   = std::hypot(dxPx, dyPx);
        return juce::jlimit(0.3f, 6.0f, px / 58.0f); // ~58 px ≈ 1.0 ft on stage
    };
    const float sm57Feet     = calcAmpMicFeet(sm57X, sm57Y);
    const float c414Feet     = calcAmpMicFeet(c414X, c414Y);
    const float sm57DistNorm = juce::jlimit(0.0f, 1.0f, (sm57Feet - 0.3f) / 2.7f);
    const float c414DistNorm = juce::jlimit(0.0f, 1.0f, (c414Feet - 0.3f) / 2.7f);

    const int sm57DelaySamp = juce::jlimit(1, 1000, static_cast<int>(std::round((sm57Feet * 0.3048 / 343.0) * currentSampleRate_)));
    const int c414DelaySamp = juce::jlimit(1, 1000, static_cast<int>(std::round((c414Feet * 0.3048 / 343.0) * currentSampleRate_)));

    // Stereo pan from horizontal stage offset relative to Bluesbreaker cabinet center (-1..+1)
    const float sm57Pan = juce::jlimit(-0.85f, 0.85f, (sm57X - kAmpCenterX) * 5.0f);
    const float c414Pan = juce::jlimit(-0.85f, 0.85f, (c414X - kAmpCenterX) * 5.0f);
    const float sm57Att = 1.0f / (1.0f + 0.08f * sm57Feet);
    const float c414Att = 1.0f / (1.0f + 0.08f * c414Feet);
    const float sm57GainL = (0.55f - 0.35f * sm57Pan) * sm57Att;
    const float sm57GainR = (0.55f + 0.35f * sm57Pan) * sm57Att;
    const float c414GainL = (0.55f - 0.35f * c414Pan) * c414Att;
    const float c414GainR = (0.55f + 0.35f * c414Pan) * c414Att;

    auto* outL = buffer.getWritePointer(0);
    auto* outR = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    float peakL = 0.0f;
    float peakR = 0.0f;

    auto midiIt  = midiMessages.cbegin();
    auto midiEnd = midiMessages.cend();

    for (int s = 0; s < numSamples; ++s)
    {
        // Sample-accurate MIDI dispatch
        while (midiIt != midiEnd && (*midiIt).samplePosition <= s)
        {
            handleMidiMessage((*midiIt).getMessage(), splitMode, upperCh, lowerCh, pedalCh);
            ++midiIt;
        }

        // 1. 91-Tonewheel Bank + Key Matrix + Percussion + Leakage
        const auto twOut = tonewheelEngine_.processSample(twParams);

        // 2. 18-Stage All-Pass Scanner Vibrato / Chorus (only on routed manuals)
        const float vibOut = scannerVibrato_.processSample(twOut.toVibrato, vcMode);

        // 3. AO-28 Tube Preamp & Variable-Capacitor Expression Swell
        const float preampOut = preampSwell_.processSample(vibOut + twOut.dryBypass, swell, drive, preampBiasBoost);

        // 4. Accutronics 3-Spring Reverb Tank (Pre-Leslie!)
        const float reverbOut = springReverb_.processSample(preampOut, reverbMix, reverbDecay);

        // 5. Leslie 122 Rotary Speaker (800Hz LR4 Crossover + Dual Rotor Doppler/AM)
        const auto leslieOut = leslie122_.processSample(reverbOut, leslieParams);

        // 6. Independent Distance-Coupled Studio Mic Modeling (U47 / C414 / R-121 / SM57)
        float leslieL = leslieParams.enabled ? micEqL_.processSample(leslieOut.left,  micTypeL, micDistL) : leslieOut.left;
        float leslieR = leslieParams.enabled ? micEqR_.processSample(leslieOut.right, micTypeR, micDistR) : leslieOut.right;

        // 7. Parallel Marshall 1962 "Bluesbreaker" 2x12 Combo Path
        //    Live movable dual-mic stereo capture: Shure SM57 + AKG C414
        float L = leslieL;
        float R = leslieR;
        if (marshallOn)
        {
            const float marshallMono = marshallAmp_.processSample(preampOut, marshallGain, marshallTone);

            // Push into acoustic arrival-time delay buffer
            ampMicDelayBuf_[static_cast<size_t>(ampMicDelayWrite_)] = marshallMono;
            const int sm57Read = (ampMicDelayWrite_ - sm57DelaySamp) & 1023;
            const int c414Read = (ampMicDelayWrite_ - c414DelaySamp) & 1023;
            ampMicDelayWrite_ = (ampMicDelayWrite_ + 1) & 1023;

            // Apply capsule EQ curves + live distance-coupled proximity effect
            const float sm57Sig = ampMicSm57_.processSample(ampMicDelayBuf_[static_cast<size_t>(sm57Read)], 3, sm57DistNorm);
            const float c414Sig = ampMicC414_.processSample(ampMicDelayBuf_[static_cast<size_t>(c414Read)], 1, c414DistNorm);

            // Live stereo amp mixer matrix driven by stage mic positions
            const float ampOutL = sm57Sig * sm57GainL + c414Sig * c414GainL;
            const float ampOutR = sm57Sig * sm57GainR + c414Sig * c414GainR;

            const float leslieWet = 1.0f - (marshallMix * 0.45f); // Preserve stereo Leslie swirl
            const float ampGain   = marshallMix * 1.15f;
            L = leslieL * leslieWet + ampOutL * ampGain;
            R = leslieR * leslieWet + ampOutR * ampGain;
        }
        L *= masterGain;
        R *= masterGain;

        if (outR != nullptr)
        {
            outL[s] = L;
            outR[s] = R;
        }
        else
        {
            outL[s] = 0.5f * (L + R);
        }

        peakL = std::max(peakL, std::abs(L));
        peakR = std::max(peakR, std::abs(R));
    }

    // Drain any trailing MIDI events beyond numSamples
    while (midiIt != midiEnd)
    {
        handleMidiMessage((*midiIt).getMessage(), splitMode, upperCh, lowerCh, pedalCh);
        ++midiIt;
    }

    // Publish lock-free telemetry for UI
    telemetry_.hornSpeedHz.store(leslie122_.getHornSpeedHz(), std::memory_order_relaxed);
    telemetry_.drumSpeedHz.store(leslie122_.getDrumSpeedHz(), std::memory_order_relaxed);
    telemetry_.hornAngle.store(leslie122_.getHornAngle(),     std::memory_order_relaxed);
    telemetry_.drumAngle.store(leslie122_.getDrumAngle(),     std::memory_order_relaxed);
    telemetry_.upperHeld.store(tonewheelEngine_.getKeyMatrix().getUpperHeldCount(), std::memory_order_relaxed);
    telemetry_.lowerHeld.store(tonewheelEngine_.getKeyMatrix().getLowerHeldCount(), std::memory_order_relaxed);
    telemetry_.pedalHeld.store(tonewheelEngine_.getKeyMatrix().getPedalHeldCount(), std::memory_order_relaxed);
    telemetry_.meterL.store(peakL, std::memory_order_relaxed);
    telemetry_.meterR.store(peakR, std::memory_order_relaxed);
}

int GreenOnionsAudioProcessor::getNumPrograms()
{
    return static_cast<int>(Presets::getFactoryPresets().size());
}

void GreenOnionsAudioProcessor::setCurrentProgram(int index)
{
    const int n = getNumPrograms();
    if (index < 0 || index >= n) return;
    currentProgram_      = index;
    currentPresetName_   = getProgramName(index);
    currentPresetIsUser_ = false;
    Presets::applyPreset(apvts_, index);
}

const juce::String GreenOnionsAudioProcessor::getProgramName(int index)
{
    const auto& presets = Presets::getFactoryPresets();
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[static_cast<size_t>(index)].name;
    return {};
}

juce::File GreenOnionsAudioProcessor::getUserPresetsDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("SignalForgeAudioLabs")
                   .getChildFile("GreenOnions")
                   .getChildFile("Presets");
    if (!dir.exists())
        dir.createDirectory();
    return dir;
}

juce::StringArray GreenOnionsAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    const auto dir = getUserPresetsDirectory();
    if (!dir.isDirectory())
        return names;

    const auto files = dir.findChildFiles(juce::File::findFiles, false, "*.gopreset");
    for (const auto& f : files)
        names.add(f.getFileNameWithoutExtension());

    names.sort(true); // case-insensitive alphabetical sort
    return names;
}

bool GreenOnionsAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto cleanName = juce::File::createLegalFileName(name.trim());
    if (cleanName.isEmpty())
        return false;

    const auto dir = getUserPresetsDirectory();
    if (!dir.exists() && !dir.createDirectory())
        return false;

    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr)
        return false;

    xml->setAttribute("presetName", cleanName);
    appendPresetSlotsToXml(*xml);

    const auto file = dir.getChildFile(cleanName + ".gopreset");
    if (!file.replaceWithText(xml->toString()))
        return false;

    currentPresetName_   = cleanName;
    currentPresetIsUser_ = true;
    return true;
}

bool GreenOnionsAudioProcessor::loadUserPreset(const juce::String& name)
{
    const auto cleanName = juce::File::createLegalFileName(name.trim());
    if (cleanName.isEmpty())
        return false;

    const auto file = getUserPresetsDirectory().getChildFile(cleanName + ".gopreset");
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml == nullptr || !xml->hasTagName(apvts_.state.getType()))
        return false;

    apvts_.replaceState(juce::ValueTree::fromXml(*xml));
    restorePresetSlotsFromXml(*xml);

    currentPresetName_   = cleanName;
    currentPresetIsUser_ = true;
    return true;
}

bool GreenOnionsAudioProcessor::deleteUserPreset(const juce::String& name)
{
    const auto cleanName = juce::File::createLegalFileName(name.trim());
    if (cleanName.isEmpty())
        return false;

    const auto file = getUserPresetsDirectory().getChildFile(cleanName + ".gopreset");
    if (!file.existsAsFile())
        return false;

    if (!file.deleteFile())
        return false;

    if (currentPresetIsUser_ && currentPresetName_ == cleanName)
    {
        currentPresetIsUser_ = false;
        currentPresetName_   = getProgramName(currentProgram_);
    }
    return true;
}

void GreenOnionsAudioProcessor::appendPresetSlotsToXml(juce::XmlElement& parentXml) const
{
    auto* slotsRoot = parentXml.createNewChildElement("CUSTOM_PRESET_SLOTS");

    auto encodeRegistration = [](const std::array<float, 9>& reg) -> juce::String
    {
        juce::String str;
        for (size_t d = 0; d < 9; ++d)
            str += juce::String(juce::jlimit(0, 8, static_cast<int>(std::round(reg[d]))));
        return str;
    };

    for (int slot = 1; slot <= 9; ++slot)
    {
        const size_t idx = static_cast<size_t>(slot - 1);
        auto* upperEl = slotsRoot->createNewChildElement("UPPER_SLOT");
        upperEl->setAttribute("index", slot);
        upperEl->setAttribute("bars", encodeRegistration(upperPresetSlots_[idx]));

        auto* lowerEl = slotsRoot->createNewChildElement("LOWER_SLOT");
        lowerEl->setAttribute("index", slot);
        lowerEl->setAttribute("bars", encodeRegistration(lowerPresetSlots_[idx]));
    }
}

void GreenOnionsAudioProcessor::restorePresetSlotsFromXml(const juce::XmlElement& parentXml)
{
    const auto* slotsRoot = parentXml.getChildByName("CUSTOM_PRESET_SLOTS");
    if (slotsRoot == nullptr)
        return; // Keep seeded defaults if loading an older session/preset

    auto decodeRegistration = [](const juce::String& str, std::array<float, 9>& dest)
    {
        if (str.length() < 9)
            return;
        for (int d = 0; d < 9; ++d)
        {
            const int val = str.substring(d, d + 1).getIntValue();
            dest[static_cast<size_t>(d)] = static_cast<float>(juce::jlimit(0, 8, val));
        }
    };

    for (const auto* child : slotsRoot->getChildIterator())
    {
        const int slot = child->getIntAttribute("index", -1);
        if (slot < 1 || slot > 9)
            continue;

        const size_t idx = static_cast<size_t>(slot - 1);
        const juce::String bars = child->getStringAttribute("bars");
        if (child->hasTagName("UPPER_SLOT"))
            decodeRegistration(bars, upperPresetSlots_[idx]);
        else if (child->hasTagName("LOWER_SLOT"))
            decodeRegistration(bars, lowerPresetSlots_[idx]);
    }
}

void GreenOnionsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
    {
        xml->setAttribute("currentPresetName",   currentPresetName_);
        xml->setAttribute("currentPresetIsUser", currentPresetIsUser_);
        appendPresetSlotsToXml(*xml);
        copyXmlToBinary(*xml, destData);
    }
}

void GreenOnionsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType()))
    {
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
        restorePresetSlotsFromXml(*xmlState);
        if (xmlState->hasAttribute("currentPresetName"))
        {
            currentPresetName_   = xmlState->getStringAttribute("currentPresetName");
            currentPresetIsUser_ = xmlState->getBoolAttribute("currentPresetIsUser", false);
        }
    }
}

juce::AudioProcessorEditor* GreenOnionsAudioProcessor::createEditor()
{
    return new GreenOnionsAudioProcessorEditor(*this);
}

} // namespace GreenOnions

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GreenOnions::GreenOnionsAudioProcessor();
}
