#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/Leslie122.h"
#include "../Parameters.h"
#include "TiltRockerSwitch.h"
#include <array>
#include <cmath>
#include <functional>

#if defined(SFA_HAS_STAGE_PNG_ASSETS) && SFA_HAS_STAGE_PNG_ASSETS
 #include "BinaryData.h"
#endif

namespace GreenOnions::UI
{

class StageMicLayerComponent final : public juce::Component,
                                     private juce::Timer
{
public:
    using TelemetryProvider = std::function<DSP::Leslie122::RotorTelemetry()>;

    StageMicLayerComponent(juce::AudioProcessorValueTreeState& state,
                           TelemetryProvider provider)
        : apvts_(state), getTelemetry_(std::move(provider))
    {
        perspectiveParam_ = apvts_.getRawParameterValue(Params::kStagePerspective);
        linkParam_        = apvts_.getRawParameterValue(Params::kMicStereoLink);
        micLXParam_       = apvts_.getRawParameterValue(Params::kMicLX);
        micLYParam_       = apvts_.getRawParameterValue(Params::kMicLY);
        micRXParam_       = apvts_.getRawParameterValue(Params::kMicRX);
        micRYParam_       = apvts_.getRawParameterValue(Params::kMicRY);
        micSpreadParam_   = apvts_.getRawParameterValue(Params::kMicSpread);
        micDistParam_     = apvts_.getRawParameterValue(Params::kMicDistance);
        micLTypeParam_    = apvts_.getRawParameterValue(Params::kMicLType);
        micRTypeParam_    = apvts_.getRawParameterValue(Params::kMicRType);
        cabModelParam_    = apvts_.getRawParameterValue(Params::kLeslieCabinetModel);
        marshallOnParam_  = apvts_.getRawParameterValue(Params::kMarshallOn);
        marshallGainParam_= apvts_.getRawParameterValue(Params::kMarshallGain);
        ampSm57XParam_    = apvts_.getRawParameterValue(Params::kAmpSm57X);
        ampSm57YParam_    = apvts_.getRawParameterValue(Params::kAmpSm57Y);
        ampC414XParam_    = apvts_.getRawParameterValue(Params::kAmpC414X);
        ampC414YParam_    = apvts_.getRawParameterValue(Params::kAmpC414Y);

        // Parallel Marshall 1962 Bluesbreaker 2x12 Control Strip (bottom-right of stage)
        marshallOnSwitch_ = std::make_unique<TiltRockerSwitch>(
            *apvts_.getParameter(Params::kMarshallOn), "1962", "2x12", "AMP", true);
        addAndMakeVisible(*marshallOnSwitch_);

        auto initAmpKnob = [this](juce::Slider& s, juce::Label& l, const char* paramId,
                                  const juce::String& text,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att)
        {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            addAndMakeVisible(s);
            l.setText(text, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            l.setColour(juce::Label::textColourId, juce::Colour(0xffd99b4e));
            addAndMakeVisible(l);
            att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts_, paramId, s);
        };
        initAmpKnob(marshallDriveSlider_, marshallDriveLabel_, Params::kMarshallGain, "DRIVE",    marshallDriveAtt_);
        initAmpKnob(marshallToneSlider_,  marshallToneLabel_,  Params::kMarshallTone, "PRESENCE", marshallToneAtt_);
        initAmpKnob(marshallMixSlider_,   marshallMixLabel_,   Params::kMarshallMix,  "BLEND",    marshallMixAtt_);

        leftMicBox_.addItemList({ "L: Neumann U47", "L: AKG C414", "L: Royer R-121", "L: Shure SM57" }, 1);
        rightMicBox_.addItemList({ "R: Neumann U47", "R: AKG C414", "R: Royer R-121", "R: Shure SM57" }, 1);
        cabinetModelBox_.addItemList({ "CAB: LESLIE 122", "CAB: LESLIE 147" }, 1);
        addAndMakeVisible(leftMicBox_);
        addAndMakeVisible(rightMicBox_);
        addAndMakeVisible(cabinetModelBox_);
        leftMicAtt_      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts_, Params::kMicLType, leftMicBox_);
        rightMicAtt_     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts_, Params::kMicRType, rightMicBox_);
        cabinetModelAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts_, Params::kLeslieCabinetModel, cabinetModelBox_);

        // Two-way sync: when console MIC SPREAD or DISTANCE knobs move while on the Console View
        // (stage overlay hidden), reposition L/R mics on stage. Never fight direct stage mic drags!
        auto syncMicsFromKnobs = [this](float)
        {
            if (isVisible() || draggedMic_ != 0)
                return;
            updateMicPositionsFromPolarKnobs();
        };
        spreadAtt_ = std::make_unique<juce::ParameterAttachment>(*apvts_.getParameter(Params::kMicSpread), syncMicsFromKnobs);
        distAtt_   = std::make_unique<juce::ParameterAttachment>(*apvts_.getParameter(Params::kMicDistance), syncMicsFromKnobs);
        marshallOnAtt_ = std::make_unique<juce::ParameterAttachment>(*apvts_.getParameter(Params::kMarshallOn),
            [this](float) { repaint(); });
        linkAtt_   = std::make_unique<juce::ParameterAttachment>(*apvts_.getParameter(Params::kMicStereoLink),
            [this](float v) { linkMicsBtn_.setToggleState(v > 0.5f, juce::dontSendNotification); });
        perspAtt_  = std::make_unique<juce::ParameterAttachment>(*apvts_.getParameter(Params::kStagePerspective),
            [this](float v)
            {
                const bool stg = (v > 0.5f);
                topDownBtn_.setToggleState(!stg, juce::dontSendNotification);
                stageViewBtn_.setToggleState(stg, juce::dontSendNotification);
                rebuildStageBackdrop();
                repaint();
            });

        topDownBtn_.setClickingTogglesState(true);
        stageViewBtn_.setClickingTogglesState(true);
        topDownBtn_.setRadioGroupId(401);
        stageViewBtn_.setRadioGroupId(401);

        const bool initStage = perspectiveParam_ && (perspectiveParam_->load() > 0.5f);
        topDownBtn_.setToggleState(!initStage, juce::dontSendNotification);
        stageViewBtn_.setToggleState(initStage, juce::dontSendNotification);

        auto styleTab = [](juce::TextButton& b)
        {
            b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff18191d));
            b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a351e));
            b.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xffb8ae9c));
            b.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xffffcf87));
        };
        styleTab(topDownBtn_);
        styleTab(stageViewBtn_);
        styleTab(linkMicsBtn_);

        linkMicsBtn_.setClickingTogglesState(true);
        linkMicsBtn_.setToggleState(linkParam_ ? (linkParam_->load() > 0.5f) : true, juce::dontSendNotification);

        topDownBtn_.onClick   = [this] { setPerspective(false); };
        stageViewBtn_.onClick = [this] { setPerspective(true); };
        linkMicsBtn_.onClick  = [this]
        {
            if (auto* p = apvts_.getParameter(Params::kMicStereoLink))
                p->setValueNotifyingHost(linkMicsBtn_.getToggleState() ? 1.0f : 0.0f);
        };

        addAndMakeVisible(topDownBtn_);
        addAndMakeVisible(stageViewBtn_);
        addAndMakeVisible(linkMicsBtn_);

        loadStageSprites();
        startTimerHz(60);
    }

    ~StageMicLayerComponent() override
    {
        stopTimer();
    }

    void resized() override
    {
        topDownBtn_.setBounds(52, 12, 114, 26);
        stageViewBtn_.setBounds(172, 12, 110, 26);
        leftMicBox_.setBounds(290, 12, 134, 26);
        rightMicBox_.setBounds(430, 12, 134, 26);
        cabinetModelBox_.setBounds(572, 12, 136, 26);
        linkMicsBtn_.setBounds(getWidth() - 182, 12, 130, 26);

        // Marshall 1962 Bluesbreaker 2x12 Control Strip at bottom-right of stage
        marshallStripBounds_ = juce::Rectangle<int>(getWidth() - 328, getHeight() - 84, 276, 76);
        auto strip = marshallStripBounds_.withTrimmedTop(15).reduced(6, 3);
        marshallOnSwitch_->setBounds(strip.removeFromLeft(48));
        strip.removeFromLeft(6);
        const int kw = strip.getWidth() / 3;
        auto layoutAmpKnob = [](juce::Rectangle<int> r, juce::Slider& s, juce::Label& l)
        {
            l.setBounds(r.removeFromBottom(12));
            s.setBounds(r);
        };
        layoutAmpKnob(strip.removeFromLeft(kw), marshallDriveSlider_, marshallDriveLabel_);
        layoutAmpKnob(strip.removeFromLeft(kw), marshallToneSlider_,  marshallToneLabel_);
        layoutAmpKnob(strip,                    marshallMixSlider_,   marshallMixLabel_);

        rebuildStageBackdrop();
    }

    void paint(juce::Graphics& g) override
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

        // LAYER 1: Cached Maple Stage Floor & Pleated Black Stage Curtains
        if (backdropCache_.isValid())
            g.drawImageAt(backdropCache_, 0, 0);

        const bool isStageView  = perspectiveParam_ && (perspectiveParam_->load() > 0.5f);
        const auto leslieCenter = getLeslieCenter(isStageView);

        // LAYER 2: Hammond B-3 Console + Live Spinning Leslie 122 Cabinet
        if (isStageView)
            paintFrontStageRig(g, leslieCenter);
        else
            paintTopDownRig(g, leslieCenter);

        // LAYER 3: Movable Stereo Microphones (L & R) with Boom Stands & Polar Halos
        paintMovableMics(g, leslieCenter, isStageView);

        // LAYER 4: Marshall 1962 Bluesbreaker 2x12 Control Strip Backdrop
        if (!marshallStripBounds_.isEmpty())
        {
            const auto r = marshallStripBounds_.toFloat();
            g.setColour(juce::Colour(0xff141312).withAlpha(0.90f));
            g.fillRoundedRectangle(r, 5.0f);
            g.setColour(juce::Colour(0xffc99e52).withAlpha(0.85f));
            g.drawRoundedRectangle(r, 5.0f, 1.2f);
            g.setColour(juce::Colour(0xfff0d49a));
            g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            g.drawText("MARSHALL 1962 BLUESBREAKER 2x12",
                       marshallStripBounds_.withHeight(16).reduced(8, 1),
                       juce::Justification::centredLeft, false);
        }
    }

    int hitTestMic(juce::Point<float> pt) const noexcept
    {
        auto hitCapsuleOrTag = [&](juce::Point<float> center) noexcept
        {
            // Generous hitbox covering both the 48x48 capsule sprite and the label badge beneath it
            return juce::Rectangle<float>(center.x - 32.0f, center.y - 32.0f, 64.0f, 74.0f).contains(pt);
        };

        if (hitCapsuleOrTag(getMicPixelPos(true)))  return 1; // Leslie L
        if (hitCapsuleOrTag(getMicPixelPos(false))) return 2; // Leslie R

        if (marshallOnParam_ && marshallOnParam_->load() > 0.5f)
        {
            if (hitCapsuleOrTag(getAmpMicPixelPos(true)))  return 3; // Bluesbreaker SM57
            if (hitCapsuleOrTag(getAmpMicPixelPos(false))) return 4; // Bluesbreaker C414
        }
        return 0;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = hitTestMic(e.position);
        setMouseCursor(hit != 0 ? juce::MouseCursor::PointingHandCursor
                                : juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int hit = hitTestMic(e.position);

        if (e.mods.isPopupMenu())
        {
            if (hit == 1 || hit == 2)
            {
                juce::PopupMenu menu;
                menu.addItem(1, "Neumann U47 (Tube Condenser)");
                menu.addItem(2, "AKG C414 (FET Condenser)");
                menu.addItem(3, "Royer R-121 (Figure-8 Ribbon)");
                menu.addItem(4, "Shure SM57 (Dynamic)");

                menu.showMenuAsync(juce::PopupMenu::Options(), [this, hit](int choice)
                {
                    if (choice >= 1 && choice <= 4)
                    {
                        const char* paramId = (hit == 1) ? Params::kMicLType : Params::kMicRType;
                        if (auto* p = apvts_.getParameter(paramId))
                            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(choice - 1)));
                        repaint();
                    }
                });
                return;
            }
        }

        draggedMic_ = hit;
        if (draggedMic_ != 0)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggedMic_ == 0 || getWidth() <= 0 || getHeight() <= 0)
            return;

        auto setParam = [this](const char* id, float val)
        {
            if (auto* p = apvts_.getParameter(id))
                p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, val));
        };

        const float nx = juce::jlimit(0.06f, 0.94f, e.position.x / static_cast<float>(getWidth()));
        const float ny = juce::jlimit(0.12f, 0.88f, e.position.y / static_cast<float>(getHeight()));

        // Dragging Bluesbreaker SM57 (3) or AKG C414 (4)
        if (draggedMic_ == 3 || draggedMic_ == 4)
        {
            if (draggedMic_ == 3)
            {
                setParam(Params::kAmpSm57X, nx);
                setParam(Params::kAmpSm57Y, ny);
            }
            else
            {
                setParam(Params::kAmpC414X, nx);
                setParam(Params::kAmpC414Y, ny);
            }
            repaint();
            return;
        }

        const bool isStageView = perspectiveParam_ && (perspectiveParam_->load() > 0.5f);
        const auto center      = getLeslieCenter(isStageView);
        const float cxNorm     = center.x / static_cast<float>(getWidth());

        const bool linked = linkMicsBtn_.getToggleState();

        if (draggedMic_ == 1)
        {
            setParam(Params::kMicLX, nx);
            setParam(Params::kMicLY, ny);
            if (linked)
            {
                const float mirroredX = juce::jlimit(0.06f, 0.94f, cxNorm + (cxNorm - nx));
                setParam(Params::kMicRX, mirroredX);
                setParam(Params::kMicRY, ny);
            }
        }
        else if (draggedMic_ == 2)
        {
            setParam(Params::kMicRX, nx);
            setParam(Params::kMicRY, ny);
            if (linked)
            {
                const float mirroredX = juce::jlimit(0.06f, 0.94f, cxNorm - (nx - cxNorm));
                setParam(Params::kMicLX, mirroredX);
                setParam(Params::kMicLY, ny);
            }
        }

        syncDspLeslieMics(isStageView);
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggedMic_ = 0;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

