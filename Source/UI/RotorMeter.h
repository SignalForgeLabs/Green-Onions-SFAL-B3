#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"
#include <cmath>
#include <numbers>

namespace GreenOnions::UI
{

class RotorMeter final : public juce::Component
{
public:
    void setTelemetry(float hornAngle, float drumAngle, float hornHz, float drumHz) noexcept
    {
        hornAngle_ = hornAngle;
        drumAngle_ = drumAngle;
        hornHz_    = hornHz;
        drumHz_    = drumHz;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced(2.0f);
        g.setColour(juce::Colour(0xff110f0e));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(Palette::panelBorder);
        g.drawRoundedRectangle(b, 5.0f, 1.0f);

        const auto leftHalf  = b.withWidth(b.getWidth() * 0.5f).reduced(4.0f);
        const auto rightHalf = b.withLeft(b.getCentreX()).reduced(4.0f);

        drawRotor(g, leftHalf,  hornAngle_, hornHz_, "HORN", Palette::amberGlow);
        drawRotor(g, rightHalf, drumAngle_, drumHz_, "DRUM", Palette::greenAccent);
    }

private:
    static void drawRotor(juce::Graphics& g, juce::Rectangle<float> area,
                          float angle, float hz, const juce::String& name, juce::Colour accent)
    {
        const float r  = std::min(area.getWidth(), area.getHeight() - 16.0f) * 0.44f;
        const float cx = area.getCentreX();
        const float cy = area.getY() + r + 4.0f;

        // Rotor housing circle
        g.setColour(juce::Colour(0xff231f1c));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(Palette::panelBorder);
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);

        // Spinning horn/drum blade
        const float dx = std::cos(angle) * (r * 0.85f);
        const float dy = std::sin(angle) * (r * 0.85f);

        // Main acoustic beam lobe (bright) + counter-lobe (dim)
        g.setColour(accent);
        g.drawLine(cx, cy, cx + dx, cy + dy, 3.0f);
        g.fillEllipse(cx + dx - 3.0f, cy + dy - 3.0f, 6.0f, 6.0f);

        g.setColour(accent.withAlpha(0.35f));
        g.drawLine(cx, cy, cx - dx * 0.65f, cy - dy * 0.65f, 2.0f);

        // Hub dot
        g.setColour(Palette::cream);
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);

        // RPM readout
        const int rpm = static_cast<int>(std::round(hz * 60.0f));
        g.setColour(Palette::creamDim);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(name + " " + juce::String(rpm),
                   area.withTop(area.getBottom() - 14.0f),
                   juce::Justification::centred, false);
    }

    float hornAngle_ = 0.0f;
    float drumAngle_ = 0.0f;
    float hornHz_    = 0.67f;
    float drumHz_    = 0.57f;
};

} // namespace GreenOnions::UI
