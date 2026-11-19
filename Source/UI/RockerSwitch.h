#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"

namespace GreenOnions::UI
{

class RockerSwitch final : public juce::Component
{
public:
    RockerSwitch(juce::RangedAudioParameter& param,
                 const juce::String& title,
                 const juce::String& offLabel,
                 const juce::String& onLabel)
        : title_(title),
          offLabel_(offLabel),
          onLabel_(onLabel),
          attachment_(param, [this](float newValue)
          {
              isOn_ = (newValue >= 0.5f);
              repaint();
          })
    {
        attachment_.sendInitialUpdate();
        setRepaintsOnMouseActivity(true);
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Top title
        g.setColour(Palette::creamDim);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(title_, b.removeFromTop(14.0f), juce::Justification::centred, false);

        // Tablet switch housing
        auto housing = b.reduced(4.0f, 2.0f);
        g.setColour(juce::Colour(0xff0c0b0a));
        g.fillRoundedRectangle(housing, 3.0f);

        auto tab = housing.reduced(2.0f);
        if (isOn_)
        {
            // Engaged (tilted down toward user, glowing amber indicator strip at bottom)
            juce::ColourGradient grad(Palette::cream.darker(0.18f), tab.getX(), tab.getY(),
                                      Palette::cream.brighter(0.08f), tab.getX(), tab.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(tab, 2.5f);

            // Warm amber active bar at bottom edge
            g.setColour(Palette::amber);
            g.fillRoundedRectangle(tab.withTop(tab.getBottom() - 4.0f), 1.5f);
        }
        else
        {
            // Disengaged (tilted up)
            juce::ColourGradient grad(Palette::cream.brighter(0.05f), tab.getX(), tab.getY(),
                                      Palette::cream.darker(0.32f), tab.getX(), tab.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(tab, 2.5f);
        }

        // Center rocker crease
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawHorizontalLine(static_cast<int>(tab.getCentreY()), tab.getX() + 2.0f, tab.getRight() - 2.0f);

        // Top (Off) & Bottom (On) engraved labels
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(isOn_ ? juce::Colour(0xff5a5248) : juce::Colour(0xff1a1715));
        g.drawText(offLabel_, tab.withHeight(tab.getHeight() * 0.5f), juce::Justification::centred, false);

        g.setColour(isOn_ ? juce::Colour(0xff1a1715) : juce::Colour(0xff5a5248));
        g.drawText(onLabel_, tab.withTop(tab.getCentreY()), juce::Justification::centred, false);

        if (isMouseOver())
        {
            g.setColour(Palette::amber.withAlpha(0.35f));
            g.drawRoundedRectangle(housing, 3.0f, 1.2f);
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        attachment_.setValueAsCompleteGesture(isOn_ ? 0.0f : 1.0f);
    }

private:
    juce::String title_;
    juce::String offLabel_;
    juce::String onLabel_;
    bool isOn_ = false;
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
