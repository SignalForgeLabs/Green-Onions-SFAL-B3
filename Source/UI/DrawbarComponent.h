#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"
#include <cmath>

namespace GreenOnions::UI
{

class DrawbarComponent final : public juce::Component
{
public:
    enum class CapStyle { Brown, Ivory, Black };

    DrawbarComponent(juce::RangedAudioParameter& param, const juce::String& footageLabel, CapStyle style)
        : param_(param),
          label_(footageLabel),
          capStyle_(style),
          attachment_(param, [this](float newValue)
          {
              value_ = newValue;
              repaint();
          })
    {
        attachment_.sendInitialUpdate();
        setRepaintsOnMouseActivity(true);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        // Top footage label
        const auto labelArea = bounds.withHeight(16.0f);
        g.setColour(Palette::creamDim);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(label_, labelArea, juce::Justification::centred, false);

        // Recessed drawbar slot
        const auto slotArea = bounds.withTop(18.0f).withBottom(bounds.getBottom() - 16.0f).reduced(4.0f, 0.0f);
        g.setColour(juce::Colour(0xff0d0c0b));
        g.fillRoundedRectangle(slotArea, 2.5f);
        g.setColour(Palette::panelBorder);
        g.drawRoundedRectangle(slotArea, 2.5f, 1.0f);

        // Travel geometry: 0 = fully pushed in (top), 8 = fully pulled out (bottom)
        const float capH      = 24.0f;
        const float travelTop = slotArea.getY() + 2.0f;
        const float travelMax = std::max(8.0f, slotArea.getHeight() - capH - 4.0f);
        const float norm      = juce::jlimit(0.0f, 1.0f, value_ / 8.0f);
        const float capY      = travelTop + norm * travelMax;

        // Brushed aluminum drawbar shaft extending from top down to the cap
        const float shaftW = std::min(14.0f, slotArea.getWidth() - 4.0f);
        const float shaftX = slotArea.getCentreX() - shaftW * 0.5f;
        const float shaftH = std::max(0.0f, (capY - travelTop) + 4.0f);

        if (shaftH > 1.0f)
        {
            juce::ColourGradient shaftGrad(juce::Colour(0xff9e9a93), shaftX, travelTop,
                                           juce::Colour(0xff6b6760), shaftX + shaftW, travelTop, false);
            g.setGradientFill(shaftGrad);
            g.fillRect(shaftX, travelTop, shaftW, shaftH);

            // Engraved numbers 1..8 visible on the pulled shaft
            g.setColour(juce::Colour(0xff1e1c1a));
            g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            const int currentStep = static_cast<int>(std::round(value_));
            const float stepH = travelMax / 8.0f;
            for (int s = 1; s <= currentStep; ++s)
            {
                // Draw from bottom (1) near cap upward to currentStep
                const float yNum = capY - static_cast<float>(s) * stepH + 1.0f;
                if (yNum >= travelTop - 2.0f && yNum + stepH <= capY + 2.0f)
                {
                    g.drawText(juce::String(currentStep - s + 1),
                               juce::Rectangle<float>(shaftX, yNum, shaftW, stepH),
                               juce::Justification::centred, false);
                }
            }
        }

        // Authentic beveled Hammond drawbar tip cap
        const auto capRect = juce::Rectangle<float>(slotArea.getX() - 1.5f, capY, slotArea.getWidth() + 3.0f, capH);
        juce::Colour baseCol = Palette::drawbarIvory;
        juce::Colour textCol = juce::Colour(0xff1a1715);
        if (capStyle_ == CapStyle::Brown)
        {
            baseCol = Palette::drawbarBrown;
            textCol = Palette::cream;
        }
        else if (capStyle_ == CapStyle::Black)
        {
            baseCol = Palette::drawbarBlack;
            textCol = Palette::cream;
        }

        if (isMouseOverOrDragging())
            baseCol = baseCol.brighter(0.12f);

        juce::ColourGradient capGrad(baseCol.brighter(0.22f), capRect.getX(), capRect.getY(),
                                     baseCol.darker(0.30f),   capRect.getX(), capRect.getBottom(), false);
        g.setGradientFill(capGrad);
        g.fillRoundedRectangle(capRect, 3.0f);

        g.setColour(juce::Colours::black.withAlpha(0.65f));
        g.drawRoundedRectangle(capRect, 3.0f, 1.0f);

        // Top bevel highlight
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawHorizontalLine(static_cast<int>(capRect.getY() + 2.0f), capRect.getX() + 3.0f, capRect.getRight() - 3.0f);

        // Current integer step printed on cap
        const int intVal = static_cast<int>(std::round(value_));
        g.setColour(textCol);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(juce::String(intVal), capRect, juce::Justification::centred, false);

        // Bottom numeric readout
        const auto bottomArea = bounds.withTop(bounds.getBottom() - 15.0f);
        g.setColour(intVal > 0 ? Palette::amber : Palette::creamDim.withAlpha(0.5f));
        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.drawText(juce::String(intVal), bottomArea, juce::Justification::centred, false);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        attachment_.beginGesture();
        updateFromMouseY(e.position.y);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        updateFromMouseY(e.position.y);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        attachment_.endGesture();
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        attachment_.setValueAsCompleteGesture(0.0f);
    }

    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const float delta = (wheel.deltaY < 0.0f) ? 1.0f : -1.0f; // Scroll down pulls drawbar out
        const float next = juce::jlimit(0.0f, 8.0f, std::round(value_ + delta));
        attachment_.setValueAsCompleteGesture(next);
    }

private:
    void updateFromMouseY(float y)
    {
        const float topY  = 24.0f;
        const float botY  = static_cast<float>(getHeight()) - 28.0f;
        const float norm  = juce::jlimit(0.0f, 1.0f, (y - topY) / std::max(10.0f, botY - topY));
        const float stepped = std::round(norm * 8.0f);
        attachment_.setValueAsPartOfGesture(stepped);
    }

    juce::RangedAudioParameter& param_;
    juce::String label_;
    CapStyle capStyle_;
    float value_ = 0.0f;
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
