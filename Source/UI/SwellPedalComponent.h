#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"

namespace GreenOnions::UI
{

// Authentic Hammond B-3 Ribbed Aluminum Expression / Swell Pedal (CC 11)
// Drag vertically or scroll wheel to rock the pedal from heel-down (0%) to toe-down (100%).
class SwellPedalComponent final : public juce::Component
{
public:
    explicit SwellPedalComponent(juce::RangedAudioParameter& swellParam)
        : attachment_(swellParam, [this](float newValue)
          {
              value_ = juce::jlimit(0.0f, 1.0f, newValue);
              repaint();
          })
    {
        attachment_.sendInitialUpdate();
        setRepaintsOnMouseActivity(true);
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced(2.0f);

        // Recessed floor well
        g.setColour(juce::Colour(0xff0f0d0c));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(Palette::panelBorder);
        g.drawRoundedRectangle(b, 5.0f, 1.0f);

        // Left title + percentage readout
        auto leftInfo = b.withWidth(b.getWidth() * 0.44f).reduced(6.0f, 4.0f);
        g.setColour(Palette::amber);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText("SWELL", leftInfo.removeFromTop(14.0f), juce::Justification::centredLeft, false);

        g.setColour(Palette::creamDim);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText("CC 11", leftInfo.removeFromTop(12.0f), juce::Justification::centredLeft, false);

        g.setColour(Palette::amberGlow);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String(juce::roundToInt(value_ * 100.0f)) + "%",
                   leftInfo, juce::Justification::centredLeft, false);

        // Ribbed cast-aluminum expression treadle on right side
        const auto pedalArea = b.withLeft(b.getX() + b.getWidth() * 0.44f).reduced(6.0f, 5.0f);

        // Shadow & brushed metal body (brightens as toe pushes forward)
        const float brightness = 0.72f + 0.28f * value_;
        juce::ColourGradient treadGrad(
            juce::Colour(0xff4a4742).withMultipliedBrightness(brightness), pedalArea.getX(), pedalArea.getY(),
            juce::Colour(0xff262422).withMultipliedBrightness(brightness), pedalArea.getX(), pedalArea.getBottom(), false);
        g.setGradientFill(treadGrad);
        g.fillRoundedRectangle(pedalArea, 3.5f);

        // Amber level bar filling from bottom (heel) to top (toe)
        const float fillH = pedalArea.getHeight() * value_;
        const auto fillRect = pedalArea.withTop(pedalArea.getBottom() - fillH);
        g.setColour(Palette::amber.withAlpha(0.32f));
        g.fillRoundedRectangle(fillRect, 3.0f);

        // Horizontal ribbed rubber/aluminum grip strips
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        for (float y = pedalArea.getY() + 5.0f; y < pedalArea.getBottom() - 4.0f; y += 6.0f)
            g.drawHorizontalLine(static_cast<int>(y), pedalArea.getX() + 4.0f, pedalArea.getRight() - 4.0f);

        // Active toe-position highlight bar
        const float barY = pedalArea.getBottom() - fillH;
        g.setColour(Palette::amberGlow);
        g.drawHorizontalLine(static_cast<int>(juce::jlimit(pedalArea.getY() + 2.0f, pedalArea.getBottom() - 2.0f, barY)),
                             pedalArea.getX() + 2.0f, pedalArea.getRight() - 2.0f);

        g.setColour(isMouseOverOrDragging() ? Palette::amber : juce::Colour(0xff6b6358));
        g.drawRoundedRectangle(pedalArea, 3.5f, 1.2f);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        attachment_.beginGesture();
        updateFromY(e.position.y);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        updateFromY(e.position.y);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        attachment_.endGesture();
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const float next = juce::jlimit(0.0f, 1.0f, value_ + wheel.deltaY * 0.12f);
        attachment_.setValueAsCompleteGesture(next);
    }

private:
    void updateFromY(float y)
    {
        const float top = 6.0f;
        const float bot = static_cast<float>(getHeight()) - 6.0f;
        const float norm = juce::jlimit(0.0f, 1.0f, 1.0f - (y - top) / std::max(10.0f, bot - top));
        attachment_.setValueAsPartOfGesture(norm);
    }

    float value_ = 0.85f;
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