private:
    void setPerspective(bool useStageView)
    {
        if (auto* p = apvts_.getParameter(Params::kStagePerspective))
            p->setValueNotifyingHost(useStageView ? 1.0f : 0.0f);
        rebuildStageBackdrop();
        repaint();
    }

    juce::Point<float> getLeslieCenter(bool isStageView) const noexcept
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        return isStageView ? juce::Point<float>(w * 0.67f, h * 0.52f)
                           : juce::Point<float>(w * 0.67f, h * 0.54f);
    }

    juce::Point<float> getMicPixelPos(bool isLeft) const noexcept
    {
        const float mx = isLeft ? (micLXParam_ ? micLXParam_->load() : 0.56f)
                                : (micRXParam_ ? micRXParam_->load() : 0.78f);
        const float my = isLeft ? (micLYParam_ ? micLYParam_->load() : 0.36f)
                                : (micRYParam_ ? micRYParam_->load() : 0.36f);
        return { mx * static_cast<float>(getWidth()), my * static_cast<float>(getHeight()) };
    }

    juce::Point<float> getAmpMicPixelPos(bool isSm57) const noexcept
    {
        const float mx = isSm57 ? (ampSm57XParam_ ? ampSm57XParam_->load() : 0.30f)
                                : (ampC414XParam_ ? ampC414XParam_->load() : 0.38f);
        const float my = isSm57 ? (ampSm57YParam_ ? ampSm57YParam_->load() : 0.62f)
                                : (ampC414YParam_ ? ampC414YParam_->load() : 0.78f);
        return { mx * static_cast<float>(getWidth()), my * static_cast<float>(getHeight()) };
    }

    // Convert 2D stage mic positions into Leslie122 azimuth spread & radial distance
    void syncDspLeslieMics(bool isStageView)
    {
        lastLeslieDragMs_ = juce::Time::getMillisecondCounterHiRes();
        const auto center = getLeslieCenter(isStageView);
        const auto posL   = getMicPixelPos(true);
        const auto posR   = getMicPixelPos(false);

        const float distL     = center.getDistanceFrom(posL);
        const float distR     = center.getDistanceFrom(posR);
        const float avgDistPx = 0.5f * (distL + distR);

        // Map 65px (close-miked louvers) .. 260px (room mic) -> 0.0 .. 1.0
        const float normDist   = juce::jlimit(0.0f, 1.0f, (avgDistPx - 65.0f) / 195.0f);
        const float dx         = std::abs(posR.x - posL.x);
        const float normSpread = juce::jlimit(0.0f, 1.0f, dx / 320.0f);

        if (auto* pDist = apvts_.getParameter(Params::kMicDistance))
            pDist->setValueNotifyingHost(normDist);
        if (auto* pSpread = apvts_.getParameter(Params::kMicSpread))
            pSpread->setValueNotifyingHost(normSpread);
        lastLeslieDragMs_ = juce::Time::getMillisecondCounterHiRes();
    }

    // Reverse sync: convert console MIC SPREAD & DISTANCE knobs back into symmetric stage mic coordinates
    void updateMicPositionsFromPolarKnobs()
    {
        const float spread = micSpreadParam_ ? micSpreadParam_->load() : 0.72f;
        const float dist   = micDistParam_   ? micDistParam_->load()   : 0.35f;

        const float refW = (getWidth()  > 100) ? static_cast<float>(getWidth())  : 1204.0f;
        const float refH = (getHeight() > 100) ? static_cast<float>(getHeight()) : 688.0f;

        const float radiusPx  = 65.0f + dist * 195.0f;
        const float halfDxPx  = juce::jmin(radiusPx * 0.96f, 0.5f * spread * 320.0f);
        const float dyPx      = std::sqrt(juce::jmax(0.0f, radiusPx * radiusPx - halfDxPx * halfDxPx));

        const float lx = juce::jlimit(0.36f, 0.94f, 0.67f - halfDxPx / refW);
        const float rx = juce::jlimit(0.36f, 0.94f, 0.67f + halfDxPx / refW);
        const float my = juce::jlimit(0.14f, 0.88f, 0.54f - dyPx / refH);

        auto setSilent = [this](const char* id, float val)
        {
            if (auto* p = apvts_.getParameter(id))
                p->setValueNotifyingHost(val);
        };
        setSilent(Params::kMicLX, lx);
        setSilent(Params::kMicLY, my);
        setSilent(Params::kMicRX, rx);
        setSilent(Params::kMicRY, my);
    }

    //==========================================================================
    // LAYER 1: Cached Maple Stage Floor + Pleated Black Stage Curtains
    //==========================================================================
    void rebuildStageBackdrop()
    {
        const int w = juce::jmax(1, getWidth());
        const int h = juce::jmax(1, getHeight());
        backdropCache_ = juce::Image(juce::Image::RGB, w, h, false);
        juce::Graphics g(backdropCache_);

        const bool isStageView = perspectiveParam_ && (perspectiveParam_->load() > 0.5f);

        // If a custom PNG stage backdrop sprite was embedded via BinaryData, use it directly
        const auto& customBackdrop = isStageView ? sprites_.backdropFront : sprites_.backdropTop;
        if (customBackdrop.isValid())
        {
            g.drawImage(customBackdrop, 0, 0, w, h, 0, 0, customBackdrop.getWidth(), customBackdrop.getHeight());
            return;
        }

        const float horizonY = isStageView ? static_cast<float>(h) * 0.44f : 52.0f;

        // 1. Back Wall & Pleated Black Stage Curtains
        g.setColour(juce::Colour(0xff0b0b0d));
        g.fillRect(0.0f, 0.0f, static_cast<float>(w), horizonY);

        const float foldWidth = 22.0f;
        for (float x = 0.0f; x < static_cast<float>(w); x += foldWidth)
        {
            juce::ColourGradient pleat(juce::Colour(0xff09090b), x, 0.0f,
                                       juce::Colour(0xff202126), x + foldWidth * 0.5f, 0.0f, false);
            pleat.addColour(1.0, juce::Colour(0xff0a0a0c));
            g.setGradientFill(pleat);
            g.fillRect(x, 0.0f, foldWidth, horizonY);
        }

        // 2. Warm Honey-Maple Stage Floor
        juce::ColourGradient mapleBase(juce::Colour(0xff8c5a32), static_cast<float>(w) * 0.5f, horizonY,
                                       juce::Colour(0xff59351a), static_cast<float>(w) * 0.5f, static_cast<float>(h), false);
        g.setGradientFill(mapleBase);
        g.fillRect(0.0f, horizonY, static_cast<float>(w), static_cast<float>(h) - horizonY);

        // Maple Plank Boards & Grain Lines
        const float plankH = isStageView ? 18.0f : 24.0f;
        int row = 0;
        for (float y = horizonY; y < static_cast<float>(h); y += plankH)
        {
            const float shade = static_cast<float>((row % 3) - 1) * 0.04f;
            g.setColour(shade >= 0.0f ? juce::Colours::white.withAlpha(shade)
                                      : juce::Colours::black.withAlpha(-shade));
            g.fillRect(0.0f, y, static_cast<float>(w), plankH);

            g.setColour(juce::Colour(0xff38200f).withAlpha(0.55f));
            g.drawLine(0.0f, y, static_cast<float>(w), y, 1.0f);

            const float jointOffset = (row % 2 == 0) ? 45.0f : 135.0f;
            for (float jx = jointOffset; jx < static_cast<float>(w); jx += 180.0f)
                g.drawLine(jx, y, jx, y + plankH, 1.0f);

            ++row;
        }

        // 3. Stage Overhead Spotlight Pool
        juce::ColourGradient spotlight(juce::Colour(0xffffdca8).withAlpha(0.16f),
                                       static_cast<float>(w) * 0.52f, static_cast<float>(h) * 0.55f,
                                       juce::Colours::transparentBlack,
                                       static_cast<float>(w) * 0.52f, static_cast<float>(h) * 0.05f, true);
        g.setGradientFill(spotlight);
        g.fillAll();

        // 3b. Warm Stage Footlight Glow Pools along front lip
        for (int f = 0; f < 3; ++f)
        {
            const float fx = static_cast<float>(w) * (0.24f + 0.26f * static_cast<float>(f));
            const float fy = static_cast<float>(h) - 4.0f;
            juce::ColourGradient foot(juce::Colour(0xffffc878).withAlpha(0.14f), fx, fy,
                                      juce::Colours::transparentBlack, fx, fy - 75.0f, true);
            g.setGradientFill(foot);
            g.fillEllipse(fx - 95.0f, fy - 65.0f, 190.0f, 120.0f);
        }

        // 4. Side Black Wing Curtains framing left & right edges
        auto drawWingCurtain = [&](float startX, float width)
        {
            for (float x = startX; x < startX + width; x += 16.0f)
            {
                juce::ColourGradient fold(juce::Colour(0xff070709), x, 0.0f,
                                          juce::Colour(0xff1e1f24), x + 8.0f, 0.0f, false);
                fold.addColour(1.0, juce::Colour(0xff070709));
                g.setGradientFill(fold);
                g.fillRect(x, 0.0f, 16.0f, static_cast<float>(h));
            }
        };
        drawWingCurtain(0.0f, 42.0f);
        drawWingCurtain(static_cast<float>(w) - 42.0f, 42.0f);
    }

    //==========================================================================
    // LAYER 2A: Top-Down Plan View of B-3 Console & Live Leslie 122
    //==========================================================================
    void paintTopDownRig(juce::Graphics& g, juce::Point<float> leslieCenter)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // 1. Top-Down Hammond B-3 Console (Left Side)
        const juce::Rectangle<float> b3Body(w * 0.06f, h * 0.22f, 224.0f, 134.0f);
        if (sprites_.b3Top.isValid())
        {
            g.drawImageWithin(sprites_.b3Top, static_cast<int>(b3Body.getX()), static_cast<int>(b3Body.getY()),
                              static_cast<int>(b3Body.getWidth()), static_cast<int>(b3Body.getHeight() + 46.0f),
                              juce::RectanglePlacement::centred);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.45f));
            g.fillRoundedRectangle(b3Body.translated(6.0f, 8.0f), 8.0f);

            g.setColour(juce::Colour(0xff3d2314));
            g.fillRoundedRectangle(b3Body, 8.0f);
            g.setColour(juce::Colour(0xff784c30));
            g.drawRoundedRectangle(b3Body, 8.0f, 2.0f);

            g.setColour(juce::Colour(0xfff0e8d5));
            g.fillRect(b3Body.getX() + 16.0f, b3Body.getBottom() - 40.0f, b3Body.getWidth() - 32.0f, 14.0f);
            g.fillRect(b3Body.getX() + 16.0f, b3Body.getBottom() - 22.0f, b3Body.getWidth() - 32.0f, 14.0f);

            // Walnut Organ Bench behind console
            const juce::Rectangle<float> bench(b3Body.getX() + 28.0f, b3Body.getBottom() + 12.0f, 168.0f, 34.0f);
            g.setColour(juce::Colour(0xff331d11));
            g.fillRoundedRectangle(bench, 5.0f);
            g.setColour(juce::Colour(0xff6b432a));
            g.drawRoundedRectangle(bench, 5.0f, 1.5f);

            g.setColour(juce::Colour(0xffeedcc0));
            g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
            g.drawText("HAMMOND B-3 CONSOLE", b3Body.withHeight(30.0f).toNearestInt(), juce::Justification::centred, false);
        }

        // 2. Top-Down Leslie 122 / 147 Cabinet with Live Spinning Rotor Cutaway
        const float cabSize = 132.0f;
        const juce::Rectangle<float> cabRect(leslieCenter.x - cabSize * 0.5f,
                                             leslieCenter.y - cabSize * 0.5f,
                                             cabSize, cabSize);
        const bool is147 = cabModelParam_ && (cabModelParam_->load() > 0.5f);
        const auto& customCabTop = is147 ? sprites_.leslie147Top : sprites_.leslie122Top;
        if (customCabTop.isValid())
        {
            g.drawImageWithin(customCabTop, static_cast<int>(cabRect.getX()), static_cast<int>(cabRect.getY()),
                              static_cast<int>(cabRect.getWidth()), static_cast<int>(cabRect.getHeight()),
                              juce::RectanglePlacement::centred);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.50f));
            g.fillRoundedRectangle(cabRect.translated(6.0f, 8.0f), 10.0f);

            g.setColour(juce::Colour(0xff3d2314));
            g.fillRoundedRectangle(cabRect, 10.0f);
            g.setColour(juce::Colour(0xff8c5a38));
            g.drawRoundedRectangle(cabRect, 10.0f, 2.0f);

            g.setColour(juce::Colour(0xff101114));
            g.fillEllipse(cabRect.reduced(12.0f));
        }

        // Spinning Treble Horn & Acoustic Beam (always overlaid live at 60 fps)
        const juce::Colour beamCol  = is147 ? juce::Colour(0xffff7043) : juce::Colour(0xff69f0ae);
        const juce::Colour badgeCol = is147 ? juce::Colour(0xffff8a65) : juce::Colour(0xffeedcc0);

        const float radius = cabSize * 0.42f;
        {
            juce::Graphics::ScopedSaveState save(g);
            g.addTransform(juce::AffineTransform::rotation(telemetry_.hornAngleRad, leslieCenter.x, leslieCenter.y));

            juce::Path beam;
            beam.addPieSegment(leslieCenter.x - radius * 1.35f, leslieCenter.y - radius * 1.35f,
                               radius * 2.7f, radius * 2.7f, -0.35f, 0.35f, 0.0f);
            g.setColour(beamCol.withAlpha(0.26f));
            g.fillPath(beam);

            g.setColour(juce::Colour(0xffebe6dc));
            g.fillRoundedRectangle(leslieCenter.x - 5.0f, leslieCenter.y - radius, 10.0f, radius * 2.0f, 3.0f);
            g.setColour(beamCol);
            g.fillEllipse(leslieCenter.x - 6.0f, leslieCenter.y - radius - 2.0f, 12.0f, 8.0f);
        }

        const juce::String cabLabel = is147 ? "LESLIE 147  (" : "LESLIE 122  (";
        g.setColour(badgeCol);
        g.drawText(cabLabel + juce::String(juce::roundToInt(telemetry_.hornRpm)) + " RPM)",
                   cabRect.translated(0.0f, cabSize * 0.5f + 22.0f).withHeight(20.0f).toNearestInt(),
                   juce::Justification::centred, false);

        // 3. Top-Down Marshall 1962 Bluesbreaker 2x12 Combo (between B-3 and Leslie when engaged)
        if (marshallOnParam_ && (marshallOnParam_->load() > 0.5f))
        {
            const float gainVal = marshallGainParam_ ? marshallGainParam_->load() : 0.65f;
            const juce::Rectangle<float> ampRect(b3Body.getRight() + 32.0f, b3Body.getY() + 26.0f, 134.0f, 76.0f);

            if (sprites_.marshallTop.isValid())
            {
                g.drawImageWithin(sprites_.marshallTop, static_cast<int>(ampRect.getX()), static_cast<int>(ampRect.getY()),
                                  static_cast<int>(ampRect.getWidth()), static_cast<int>(ampRect.getHeight()),
                                  juce::RectanglePlacement::centred);
            }
            else
            {
                // Drop shadow
                g.setColour(juce::Colours::black.withAlpha(0.50f));
                g.fillRoundedRectangle(ampRect.translated(5.0f, 7.0f), 5.0f);

                // Black Levant Tolex housing + Gold Piping
                g.setColour(juce::Colour(0xff18181b));
                g.fillRoundedRectangle(ampRect, 5.0f);
                g.setColour(juce::Colour(0xffd4af37));
                g.drawRoundedRectangle(ampRect.reduced(2.5f), 3.5f, 1.4f);

                // Top recessed gold Plexi control panel strip
                const auto plexi = ampRect.reduced(8.0f, 8.0f).withHeight(18.0f);
                g.setColour(juce::Colour(0xffc99e46));
                g.fillRoundedRectangle(plexi, 2.0f);

                // Warm amber jewel pilot lamp (brightness scales with marshall_gain)
                const float glowAlpha = 0.45f + gainVal * 0.55f;
                const float jx = plexi.getRight() - 10.0f;
                const float jy = plexi.getCentreY();
                g.setColour(juce::Colour(0xffff6d00).withAlpha(glowAlpha * 0.45f));
                g.fillEllipse(jx - 8.0f, jy - 8.0f, 16.0f, 16.0f);
                g.setColour(juce::Colour(0xffffab40).withAlpha(glowAlpha));
                g.fillEllipse(jx - 3.5f, jy - 3.5f, 7.0f, 7.0f);
            }

            g.setColour(juce::Colour(0xfff0d49a));
            g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
            g.drawText("1962 2x12 AMP", ampRect.withTop(ampRect.getBottom() + 4.0f).withHeight(16.0f).toNearestInt(),
                       juce::Justification::centred, false);

            // Dedicated Bluesbreaker Studio Mics (Top-Down): SM57 @ 1' (close) & AKG C414 @ 3' (room)
            paintBluesbreakerMics(g, ampRect, false);
        }
    }

    //==========================================================================
    // LAYER 2B: Front/Angled Stage View of Full B-3 Console & Tall Leslie 122
    //==========================================================================
    void paintFrontStageRig(juce::Graphics& g, juce::Point<float> leslieCenter)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        // 1. Full Front-Standing Hammond B-3 Console on Stage
        const juce::Rectangle<float> b3Console(w * 0.06f, h * 0.27f, 240.0f, 174.0f);
        if (sprites_.b3Front.isValid())
        {
            g.drawImageWithin(sprites_.b3Front, static_cast<int>(b3Console.getX()), static_cast<int>(b3Console.getY()),
                              static_cast<int>(b3Console.getWidth()), static_cast<int>(b3Console.getHeight()),
                              juce::RectanglePlacement::centred);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.50f));
            g.fillEllipse(b3Console.getX() - 10.0f, b3Console.getBottom() - 10.0f, b3Console.getWidth() + 20.0f, 22.0f);

            g.setColour(juce::Colour(0xff2b170c));
            g.fillRect(b3Console.getX() + 14.0f, b3Console.getY() + 80.0f, 16.0f, b3Console.getHeight() - 80.0f);
            g.fillRect(b3Console.getRight() - 30.0f, b3Console.getY() + 80.0f, 16.0f, b3Console.getHeight() - 80.0f);

            g.setColour(juce::Colour(0xff472917));
            g.fillRoundedRectangle(b3Console.withHeight(84.0f), 6.0f);
            g.setColour(juce::Colour(0xff8c5a38));
            g.drawRoundedRectangle(b3Console.withHeight(84.0f), 6.0f, 1.8f);

            g.setColour(juce::Colour(0xfff2ead8));
            g.fillRect(b3Console.getX() + 20.0f, b3Console.getY() + 50.0f, b3Console.getWidth() - 40.0f, 10.0f);
            g.fillRect(b3Console.getX() + 20.0f, b3Console.getY() + 64.0f, b3Console.getWidth() - 40.0f, 10.0f);

            g.setColour(juce::Colour(0xff6b4428));
            g.fillRect(b3Console.getX() + 42.0f, b3Console.getBottom() - 12.0f, b3Console.getWidth() - 84.0f, 11.0f);
        }

        // 2. Tall Louvered Leslie 122 / 147 Cabinet on Stage
        const juce::Rectangle<float> leslieCab(leslieCenter.x - 62.0f, leslieCenter.y - 96.0f, 124.0f, 196.0f);
        const bool is147 = cabModelParam_ && (cabModelParam_->load() > 0.5f);
        const auto& customCabFront = is147 ? sprites_.leslie147Front : sprites_.leslie122Front;
        if (customCabFront.isValid())
        {
            g.drawImageWithin(customCabFront, static_cast<int>(leslieCab.getX()), static_cast<int>(leslieCab.getY()),
                              static_cast<int>(leslieCab.getWidth()), static_cast<int>(leslieCab.getHeight()),
                              juce::RectanglePlacement::centred);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.fillEllipse(leslieCab.getX() - 8.0f, leslieCab.getBottom() - 10.0f, leslieCab.getWidth() + 16.0f, 22.0f);

            g.setColour(juce::Colour(0xff472917));
            g.fillRoundedRectangle(leslieCab, 6.0f);
            g.setColour(juce::Colour(0xff8c5a38));
            g.drawRoundedRectangle(leslieCab, 6.0f, 1.8f);

            auto drawLouvers = [&](juce::Rectangle<float> zone)
            {
                g.setColour(juce::Colour(0xff14110f));
                g.fillRoundedRectangle(zone, 3.0f);
                g.setColour(juce::Colour(0xff5c3720));
                for (float ly = zone.getY() + 6.0f; ly < zone.getBottom() - 4.0f; ly += 8.0f)
                    g.fillRect(zone.getX() + 4.0f, ly, zone.getWidth() - 8.0f, 3.0f);
            };
            drawLouvers(leslieCab.reduced(14.0f, 16.0f).withHeight(66.0f));
            drawLouvers(leslieCab.reduced(14.0f, 16.0f).withTop(leslieCab.getCentreY() + 8.0f));
        }

        // Animated horizontal horn sweep glow through top louvers (always live at 60 fps)
        const float sweepX = leslieCab.getCentreX() + std::sin(telemetry_.hornAngleRad) * 42.0f;
        g.setColour((is147 ? juce::Colour(0xffff7043) : juce::Colour(0xff69f0ae)).withAlpha(0.45f));
        g.fillEllipse(sweepX - 14.0f, leslieCab.getY() + 34.0f, 28.0f, 28.0f);

        // Cabinet nameplate badge on front rail
        const juce::Rectangle<float> badge(leslieCab.getCentreX() - 38.0f, leslieCab.getCentreY() - 10.0f, 76.0f, 16.0f);
        g.setColour(is147 ? juce::Colour(0xff5c1e16) : juce::Colour(0xff2c2116));
        g.fillRoundedRectangle(badge, 3.0f);
        g.setColour(is147 ? juce::Colour(0xffff8a65) : juce::Colour(0xffd99b4e));
        g.drawRoundedRectangle(badge, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText(is147 ? "LESLIE 147" : "LESLIE 122", badge.toNearestInt(), juce::Justification::centred, false);

        // 3. Front-Standing Marshall 1962 Bluesbreaker 2x12 Combo on Stage (between B-3 and Leslie)
        if (marshallOnParam_ && (marshallOnParam_->load() > 0.5f))
        {
            const float gainVal = marshallGainParam_ ? marshallGainParam_->load() : 0.65f;
            const juce::Rectangle<float> ampCab(b3Console.getRight() + 28.0f, b3Console.getBottom() - 114.0f, 146.0f, 114.0f);

            if (sprites_.marshallFront.isValid())
            {
                g.drawImageWithin(sprites_.marshallFront, static_cast<int>(ampCab.getX()), static_cast<int>(ampCab.getY()),
                                  static_cast<int>(ampCab.getWidth()), static_cast<int>(ampCab.getHeight()),
                                  juce::RectanglePlacement::centred);
            }
            else
            {
                // Floor shadow
                g.setColour(juce::Colours::black.withAlpha(0.55f));
                g.fillEllipse(ampCab.getX() - 6.0f, ampCab.getBottom() - 8.0f, ampCab.getWidth() + 12.0f, 18.0f);

                // Black Levant Tolex Cabinet & Gold Piping
                g.setColour(juce::Colour(0xff161619));
                g.fillRoundedRectangle(ampCab, 5.0f);
                g.setColour(juce::Colour(0xffd4af37));
                g.drawRoundedRectangle(ampCab.reduced(3.0f), 3.5f, 1.5f);

                // Gold Plexi top control strip with amber pilot jewel
                const auto plexi = ampCab.reduced(9.0f, 8.0f).withHeight(18.0f);
                g.setColour(juce::Colour(0xffc99e46));
                g.fillRoundedRectangle(plexi, 2.0f);
                const float glowAlpha = 0.45f + gainVal * 0.55f;
                const float jx = plexi.getRight() - 10.0f;
                const float jy = plexi.getCentreY();
                g.setColour(juce::Colour(0xffff6d00).withAlpha(glowAlpha * 0.50f));
                g.fillEllipse(jx - 8.0f, jy - 8.0f, 16.0f, 16.0f);
                g.setColour(juce::Colour(0xffffab40).withAlpha(glowAlpha));
                g.fillEllipse(jx - 3.5f, jy - 3.5f, 7.0f, 7.0f);

                // Salt-and-pepper grille cloth + two 12" Celestion Alnico speaker cones
                const auto grille = ampCab.withTop(plexi.getBottom() + 5.0f).reduced(9.0f, 8.0f);
                g.setColour(juce::Colour(0xff3c3a36));
                g.fillRoundedRectangle(grille, 2.5f);

                g.setColour(juce::Colours::white.withAlpha(0.08f));
                for (float gy = grille.getY() + 2.0f; gy < grille.getBottom(); gy += 4.0f)
                    g.drawHorizontalLine(static_cast<int>(gy), grille.getX(), grille.getRight());

                const float spkR = juce::jmin(grille.getHeight() * 0.41f, grille.getWidth() * 0.22f);
                const float cy   = grille.getCentreY();
                const float cxL  = grille.getX() + grille.getWidth() * 0.28f;
                const float cxR  = grille.getX() + grille.getWidth() * 0.72f;
                auto drawSpeakerCone = [&](float cx)
                {
                    g.setColour(juce::Colour(0xff141312).withAlpha(0.72f));
                    g.fillEllipse(cx - spkR, cy - spkR, spkR * 2.0f, spkR * 2.0f);
                    g.setColour(juce::Colour(0xff524d45).withAlpha(0.55f));
                    g.drawEllipse(cx - spkR * 0.65f, cy - spkR * 0.65f, spkR * 1.3f, spkR * 1.3f, 1.2f);
                    g.setColour(juce::Colour(0xff232120));
                    g.fillEllipse(cx - spkR * 0.28f, cy - spkR * 0.28f, spkR * 0.56f, spkR * 0.56f);
                };
                drawSpeakerCone(cxL);
                drawSpeakerCone(cxR);

                g.setColour(juce::Colour(0xfff5eedc));
                g.setFont(juce::FontOptions(9.5f, juce::Font::bold | juce::Font::italic));
                g.drawText("Bluesbreaker 1962", grille.withHeight(16.0f).toNearestInt(), juce::Justification::centred, false);
            }

            // Dedicated Bluesbreaker Studio Mics (Front Stage): SM57 @ 1' (close) & AKG C414 @ 3' (room)
            paintBluesbreakerMics(g, ampCab, true);
        }
    }

    //==========================================================================
    // Movable Bluesbreaker 2x12 Studio Mics: Shure SM57 & AKG C414
    //==========================================================================
    void paintBluesbreakerMics(juce::Graphics& g, juce::Rectangle<float> ampRect, bool isStageView)
    {
        const auto sm57Pt = getAmpMicPixelPos(true);
        const auto c414Pt = getAmpMicPixelPos(false);

        const float sm57Ft = juce::jlimit(0.3f, 6.0f, ampRect.getCentre().getDistanceFrom(sm57Pt) / 58.0f);
        const float c414Ft = juce::jlimit(0.3f, 6.0f, ampRect.getCentre().getDistanceFrom(c414Pt) / 58.0f);

        // Acoustic sightlines from twin 12" Celestion cones to each movable mic
        const juce::Point<float> coneL(ampRect.getX() + ampRect.getWidth() * 0.28f, ampRect.getCentreY());
        const juce::Point<float> coneR(ampRect.getX() + ampRect.getWidth() * 0.72f, ampRect.getCentreY());
        const float dash[2] = { 3.0f, 3.0f };
        g.setColour(juce::Colour(0xff8c94a0).withAlpha(0.45f));
        g.drawDashedLine(juce::Line<float>(coneL, sm57Pt), dash, 2, 1.1f);
        g.setColour(juce::Colour(0xffffcf56).withAlpha(0.45f));
        g.drawDashedLine(juce::Line<float>(coneR, c414Pt), dash, 2, 1.1f);

        auto drawAmpMic = [&](juce::Point<float> pt, int modelIdx, const juce::String& badge,
                              juce::Colour accent, bool isDragging)
        {
            if (isStageView)
            {
                const float floorY = juce::jmin(static_cast<float>(getHeight()) - 20.0f, pt.y + 34.0f);
                g.setColour(juce::Colour(0xff1e2024));
                g.drawLine(pt.x, pt.y + 10.0f, pt.x, floorY, 2.0f);
                g.drawLine(pt.x, floorY - 5.0f, pt.x - 10.0f, floorY + 2.0f, 1.6f);
                g.drawLine(pt.x, floorY - 5.0f, pt.x + 10.0f, floorY + 2.0f, 1.6f);
            }

            // Soft pickup halo (brightens while dragging)
            g.setColour(accent.withAlpha(isDragging ? 0.28f : 0.14f));
            g.fillEllipse(pt.x - 19.0f, pt.y - 19.0f, 38.0f, 38.0f);

            const auto& sprite = isStageView ? sprites_.micsFront[static_cast<size_t>(modelIdx)]
                                             : sprites_.micsTop[static_cast<size_t>(modelIdx)];
            if (sprite.isValid())
            {
                g.drawImageWithin(sprite, static_cast<int>(pt.x - 20.0f), static_cast<int>(pt.y - 20.0f), 40, 40,
                                  juce::RectanglePlacement::centred);
                if (isDragging)
                {
                    g.setColour(juce::Colours::white);
                    g.drawEllipse(pt.x - 20.0f, pt.y - 20.0f, 40.0f, 40.0f, 1.6f);
                }
            }
            else
            {
                g.setColour(juce::Colours::black.withAlpha(0.60f));
                g.fillEllipse(pt.x - 9.0f, pt.y - 8.0f, 20.0f, 20.0f);
                g.setColour(juce::Colour(0xff232529));
                g.fillEllipse(pt.x - 10.0f, pt.y - 10.0f, 20.0f, 20.0f);
                g.setColour(isDragging ? juce::Colours::white : accent);
                g.drawEllipse(pt.x - 10.0f, pt.y - 10.0f, 20.0f, 20.0f, 1.8f);
                g.setColour(juce::Colours::white);
                g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
                g.drawText(modelIdx == 3 ? "57" : "414", static_cast<int>(pt.x - 10), static_cast<int>(pt.y - 10),
                           20, 20, juce::Justification::centred, false);
            }

            // Live Distance & Model Badge Pill
            const juce::Rectangle<float> tag(pt.x - 28.0f, pt.y + 17.0f, 56.0f, 14.0f);
            g.setColour(juce::Colour(0xff121316).withAlpha(0.90f));
            g.fillRoundedRectangle(tag, 3.0f);
            g.setColour(accent);
            g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
            g.drawText(badge, tag.toNearestInt(), juce::Justification::centred, false);
        };

        drawAmpMic(sm57Pt, 3, "SM57 " + juce::String(sm57Ft, 1) + "'", juce::Colour(0xff9aa3b0), draggedMic_ == 3);
        drawAmpMic(c414Pt, 1, "C414 " + juce::String(c414Ft, 1) + "'", juce::Colour(0xffffcf56), draggedMic_ == 4);
    }

    //==========================================================================
    // LAYER 3: Draggable Stereo Microphones (L & R) with Stage Boom Stands
    //==========================================================================
    void paintMovableMics(juce::Graphics& g, juce::Point<float> leslieCenter, bool isStageView)
    {
        const auto posL = getMicPixelPos(true);
        const auto posR = getMicPixelPos(false);

        g.setColour(juce::Colour(0xffffb74d).withAlpha(0.28f));
        const float dashLengths[2] = { 4.0f, 4.0f };
        g.drawDashedLine(juce::Line<float>(leslieCenter, posL), dashLengths, 2, 1.2f);
        g.drawDashedLine(juce::Line<float>(leslieCenter, posR), dashLengths, 2, 1.2f);

        // In Front Stage View, draw tripod boom mic stands rising from the maple floor to each capsule
        if (isStageView)
        {
            const float floorY = static_cast<float>(getHeight()) - 28.0f;
            auto drawBoomStand = [&](juce::Point<float> pt, float sideSign)
            {
                const float standX = juce::jlimit(56.0f, static_cast<float>(getWidth()) - 56.0f, pt.x + sideSign * 26.0f);
                const float elbowY = juce::jmin(floorY - 22.0f, pt.y + 38.0f);

                // Tripod floor shadow & base legs
                g.setColour(juce::Colours::black.withAlpha(0.50f));
                g.fillEllipse(standX - 18.0f, floorY - 4.0f, 36.0f, 10.0f);
                g.setColour(juce::Colour(0xff1e2024));
                g.drawLine(standX, floorY - 8.0f, standX - 14.0f, floorY + 2.0f, 2.5f);
                g.drawLine(standX, floorY - 8.0f, standX + 14.0f, floorY + 2.0f, 2.5f);
                g.drawLine(standX, floorY - 8.0f, standX,         floorY + 3.0f, 2.5f);

                // Vertical black chrome shaft
                g.setColour(juce::Colour(0xff2d3036));
                g.drawLine(standX, floorY - 6.0f, standX, elbowY, 3.0f);
                g.setColour(juce::Colours::white.withAlpha(0.22f));
                g.drawLine(standX - 0.8f, floorY - 6.0f, standX - 0.8f, elbowY, 0.8f);

                // Boom arm clutch + angled boom extending to mic capsule
                g.setColour(juce::Colour(0xff525760));
                g.fillEllipse(standX - 4.0f, elbowY - 4.0f, 8.0f, 8.0f);
                g.setColour(juce::Colour(0xff383c44));
                g.drawLine(standX, elbowY, pt.x, pt.y, 2.2f);
            };
            drawBoomStand(posL, -1.0f);
            drawBoomStand(posR, +1.0f);
        }

        const int typeL = micLTypeParam_ ? static_cast<int>(micLTypeParam_->load() + 0.5f) : 0;
        const int typeR = micRTypeParam_ ? static_cast<int>(micRTypeParam_->load() + 0.5f) : 0;

        auto drawMicCapsule = [&](juce::Point<float> pt, const juce::String& chLabel, int modelIdx, bool isDragging)
        {
            static constexpr const char* kShortTags[4] = { "U47", "C414", "R-121", "SM57" };
            const juce::Colour bodyCols[4] = {
                juce::Colour(0xffc9cfd8), // U47 satin nickel
                juce::Colour(0xff2c2e33), // C414 dark anodized + gold grille
                juce::Colour(0xff9ea6b2), // R-121 brushed silver ribbon
                juce::Colour(0xff232529)  // SM57 charcoal dynamic
            };
            const juce::Colour ringCols[4] = {
                juce::Colour(0xffd99b4e), // U47 brass ring
                juce::Colour(0xffffcf56), // C414 gold capsule rim
                juce::Colour(0xff69f0ae), // R-121 emerald ribbon accent
                juce::Colour(0xff8c94a0)  // SM57 steel grille rim
            };

            const int idx = juce::jlimit(0, 3, modelIdx);

            // Polar pickup pattern halo (Figure-8 dual lobe for Royer R-121, Cardioid for others)
            g.setColour(ringCols[idx].withAlpha(isDragging ? 0.25f : 0.12f));
            if (idx == 2)
            {
                g.fillEllipse(pt.x - 16.0f, pt.y - 28.0f, 32.0f, 26.0f);
                g.fillEllipse(pt.x - 16.0f, pt.y + 2.0f,  32.0f, 26.0f);
            }
            else
            {
                g.fillEllipse(pt.x - 22.0f, pt.y - 22.0f, 44.0f, 44.0f);
            }

            // Drop shadow & capsule body (or perspective-matched embedded BinaryData PNG sprite)
            const auto& micSprite = isStageView ? sprites_.micsFront[static_cast<size_t>(idx)]
                                                : sprites_.micsTop[static_cast<size_t>(idx)];
            if (micSprite.isValid())
            {
                g.drawImageWithin(micSprite,
                                  static_cast<int>(pt.x - 24.0f), static_cast<int>(pt.y - 24.0f), 48, 48,
                                  juce::RectanglePlacement::centred);
                if (isDragging)
                {
                    g.setColour(ringCols[idx]);
                    g.drawEllipse(pt.x - 24.0f, pt.y - 24.0f, 48.0f, 48.0f, 1.8f);
                }
            }
            else
            {
                g.setColour(juce::Colours::black.withAlpha(0.65f));
                g.fillEllipse(pt.x - 12.0f, pt.y - 10.0f, 26.0f, 26.0f);

                g.setColour(bodyCols[idx]);
                g.fillEllipse(pt.x - 13.0f, pt.y - 13.0f, 26.0f, 26.0f);
                g.setColour(isDragging ? juce::Colours::white : ringCols[idx]);
                g.drawEllipse(pt.x - 13.0f, pt.y - 13.0f, 26.0f, 26.0f, 2.2f);

                g.setColour((idx == 1 || idx == 3) ? juce::Colours::white : juce::Colour(0xff141518));
                g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
                g.drawText(chLabel, static_cast<int>(pt.x - 13), static_cast<int>(pt.y - 13), 26, 26,
                           juce::Justification::centred, false);
            }

            // Channel + Model tag badge below capsule
            const juce::Rectangle<float> tagRect(pt.x - 26.0f, pt.y + 22.0f, 52.0f, 15.0f);
            g.setColour(juce::Colour(0xff121316).withAlpha(0.90f));
            g.fillRoundedRectangle(tagRect, 3.0f);
            g.setColour(ringCols[idx]);
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.drawText(chLabel + ": " + kShortTags[idx], tagRect.toNearestInt(), juce::Justification::centred, false);
        };

        drawMicCapsule(posL, "L", typeL, draggedMic_ == 1);
        drawMicCapsule(posR, "R", typeR, draggedMic_ == 2);

        const float spread = micSpreadParam_ ? micSpreadParam_->load() : 0.72f;
        const float dist   = micDistParam_   ? micDistParam_->load()   : 0.35f;
        const juce::String readout = "STEREO ANGLE: " + juce::String(juce::roundToInt(35.0f + spread * 135.0f))
                                   + " deg   |   MIC DISTANCE: " + juce::String(0.3f + dist * 2.7f, 1) + " ft";

        const juce::Rectangle<float> pill(54.0f, static_cast<float>(getHeight()) - 38.0f, 360.0f, 24.0f);
        g.setColour(juce::Colour(0xff121316).withAlpha(0.85f));
        g.fillRoundedRectangle(pill, 4.0f);
        g.setColour(juce::Colour(0xfff0e4ce));
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(readout, pill.toNearestInt(), juce::Justification::centred, false);
    }

    void timerCallback() override
    {
        if (isVisible() && getTelemetry_)
        {
            telemetry_ = getTelemetry_();
            repaint();
        }
    }

    struct StageSprites
    {
        juce::Image backdropTop;
        juce::Image backdropFront;
        juce::Image b3Top;
        juce::Image b3Front;
        juce::Image leslie122Top;
        juce::Image leslie122Front;
        juce::Image leslie147Top;
        juce::Image leslie147Front;
        juce::Image marshallTop;
        juce::Image marshallFront;
        std::array<juce::Image, 4> micsTop;
        std::array<juce::Image, 4> micsFront;
    };

    static juce::Image loadBinaryPng([[maybe_unused]] const char* resourceName)
    {
#if defined(SFA_HAS_STAGE_PNG_ASSETS) && SFA_HAS_STAGE_PNG_ASSETS
        int size = 0;
        if (const char* data = BinaryData::getNamedResource(resourceName, size); data != nullptr && size > 0)
            return juce::ImageCache::getFromMemory(data, size);
#endif
        return {};
    }

    void loadStageSprites()
    {
        auto loadFirst = [](const char* primary, const char* fallback) -> juce::Image
        {
            auto img = loadBinaryPng(primary);
            return img.isValid() ? img : loadBinaryPng(fallback);
        };

        sprites_.backdropTop    = loadFirst("stage_maple_topdown_png", "stage_backdrop_top_png");
        sprites_.backdropFront  = loadFirst("stage_maple_front_png",   "stage_backdrop_front_png");
        sprites_.b3Top          = loadBinaryPng("b3_console_top_png");
        sprites_.b3Front        = loadBinaryPng("b3_console_front_png");
        sprites_.leslie122Top   = loadBinaryPng("leslie_122_top_png");
        sprites_.leslie122Front = loadBinaryPng("leslie_122_front_png");
        sprites_.leslie147Top   = loadFirst("leslie_147_top_png",   "leslie_122_top_png");
        sprites_.leslie147Front = loadFirst("leslie_147_front_png", "leslie_122_front_png");
        sprites_.marshallTop    = loadFirst("marshall_bluesbreaker_top_png",   "marshall_1962_top_png");
        sprites_.marshallFront  = loadFirst("marshall_bluesbreaker_front_png", "marshall_1962_front_png");

        sprites_.micsTop[0]     = loadFirst("mic_u47_top_png",    "mic_u47_png");
        sprites_.micsTop[1]     = loadFirst("mic_c414_top_png",   "mic_c414_png");
        sprites_.micsTop[2]     = loadFirst("mic_r121_top_png",   "mic_r121_png");
        sprites_.micsTop[3]     = loadFirst("mic_sm57_top_png",   "mic_sm57_png");

        sprites_.micsFront[0]   = loadFirst("mic_u47_front_png",  "mic_u47_png");
        sprites_.micsFront[1]   = loadFirst("mic_c414_front_png", "mic_c414_png");
        sprites_.micsFront[2]   = loadFirst("mic_r121_front_png", "mic_r121_png");
        sprites_.micsFront[3]   = loadFirst("mic_sm57_front_png", "mic_sm57_png");
    }

    StageSprites sprites_;

    juce::AudioProcessorValueTreeState& apvts_;
    TelemetryProvider getTelemetry_;
    DSP::Leslie122::RotorTelemetry telemetry_ {};

    std::atomic<float>* perspectiveParam_ = nullptr;
    std::atomic<float>* linkParam_        = nullptr;
    std::atomic<float>* micLXParam_       = nullptr;
    std::atomic<float>* micLYParam_       = nullptr;
    std::atomic<float>* micRXParam_       = nullptr;
    std::atomic<float>* micRYParam_       = nullptr;
    std::atomic<float>* micSpreadParam_   = nullptr;
    std::atomic<float>* micDistParam_     = nullptr;
    std::atomic<float>* micLTypeParam_    = nullptr;
    std::atomic<float>* micRTypeParam_    = nullptr;
    std::atomic<float>* cabModelParam_    = nullptr;
    std::atomic<float>* marshallOnParam_   = nullptr;
    std::atomic<float>* marshallGainParam_ = nullptr;
    std::atomic<float>* ampSm57XParam_     = nullptr;
    std::atomic<float>* ampSm57YParam_     = nullptr;
    std::atomic<float>* ampC414XParam_     = nullptr;
    std::atomic<float>* ampC414YParam_     = nullptr;

    juce::TextButton topDownBtn_   { "TOP-DOWN VIEW" };
    juce::TextButton stageViewBtn_ { "STAGE VIEW" };
    juce::TextButton linkMicsBtn_  { "LINK L/R MICS" };
    juce::ComboBox   leftMicBox_;
    juce::ComboBox   rightMicBox_;
    juce::ComboBox   cabinetModelBox_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> leftMicAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rightMicAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cabinetModelAtt_;
    std::unique_ptr<juce::ParameterAttachment> spreadAtt_;
    std::unique_ptr<juce::ParameterAttachment> distAtt_;
    std::unique_ptr<juce::ParameterAttachment> linkAtt_;
    std::unique_ptr<juce::ParameterAttachment> perspAtt_;
    std::unique_ptr<juce::ParameterAttachment> marshallOnAtt_;

    // Parallel Marshall 1962 Bluesbreaker 2x12 Controls
    juce::Rectangle<int> marshallStripBounds_;
    std::unique_ptr<TiltRockerSwitch> marshallOnSwitch_;
    juce::Slider marshallDriveSlider_, marshallToneSlider_, marshallMixSlider_;
    juce::Label  marshallDriveLabel_,  marshallToneLabel_,  marshallMixLabel_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> marshallDriveAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> marshallToneAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> marshallMixAtt_;

    juce::Image backdropCache_;
    int draggedMic_ = 0; // 0 = none, 1 = Leslie L, 2 = Leslie R, 3 = Bluesbreaker SM57, 4 = Bluesbreaker C414
    double lastLeslieDragMs_ = 0.0;
};

} // namespace GreenOnions::UI
