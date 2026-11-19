#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DrawbarComponent.h"
#include "../Parameters.h"
#include <array>
#include <cmath>
#include <memory>

namespace GreenOnions::UI
{

class DualDrawbarDeck final : public juce::Component
{
public:
    explicit DualDrawbarDeck(juce::AudioProcessorValueTreeState& apvts)
        : upperKeyAttachment_(*apvts.getParameter(Params::kUpperPresetKey),
                              [this](float v) { upperKey_ = static_cast<int>(std::round(v)); repaint(); }),
          lowerKeyAttachment_(*apvts.getParameter(Params::kLowerPresetKey),
                              [this](float v) { lowerKey_ = static_cast<int>(std::round(v)); repaint(); })
    {
        constexpr std::array<DrawbarComponent::CapStyle, 9> kCapStyles = {
            DrawbarComponent::CapStyle::Brown,
            DrawbarComponent::CapStyle::Brown,
            DrawbarComponent::CapStyle::Ivory,
            DrawbarComponent::CapStyle::Ivory,
            DrawbarComponent::CapStyle::Black,
            DrawbarComponent::CapStyle::Ivory,
            DrawbarComponent::CapStyle::Black,
            DrawbarComponent::CapStyle::Black,
            DrawbarComponent::CapStyle::Ivory
        };

        for (size_t i = 0; i < 9; ++i)
        {
            upperA_[i] = std::make_unique<DrawbarComponent>(
                *apvts.getParameter(Params::kUpperDrawbarIds[i]), Params::kDrawbarLabels[i], kCapStyles[i]);
            upperB_[i] = std::make_unique<DrawbarComponent>(
                *apvts.getParameter(Params::kUpperBDrawbarIds[i]), Params::kDrawbarLabels[i], kCapStyles[i]);
            lowerA_[i] = std::make_unique<DrawbarComponent>(
                *apvts.getParameter(Params::kLowerDrawbarIds[i]), Params::kDrawbarLabels[i], kCapStyles[i]);
            lowerB_[i] = std::make_unique<DrawbarComponent>(
                *apvts.getParameter(Params::kLowerBDrawbarIds[i]), Params::kDrawbarLabels[i], kCapStyles[i]);

            addAndMakeVisible(*upperA_[i]);
            addAndMakeVisible(*upperB_[i]);
            addAndMakeVisible(*lowerA_[i]);
            addAndMakeVisible(*lowerB_[i]);
        }

        pedal16_ = std::make_unique<DrawbarComponent>(
            *apvts.getParameter(Params::kPedalDrawbarIds[0]), "16'", DrawbarComponent::CapStyle::Brown);
        pedal8_  = std::make_unique<DrawbarComponent>(
            *apvts.getParameter(Params::kPedalDrawbarIds[1]), "8'",  DrawbarComponent::CapStyle::Ivory);
        addAndMakeVisible(*pedal16_);
        addAndMakeVisible(*pedal8_);

        upperKeyAttachment_.sendInitialUpdate();
        lowerKeyAttachment_.sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        VintageLookAndFeel::drawAnodizedPanel(g, bounds.toFloat(), 5.0f);

        auto drawGroupHeader = [&](juce::Rectangle<int> area, const juce::String& label, bool isLive, bool isPreset)
        {
            g.setColour(isLive ? juce::Colour(0xff2c2419) : juce::Colour(0xff171514));
            g.fillRoundedRectangle(area.toFloat(), 3.0f);
            g.setColour(isLive ? Palette::amber.withAlpha(0.7f) : Palette::panelBorder);
            g.drawRoundedRectangle(area.toFloat(), 3.0f, 0.9f);

            juce::String txt = label;
            if (isLive)        txt += "  [LIVE]";
            else if (isPreset) txt += "  [PRESET]";

            g.setColour(isLive ? Palette::amberGlow : Palette::creamDim);
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            g.drawText(txt, area, juce::Justification::centred, false);
        };

        const int pedalW = 78;
        const int gap    = 6;
        const int setW   = (getWidth() - pedalW - gap * 6) / 4;

        const bool upperLiveA = (upperKey_ == 11);
        const bool upperLiveB = (upperKey_ == 10);
        const bool upperSlot  = (upperKey_ >= 1 && upperKey_ <= 9);

        const bool lowerLiveA = (lowerKey_ == 11);
        const bool lowerLiveB = (lowerKey_ == 10);
        const bool lowerSlot  = (lowerKey_ >= 1 && lowerKey_ <= 9);

        int x = gap;
        drawGroupHeader({ x, 4, setW, 18 }, "UPPER SET A (B)",  upperLiveA, upperSlot);
        x += setW + gap;
        drawGroupHeader({ x, 4, setW, 18 }, "UPPER SET B (Bb)", upperLiveB, false);
        x += setW + gap;
        drawGroupHeader({ x, 4, pedalW, 18 }, "PEDALS", true, false);
        x += pedalW + gap;
        drawGroupHeader({ x, 4, setW, 18 }, "LOWER SET A (B)",  lowerLiveA, lowerSlot);
        x += setW + gap;
        drawGroupHeader({ x, 4, setW, 18 }, "LOWER SET B (Bb)", lowerLiveB, false);
    }

    void resized() override
    {
        const int pedalW = 78;
        const int gap    = 6;
        const int setW   = (getWidth() - pedalW - gap * 6) / 4;
        const int top    = 24;
        const int h      = getHeight() - top - 4;

        auto layoutSet = [&](int startX, int width, auto& bars)
        {
            const int barW = width / 9;
            const int rem  = (width - barW * 9) / 2;
            for (int i = 0; i < 9; ++i)
                bars[static_cast<size_t>(i)]->setBounds(startX + rem + i * barW, top, barW, h);
        };

        int x = gap;
        layoutSet(x, setW, upperA_);
        x += setW + gap;
        layoutSet(x, setW, upperB_);
        x += setW + gap;

        const int pBarW = (pedalW - 4) / 2;
        pedal16_->setBounds(x + 2, top, pBarW, h);
        pedal8_->setBounds(x + 2 + pBarW, top, pBarW, h);
        x += pedalW + gap;

        layoutSet(x, setW, lowerA_);
        x += setW + gap;
        layoutSet(x, setW, lowerB_);
    }

private:
    std::array<std::unique_ptr<DrawbarComponent>, 9> upperA_, upperB_, lowerA_, lowerB_;
    std::unique_ptr<DrawbarComponent> pedal16_, pedal8_;

    int upperKey_ = 11;
    int lowerKey_ = 11;
    juce::ParameterAttachment upperKeyAttachment_;
    juce::ParameterAttachment lowerKeyAttachment_;
};

} // namespace GreenOnions::UI
