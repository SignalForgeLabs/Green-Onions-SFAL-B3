#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace GreenOnions::UI
{

class TiltRockerSwitch final : public juce::Component
{
public:
    TiltRockerSwitch(juce::RangedAudioParameter& param,
                     const juce::String& titleText,
                     const juce::String& topLabelText,
                     const juce::String& bottomLabelText,
                     bool showPilotLed = false)
        : title_(titleText),
          topText_(topLabelText),
          bottomText_(bottomLabelText),
          hasLed_(showPilotLed),
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
        auto area = getLocalBounds().toFloat();

        // Top section title
        if (title_.isNotEmpty())
        {
            g.setColour(juce::Colour(0xffa89c87));
            g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            g.drawText(title_, area.removeFromTop(13.0f), juce::Justification::centred, false);
        }

        const auto b = area.reduced(2.0f);

        // Recessed dark bezel housing
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(isMouseOver() ? juce::Colour(0xffd98a2b) : juce::Colour(0xff3d352c));
        g.drawRoundedRectangle(b, 4.0f, 1.0f);

        const auto tablet = b.reduced(3.0f);
        const float midY  = tablet.getCentreY();

        // Aged Bakelite Ivory Gradient (flips direction based on tilt state)
        const juce::Colour ivoryBright(0xfff5eedc);
        const juce::Colour ivoryMid(0xffe0d5bc);
        const juce::Colour ivoryShadow(0xff9e927a);

        juce::ColourGradient grad = isOn_
            ? juce::ColourGradient(ivoryShadow, 0.0f, tablet.getY(),
                                   ivoryBright, 0.0f, tablet.getBottom(), false)
            : juce::ColourGradient(ivoryBright, 0.0f, tablet.getY(),
                                   ivoryShadow, 0.0f, tablet.getBottom(), false);
        grad.addColour(0.5, ivoryMid);

        g.setGradientFill(grad);
        g.fillRoundedRectangle(tablet, 3.0f);

        // Center pivot crease
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawLine(tablet.getX() + 2.0f, midY, tablet.getRight() - 2.0f, midY, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawLine(tablet.getX() + 2.0f, midY + 1.2f, tablet.getRight() - 2.0f, midY + 1.2f, 1.0f);

        // Engraved Top & Bottom Labels
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(isOn_ ? juce::Colour(0xff52493b) : juce::Colour(0xff1c1915));
        g.drawText(topText_, tablet.withBottom(midY).toNearestInt(), juce::Justification::centred, false);

        g.setColour(isOn_ ? juce::Colour(0xff1c1915) : juce::Colour(0xff52493b));
        g.drawText(bottomText_, tablet.withTop(midY).toNearestInt(), juce::Justification::centred, false);

        // Optional Warm Tungsten Pilot Jewel
        if (hasLed_)
        {
            const float r  = 2.8f;
            const float cx = tablet.getCentreX();
            const float cy = tablet.getBottom() - 4.5f;
            if (isOn_)
            {
                g.setColour(juce::Colour(0xffff9e2c).withAlpha(0.35f));
                g.fillEllipse(cx - r * 1.8f, cy - r * 1.8f, r * 3.6f, r * 3.6f);
            }
            g.setColour(isOn_ ? juce::Colour(0xffffa834) : juce::Colour(0xff3a2a1d));
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        attachment_.setValueAsCompleteGesture(isOn_ ? 0.0f : 1.0f);
    }

private:
    juce::String title_;
    juce::String topText_;
    juce::String bottomText_;
    bool hasLed_ = false;
    bool isOn_   = false;
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
