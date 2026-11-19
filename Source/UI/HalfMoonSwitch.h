#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"
#include <cmath>
#include <numbers>

namespace GreenOnions::UI
{

class HalfMoonSwitch final : public juce::Component
{
public:
    explicit HalfMoonSwitch(juce::RangedAudioParameter& speedParam)
        : attachment_(speedParam, [this](float newValue)
          {
              mode_ = static_cast<int>(std::round(newValue));
              repaint();
          })
    {
        attachment_.sendInitialUpdate();
        setRepaintsOnMouseActivity(true);
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();

        // Top labels: CHORALE | BRAKE | TREMOLO
        const float w3 = b.getWidth() / 3.0f;
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));

        g.setColour(mode_ == 0 ? Palette::amberGlow : Palette::creamDim);
        g.drawText("CHORALE", juce::Rectangle<float>(b.getX(), b.getY(), w3, 14.0f), juce::Justification::centred, false);

        g.setColour(mode_ == 1 ? Palette::amberGlow : Palette::creamDim);
        g.drawText("BRAKE", juce::Rectangle<float>(b.getX() + w3, b.getY(), w3, 14.0f), juce::Justification::centred, false);

        g.setColour(mode_ == 2 ? Palette::amberGlow : Palette::creamDim);
        g.drawText("TREMOLO", juce::Rectangle<float>(b.getX() + 2.0f * w3, b.getY(), w3, 14.0f), juce::Justification::centred, false);

        // Half-moon bakelite housing
        const auto housing = b.withTop(16.0f).reduced(6.0f, 3.0f);
        const float cx = housing.getCentreX();
        const float cy = housing.getBottom();
        const float rx = housing.getWidth() * 0.48f;
        const float ry = housing.getHeight() * 0.92f;

        juce::Path moon;
        moon.addCentredArc(cx, cy, rx, ry, 0.0f, -std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float> * 0.5f, true);
        moon.closeSubPath();

        juce::ColourGradient moonGrad(juce::Colour(0xff3a2a1e), cx, cy - ry,
                                      juce::Colour(0xff17110d), cx, cy, false);
        g.setGradientFill(moonGrad);
        g.fillPath(moon);

        g.setColour(Palette::panelBorder.brighter(0.2f));
        g.strokePath(moon, juce::PathStrokeType(1.4f));

        // Recessed chrome slot
        const float slotR = std::min(rx, ry) * 0.55f;
        juce::Path slot;
        slot.addCentredArc(cx, cy - 3.0f, slotR, slotR, 0.0f, -0.85f, 0.85f, true);
        g.setColour(juce::Colours::black);
        g.strokePath(slot, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Chrome toggle lever pointing to Chorale (-0.75 rad), Brake (0 rad), or Tremolo (+0.75 rad)
        const float leverAngle = (mode_ == 0) ? -0.75f : (mode_ == 1 ? 0.0f : 0.75f);
        const float pivotX = cx;
        const float pivotY = cy - 4.0f;
        const float tipLen = ry * 0.90f;
        const float tipX   = pivotX + std::sin(leverAngle) * tipLen;
        const float tipY   = pivotY - std::cos(leverAngle) * tipLen;

        // Lever shaft
        g.setColour(juce::Colour(0xffc9c3b8));
        g.drawLine(pivotX, pivotY, tipX, tipY, 4.5f);

        // Lever bat tip
        g.setColour(Palette::cream);
        g.fillEllipse(tipX - 5.5f, tipY - 5.5f, 11.0f, 11.0f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawEllipse(tipX - 5.5f, tipY - 5.5f, 11.0f, 11.0f, 1.0f);

        // Center chrome nut
        g.setColour(juce::Colour(0xff8a8378));
        g.fillEllipse(pivotX - 5.0f, pivotY - 5.0f, 10.0f, 10.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override { selectFromX(e.position.x); }
    void mouseDrag(const juce::MouseEvent& e) override { selectFromX(e.position.x); }

private:
    void selectFromX(float x)
    {
        const float w3 = static_cast<float>(getWidth()) / 3.0f;
        int next = (x < w3) ? 0 : (x < 2.0f * w3 ? 1 : 2);
        if (next != mode_)
            attachment_.setValueAsCompleteGesture(static_cast<float>(next));
    }

    int mode_ = 0;
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
