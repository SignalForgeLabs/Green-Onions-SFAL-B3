#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include <functional>

namespace GreenOnions::UI
{

class PresetOctaveComponent final : public juce::Component,
                                    public juce::SettableTooltipClient
{
public:
    // Callback invoked when a preset key is clicked: (keyIndex 0..11, saveCurrentToSlot)
    // 0 = C (Cancel), 1..9 = C#..A (9 Instant Registrations), 10 = Bb (Set B), 11 = B (Set A)
    std::function<void(int keyIndex, bool saveCurrentToSlot)> onPresetKeyLatched;

    PresetOctaveComponent(juce::RangedAudioParameter& presetParam, const juce::String& manualLabel)
        : title_(manualLabel),
          attachment_(presetParam, [this](float newValue)
          {
              latchedKey_ = static_cast<int>(std::round(newValue));
              repaint();
          })
    {
        attachment_.sendInitialUpdate();
        setTooltip("Reverse-color preset octave (C=Cancel, C#-A=Presets, Bb=Set B, B=Set A). Shift/Right-click C#-A to save live drawbars.");
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();

        // Dark keybed housing
        g.setColour(juce::Colour(0xff141210));
        g.fillRoundedRectangle(b, 3.0f);

        // Classic Hammond red felt strip along keybed top
        g.setColour(juce::Colour(0xff7a1c1c));
        g.fillRect(b.getX() + 2.0f, b.getY() + 2.0f, b.getWidth() - 4.0f, 4.0f);

        const float keyTop    = b.getY() + 6.0f;
        const float keyHeight = b.getHeight() - 8.0f;
        const float whiteW    = (b.getWidth() - 4.0f) / 7.0f;

        // 7 Natural Keys (Reverse Color = Ebony Black): C(0), D(2), E(4), F(5), G(7), A(9), B(11)
        static constexpr int kNaturalIndices[7]   = { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr const char* kNatNames[7] = { "C", "D", "E", "F", "G", "A", "B" };

        for (int i = 0; i < 7; ++i)
        {
            const int keyIdx  = kNaturalIndices[i];
            const bool isDown = (latchedKey_ == keyIdx);
            const float x     = b.getX() + 2.0f + static_cast<float>(i) * whiteW;
            const float drop  = isDown ? 5.0f : 0.0f;

            const juce::Rectangle<float> keyRect(x + 0.5f, keyTop + drop, whiteW - 1.0f, keyHeight - drop);

            // Reverse Ebony Key Body
            g.setColour(isDown ? juce::Colour(0xff121316) : juce::Colour(0xff26282e));
            g.fillRoundedRectangle(keyRect, 2.0f);

            g.setColour(isDown ? juce::Colour(0xffd99b4e) : juce::Colour(0xff42464f));
            g.drawRoundedRectangle(keyRect, 2.0f, isDown ? 1.5f : 0.8f);

            // Key legend at bottom tip
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.setColour(isDown ? juce::Colour(0xffffb74d) : juce::Colour(0xffa69c8b));
            g.drawText(kNatNames[i], keyRect.withTop(keyRect.getBottom() - 15.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }

        // 5 Sharp Keys (Reverse Color = Warm Ivory): C#(1), D#(3), F#(6), G#(8), Bb(10)
        static constexpr std::pair<int, int> kSharpSlots[5] = {
            { 0, 1 }, { 1, 3 }, { 3, 6 }, { 4, 8 }, { 5, 10 }
        };
        static constexpr const char* kSharpNames[5] = { "C#", "D#", "F#", "G#", "Bb" };

        for (int s = 0; s < 5; ++s)
        {
            const auto [slot, keyIdx] = kSharpSlots[s];
            const bool isDown = (latchedKey_ == keyIdx);
            const float x      = b.getX() + 2.0f + (static_cast<float>(slot) + 0.64f) * whiteW;
            const float sharpW = whiteW * 0.70f;
            const float sharpH = keyHeight * 0.58f;
            const float drop   = isDown ? 4.0f : 0.0f;

            const juce::Rectangle<float> sharpRect(x, keyTop + drop, sharpW, sharpH - drop);

            // Reverse Ivory Sharp Key Body
            g.setColour(isDown ? juce::Colour(0xffc9bfa8) : juce::Colour(0xfff2ead8));
            g.fillRoundedRectangle(sharpRect, 2.0f);

            g.setColour(isDown ? juce::Colour(0xffd99b4e) : juce::Colour(0xff6b6254));
            g.drawRoundedRectangle(sharpRect, 2.0f, isDown ? 1.5f : 0.8f);

            g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
            g.setColour(juce::Colour(0xff231f1a));
            g.drawText(kSharpNames[s], sharpRect.withTop(sharpRect.getBottom() - 13.0f).toNearestInt(),
                       juce::Justification::centred, false);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int hit = hitTestKey(e.position);
        if (hit < 0) return;

        const bool saveSlot = e.mods.isShiftDown() || e.mods.isRightButtonDown();
        if (!saveSlot)
        {
            attachment_.setValueAsCompleteGesture(static_cast<float>(hit));
        }

        if (onPresetKeyLatched)
            onPresetKeyLatched(hit, saveSlot);

        repaint();
    }

private:
    int hitTestKey(juce::Point<float> pt) const noexcept
    {
        const auto b = getLocalBounds().toFloat();
        const float keyTop    = b.getY() + 6.0f;
        const float keyHeight = b.getHeight() - 8.0f;
        const float whiteW    = (b.getWidth() - 4.0f) / 7.0f;

        static constexpr std::pair<int, int> kSharpSlots[5] = {
            { 0, 1 }, { 1, 3 }, { 3, 6 }, { 4, 8 }, { 5, 10 }
        };
        if (pt.y <= keyTop + keyHeight * 0.58f)
        {
            for (const auto& [slot, keyIdx] : kSharpSlots)
            {
                const float x = b.getX() + 2.0f + (static_cast<float>(slot) + 0.64f) * whiteW;
                if (pt.x >= x && pt.x <= x + whiteW * 0.70f)
                    return keyIdx;
            }
        }

        static constexpr int kNaturalIndices[7] = { 0, 2, 4, 5, 7, 9, 11 };
        const int natIdx = juce::jlimit(0, 6, static_cast<int>((pt.x - (b.getX() + 2.0f)) / whiteW));
        return kNaturalIndices[natIdx];
    }

    juce::String title_;
    int latchedKey_ = 11; // Default to 'B' (Set A live drawbars)
    juce::ParameterAttachment attachment_;
};

} // namespace GreenOnions::UI
