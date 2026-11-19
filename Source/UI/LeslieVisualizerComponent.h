#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/Leslie122.h"
#include "../Parameters.h"
#include <cmath>
#include <functional>

namespace GreenOnions::UI
{

class LeslieVisualizerComponent final : public juce::Component,
                                        private juce::Timer
{
public:
    using TelemetryProvider = std::function<DSP::Leslie122::RotorTelemetry()>;

    LeslieVisualizerComponent(juce::AudioProcessorValueTreeState& state,
                              TelemetryProvider provider)
        : apvts_(state), getTelemetry_(std::move(provider))
    {
        micSpreadParam_ = apvts_.getRawParameterValue(Params::kMicSpread);
        micDistParam_   = apvts_.getRawParameterValue(Params::kMicDistance);
        cabModelParam_  = apvts_.getRawParameterValue(Params::kLeslieCabinetModel);
        startTimerHz(60); // Smooth 60 fps rotor animation
    }

    ~LeslieVisualizerComponent() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.5f);

        // 1. Dark Anodized Inspector Frame
        g.setColour(juce::Colour(0xff121316));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xff3b3228));
        g.drawRoundedRectangle(bounds, 5.0f, 1.1f);

        const bool is147 = cabModelParam_ && (cabModelParam_->load() > 0.5f);
        const juce::Colour hornAccent = is147 ? juce::Colour(0xffff7043) : juce::Colour(0xff69f0ae);

        // Header Title
        g.setColour(is147 ? juce::Colour(0xffff8a65) : juce::Colour(0xffd4c5a9));
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(is147 ? "147 ROTOR CHAMBER" : "122 ROTOR CHAMBER",
                   bounds.withHeight(15.0f).reduced(5.0f, 1.0f).toNearestInt(),
                   juce::Justification::centred, false);

        // Cabinet Cutaway Geometry
        const float dialSize = juce::jmax(28.0f, juce::jmin(bounds.getWidth() - 22.0f, bounds.getHeight() - 36.0f));
        const juce::Rectangle<float> chamber(
            bounds.getCentreX() - dialSize * 0.5f,
            bounds.getY() + 16.0f + ((bounds.getHeight() - 34.0f) - dialSize) * 0.5f,
            dialSize,
            dialSize
        );
        const auto center  = chamber.getCentre();
        const float radius = dialSize * 0.5f;

        // 2. Walnut Cabinet Outer Housing & Acoustic Louver Ring
        g.setColour(juce::Colour(0xff382216));
        g.fillRoundedRectangle(chamber.expanded(2.5f), 7.0f);
        g.setColour(juce::Colour(0xff6e4a32));
        g.drawRoundedRectangle(chamber.expanded(2.5f), 7.0f, 1.3f);

        g.setColour(juce::Colour(0xff0b0c0e));
        g.fillEllipse(chamber.reduced(3.0f));

        // Subtle compass crosshairs
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawLine(center.x - radius + 5.0f, center.y, center.x + radius - 5.0f, center.y, 1.0f);
        g.drawLine(center.x, center.y - radius + 5.0f, center.x, center.y + radius - 5.0f, 1.0f);

        // 3. Lower Bass Rotor (Wooden Scoop Drum)
        const float drumR = radius * 0.72f;
        {
            juce::Graphics::ScopedSaveState save(g);
            g.addTransform(juce::AffineTransform::rotation(telemetry_.drumAngleRad, center.x, center.y));

            // Drum cylinder
            g.setColour(juce::Colour(0xff593924).withAlpha(0.58f));
            g.fillEllipse(center.x - drumR, center.y - drumR, drumR * 2.0f, drumR * 2.0f);
            g.setColour(juce::Colour(0xff8c5d3c).withAlpha(0.70f));
            g.drawEllipse(center.x - drumR, center.y - drumR, drumR * 2.0f, drumR * 2.0f, 1.4f);

            // Acoustic Wooden Scoop Opening (faces 0 rad = Top / 12 o'clock)
            juce::Path scoop;
            scoop.addPieSegment(center.x - drumR, center.y - drumR, drumR * 2.0f, drumR * 2.0f,
                                -0.55f, 0.55f, 0.0f);
            g.setColour(juce::Colour(0xffd99b4e).withAlpha(0.35f));
            g.fillPath(scoop);
        }

        // 4. Upper Treble Horn (Dual-Bell Phenolic Rotor)
        const float hornLen = radius * 0.68f;
        {
            juce::Graphics::ScopedSaveState save(g);
            g.addTransform(juce::AffineTransform::rotation(telemetry_.hornAngleRad, center.x, center.y));

            // Sound projection cone from the live acoustic horn (facing 0 rad)
            juce::Path beam;
            beam.addPieSegment(center.x - radius * 0.88f, center.y - radius * 0.88f,
                               radius * 1.76f, radius * 1.76f,
                               -0.38f, 0.38f, 0.0f);
            juce::ColourGradient beamGrad(hornAccent.withAlpha(0.38f), center.x, center.y,
                                          hornAccent.withAlpha(0.0f),  center.x, center.y - radius * 0.88f, false);
            g.setGradientFill(beamGrad);
            g.fillPath(beam);

            // Counterweight Dummy Horn (180 degrees opposite, dark charcoal)
            juce::Path dummyBell;
            dummyBell.startNewSubPath(center.x - 3.5f, center.y);
            dummyBell.lineTo(center.x - 8.5f, center.y + hornLen);
            dummyBell.lineTo(center.x + 8.5f, center.y + hornLen);
            dummyBell.lineTo(center.x + 3.5f, center.y);
            dummyBell.closeSubPath();
            g.setColour(juce::Colour(0xff2a2d34));
            g.fillPath(dummyBell);

            // Active Acoustic Treble Horn (0 degrees, bright rim + emerald/amber highlight)
            juce::Path activeBell;
            activeBell.startNewSubPath(center.x - 3.5f, center.y);
            activeBell.lineTo(center.x - 10.5f, center.y - hornLen);
            activeBell.lineTo(center.x + 10.5f, center.y - hornLen);
            activeBell.lineTo(center.x + 3.5f, center.y);
            activeBell.closeSubPath();
            g.setColour(juce::Colour(0xffebe6dc));
            g.fillPath(activeBell);
            g.setColour(hornAccent);
            g.drawLine(center.x - 10.5f, center.y - hornLen, center.x + 10.5f, center.y - hornLen, 2.2f);

            // Center Brass Spindle Cap
            g.setColour(juce::Colour(0xffd99b4e));
            g.fillEllipse(center.x - 5.0f, center.y - 5.0f, 10.0f, 10.0f);
        }

        // 5. Virtual Stereo Microphones (L & R) synced to mic_spread & mic_distance
        const float spread = micSpreadParam_ ? micSpreadParam_->load() : 0.72f;
        const float dist   = micDistParam_   ? micDistParam_->load()   : 0.35f;
        const float micOffsetRad = 0.25f + spread * (1.5707963f - 0.25f);
        const float micOrbitR    = radius * (0.82f + dist * 0.20f);

        auto drawMic = [&](float angleRad, const juce::String& label)
        {
            const float mx = center.x + std::sin(angleRad) * micOrbitR;
            const float my = center.y - std::cos(angleRad) * micOrbitR;

            g.setColour(juce::Colour(0xffffb74d));
            g.fillEllipse(mx - 5.0f, my - 5.0f, 10.0f, 10.0f);
            g.setColour(juce::Colours::black);
            g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
            g.drawText(label, static_cast<int>(mx - 5), static_cast<int>(my - 5), 10, 10,
                       juce::Justification::centred, false);
        };

        drawMic(-micOffsetRad, "L");
        drawMic(+micOffsetRad, "R");

        // 6. Bottom Instantaneous RPM Telemetry Bar
        const auto footer = bounds.withTop(bounds.getBottom() - 16.0f).reduced(5.0f, 0.0f);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));

        g.setColour(hornAccent);
        g.drawText("H:" + juce::String(juce::roundToInt(telemetry_.hornRpm)),
                   footer.withRight(footer.getCentreX()).toNearestInt(),
                   juce::Justification::centredLeft, false);

        g.setColour(juce::Colour(0xffd99b4e));
        g.drawText("D:" + juce::String(juce::roundToInt(telemetry_.drumRpm)),
                   footer.withLeft(footer.getCentreX()).toNearestInt(),
                   juce::Justification::centredRight, false);
    }

private:
    void timerCallback() override
    {
        if (getTelemetry_)
        {
            telemetry_ = getTelemetry_();
            repaint();
        }
    }

    juce::AudioProcessorValueTreeState& apvts_;
    TelemetryProvider getTelemetry_;
    std::atomic<float>* micSpreadParam_ = nullptr;
    std::atomic<float>* micDistParam_   = nullptr;
    std::atomic<float>* cabModelParam_  = nullptr;
    DSP::Leslie122::RotorTelemetry telemetry_ {};
};

} // namespace GreenOnions::UI
