#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VintageLookAndFeel.h"
#include <functional>
#include <vector>

namespace GreenOnions::UI
{

class OrganKeyboard final : public juce::Component
{
public:
    OrganKeyboard(int startMidi, int numNotes, int midiChannel, bool isPedalboard,
                  std::function<void(int, int, float)> onNoteOn,
                  std::function<void(int, int)> onNoteOff)
        : startMidi_(startMidi),
          numNotes_(numNotes),
          channel_(midiChannel),
          isPedal_(isPedalboard),
          onNoteOn_(std::move(onNoteOn)),
          onNoteOff_(std::move(onNoteOff))
    {
        for (int n = startMidi_; n < startMidi_ + numNotes_; ++n)
        {
            if (!isBlackKey(n))
                ++numWhiteKeys_;
        }
    }

    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour(isPedal_ ? Palette::walnutDark : juce::Colour(0xff12100f));
        g.fillRect(b);

        if (numWhiteKeys_ <= 0) return;

        const float whiteW = b.getWidth() / static_cast<float>(numWhiteKeys_);
        const float whiteH = b.getHeight();
        const float blackW = whiteW * 0.62f;
        const float blackH = whiteH * (isPedal_ ? 0.56f : 0.60f);

        // Pass 1: White / Natural keys (or wooden natural pedals)
        int whiteIdx = 0;
        for (int i = 0; i < numNotes_; ++i)
        {
            const int midi = startMidi_ + i;
            if (isBlackKey(midi)) continue;

            const float x = b.getX() + static_cast<float>(whiteIdx) * whiteW;
            const auto rect = juce::Rectangle<float>(x, b.getY(), whiteW - 1.0f, whiteH - 1.0f);
            const bool active = (midi == activeNote_);

            if (isPedal_)
            {
                g.setColour(active ? Palette::amber : juce::Colour(0xff7d4e2d));
                g.fillRoundedRectangle(rect.reduced(1.5f, 1.0f), 2.0f);
                g.setColour(juce::Colours::black.withAlpha(0.55f));
                g.drawRoundedRectangle(rect.reduced(1.5f, 1.0f), 2.0f, 1.0f);
            }
            else
            {
                g.setColour(active ? Palette::amberGlow : Palette::cream);
                g.fillRoundedRectangle(rect, 2.0f);
                g.setColour(juce::Colour(0xff524a40));
                g.drawRoundedRectangle(rect, 2.0f, 0.8f);
            }
            ++whiteIdx;
        }

        // Pass 2: Black / Sharp keys
        whiteIdx = 0;
        for (int i = 0; i < numNotes_; ++i)
        {
            const int midi = startMidi_ + i;
            if (!isBlackKey(midi))
            {
                ++whiteIdx;
                continue;
            }

            const float x = b.getX() + static_cast<float>(whiteIdx) * whiteW - blackW * 0.5f;
            const auto rect = juce::Rectangle<float>(x, b.getY(), blackW, blackH);
            const bool active = (midi == activeNote_);

            g.setColour(active ? Palette::amber : (isPedal_ ? juce::Colour(0xff2c1a0e) : juce::Colour(0xff1b1918)));
            g.fillRoundedRectangle(rect, 2.0f);
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(rect, 2.0f, 1.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int note = hitTestNote(e.position);
        if (note >= 0)
        {
            activeNote_ = note;
            if (onNoteOn_) onNoteOn_(channel_, activeNote_, 0.88f);
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        const int note = hitTestNote(e.position);
        if (note != activeNote_)
        {
            if (activeNote_ >= 0 && onNoteOff_)
                onNoteOff_(channel_, activeNote_);
            activeNote_ = note;
            if (activeNote_ >= 0 && onNoteOn_)
                onNoteOn_(channel_, activeNote_, 0.88f);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (activeNote_ >= 0 && onNoteOff_)
            onNoteOff_(channel_, activeNote_);
        activeNote_ = -1;
        repaint();
    }

private:
    static bool isBlackKey(int midi) noexcept
    {
        const int pc = midi % 12;
        return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    }

    int hitTestNote(juce::Point<float> pt) const noexcept
    {
        if (!getLocalBounds().toFloat().contains(pt) || numWhiteKeys_ <= 0)
            return -1;

        const float whiteW = static_cast<float>(getWidth()) / static_cast<float>(numWhiteKeys_);
        const float blackW = whiteW * 0.62f;
        const float blackH = static_cast<float>(getHeight()) * (isPedal_ ? 0.56f : 0.60f);

        // Check black keys first (they sit on top)
        if (pt.y <= blackH)
        {
            int whiteIdx = 0;
            for (int i = 0; i < numNotes_; ++i)
            {
                const int midi = startMidi_ + i;
                if (!isBlackKey(midi))
                {
                    ++whiteIdx;
                    continue;
                }
                const float x = static_cast<float>(whiteIdx) * whiteW - blackW * 0.5f;
                if (pt.x >= x && pt.x <= x + blackW)
                    return midi;
            }
        }

        // Check white keys
        int whiteIdx = 0;
        for (int i = 0; i < numNotes_; ++i)
        {
            const int midi = startMidi_ + i;
            if (isBlackKey(midi)) continue;
            const float x = static_cast<float>(whiteIdx) * whiteW;
            if (pt.x >= x && pt.x < x + whiteW)
                return midi;
            ++whiteIdx;
        }
        return -1;
    }

    int startMidi_    = 36;
    int numNotes_     = 61;
    int channel_      = 1;
    bool isPedal_     = false;
    int numWhiteKeys_ = 0;
    int activeNote_   = -1;

    std::function<void(int, int, float)> onNoteOn_;
    std::function<void(int, int)>        onNoteOff_;
};

} // namespace GreenOnions::UI
