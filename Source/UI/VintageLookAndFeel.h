#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <numbers>
#include <random>

namespace GreenOnions::UI
{

namespace Palette
{
    inline const juce::Colour walnutDark   { 0xff1b100a };
    inline const juce::Colour walnutMid    { 0xff3b2113 };
    inline const juce::Colour walnutLight  { 0xff5e3620 };
    inline const juce::Colour panelBg      { 0xff141518 };
    inline const juce::Colour panelSection { 0xff1d1e22 };
    inline const juce::Colour panelBorder  { 0xff3d352c };
    inline const juce::Colour cream        { 0xfff0e4ce };
    inline const juce::Colour creamDim     { 0xffa89c87 };
    inline const juce::Colour amber        { 0xffd99b4e };
    inline const juce::Colour amberGlow    { 0xffffcf87 };
    inline const juce::Colour greenAccent  { 0xff69f0ae };
    inline const juce::Colour drawbarBrown { 0xff6e3b20 };
    inline const juce::Colour drawbarIvory { 0xffeae2d0 };
    inline const juce::Colour drawbarBlack { 0xff242220 };
}

class VintageLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel()
    {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141518));
        setColour(juce::ComboBox::textColourId,       juce::Colour(0xfff0e4ce));
        setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff6b5335));
        setColour(juce::ComboBox::arrowColourId,      juce::Colour(0xffd99b4e));

        setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(0xff18191d));
        setColour(juce::PopupMenu::textColourId,                  juce::Colour(0xfff0e4ce));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff4a351e));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(0xffffcf87));

        setColour(juce::Label::textColourId, Palette::cream);
    }

    //==========================================================================
    // Cached Procedural American Black Walnut Wood Grain Texture
    //==========================================================================
    const juce::Image& getWalnutTexture(int width, int height)
    {
        width  = juce::jmax(1, width);
        height = juce::jmax(1, height);

        if (walnutCache_.isValid() && walnutCache_.getWidth() == width && walnutCache_.getHeight() == height)
            return walnutCache_;

        walnutCache_ = juce::Image(juce::Image::RGB, width, height, false);
        juce::Image::BitmapData pixels(walnutCache_, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < height; ++y)
        {
            const float ny = static_cast<float>(y) / static_cast<float>(height);
            const float edgeShade = 0.78f + 0.22f * std::sin(ny * juce::MathConstants<float>::pi);

            for (int x = 0; x < width; ++x)
            {
                const float nx = static_cast<float>(x) * 0.018f;
                const float wave = std::sin(ny * 38.0f + std::sin(nx * 2.3f) * 3.2f + std::cos(nx * 0.7f) * 1.8f);
                const float fineGrain = std::sin(ny * 145.0f + nx * 5.0f) * 0.25f;
                const float cathedral = juce::jlimit(0.0f, 1.0f, 0.5f + 0.38f * wave + 0.12f * fineGrain);

                // Rich oiled American walnut palette (#1b100a -> #4c2d1b)
                const auto r = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, (28.0f + cathedral * 52.0f) * edgeShade));
                const auto g = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, (16.0f + cathedral * 31.0f) * edgeShade));
                const auto b = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, (10.0f + cathedral * 18.0f) * edgeShade));

                pixels.setPixelColour(x, y, juce::Colour(r, g, b));
            }
        }
        return walnutCache_;
    }

    //==========================================================================
    // Cached Procedural Brushed Dark Anodized Aluminum Texture
    //==========================================================================
    const juce::Image& getAnodizedMetalTexture(int width, int height)
    {
        width  = juce::jmax(1, width);
        height = juce::jmax(1, height);

        if (metalCache_.isValid() && metalCache_.getWidth() == width && metalCache_.getHeight() == height)
            return metalCache_;

        metalCache_ = juce::Image(juce::Image::RGB, width, height, false);
        juce::Image::BitmapData pixels(metalCache_, juce::Image::BitmapData::writeOnly);

        std::mt19937 rng(73122u);
        std::uniform_real_distribution<float> streakDist(-1.0f, 1.0f);

        for (int y = 0; y < height; ++y)
        {
            const float rowStreak = streakDist(rng) * 3.2f;
            const float topSheen  = 1.0f - (static_cast<float>(y) / static_cast<float>(height)) * 0.16f;

            for (int x = 0; x < width; ++x)
            {
                const float micro = std::sin(static_cast<float>(x) * 0.045f + static_cast<float>(y) * 1.7f) * 1.4f;
                const float base  = (29.0f + rowStreak + micro) * topSheen;

                const auto r = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, base));
                const auto g = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, base + 1.0f));
                const auto b = static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, base + 4.0f));

                pixels.setPixelColour(x, y, juce::Colour(r, g, b));
            }
        }
        return metalCache_;
    }

    // Helper for crisp dark anodized sub-panel frames
    static void drawAnodizedPanel(juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius = 5.0f)
    {
        juce::ColourGradient metalGrad(juce::Colour(0xff232428).withAlpha(0.88f), r.getX(), r.getY(),
                                       juce::Colour(0xff16171a).withAlpha(0.92f), r.getX(), r.getBottom(), false);
        g.setGradientFill(metalGrad);
        g.fillRoundedRectangle(r, cornerRadius);

        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.drawHorizontalLine(static_cast<int>(r.getY() + 1.0f), r.getX() + cornerRadius, r.getRight() - cornerRadius);
        g.setColour(Palette::panelBorder);
        g.drawRoundedRectangle(r, cornerRadius, 1.0f);
    }

    //==========================================================================
    // Signal Forge Fluted Bakelite Skirted Rotary Knob
    //==========================================================================
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                   static_cast<float>(width), static_cast<float>(height)).reduced(3.0f);
        const auto center  = bounds.getCentre();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float angle  = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // 1. Graduated Radial Scale Ticks (11 ticks around travel arc)
        const float tickOuterR = radius;
        const float tickInnerR = radius - 4.0f;
        for (int t = 0; t <= 10; ++t)
        {
            const float frac       = static_cast<float>(t) / 10.0f;
            const float tickAng    = rotaryStartAngle + frac * (rotaryEndAngle - rotaryStartAngle);
            const bool activeTick  = (frac <= sliderPosProportional + 0.01f);

            const float sx = center.x + std::sin(tickAng) * tickInnerR;
            const float sy = center.y - std::cos(tickAng) * tickInnerR;
            const float ex = center.x + std::sin(tickAng) * tickOuterR;
            const float ey = center.y - std::cos(tickAng) * tickOuterR;

            g.setColour(activeTick ? juce::Colour(0xffffb74d) : juce::Colour(0xff5e574c));
            g.drawLine(sx, sy, ex, ey, (t == 0 || t == 5 || t == 10) ? 1.6f : 1.0f);
        }

        // 2. Soft Directional Drop Shadow
        const float skirtR = juce::jmax(6.0f, radius - 5.5f);
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(center.x - skirtR + 1.5f, center.y - skirtR + 2.5f, skirtR * 2.0f, skirtR * 2.0f);

        // 3. Machined Aluminum Skirt Base
        juce::ColourGradient skirtGrad(juce::Colour(0xff9da4b0), center.x, center.y - skirtR,
                                       juce::Colour(0xff3b3f47), center.x, center.y + skirtR, false);
        g.setGradientFill(skirtGrad);
        g.fillEllipse(center.x - skirtR, center.y - skirtR, skirtR * 2.0f, skirtR * 2.0f);

        g.setColour(juce::Colour(0xff141518));
        g.drawEllipse(center.x - skirtR, center.y - skirtR, skirtR * 2.0f, skirtR * 2.0f, 1.0f);

        // 4. Fluted Bakelite Crown Cap (12 scalloped grip flutes)
        const float capR = skirtR * 0.76f;
        juce::Path flutedCap;
        constexpr int kFlutes   = 12;
        constexpr int kSegments = 72;
        for (int i = 0; i <= kSegments; ++i)
        {
            const float theta   = angle + (static_cast<float>(i) / static_cast<float>(kSegments)) * juce::MathConstants<float>::twoPi;
            const float scallop = 1.0f - 0.055f * (0.5f + 0.5f * std::cos(static_cast<float>(kFlutes) * (theta - angle)));
            const float r       = capR * scallop;
            const float px      = center.x + std::sin(theta) * r;
            const float py      = center.y - std::cos(theta) * r;

            if (i == 0) flutedCap.startNewSubPath(px, py);
            else        flutedCap.lineTo(px, py);
        }
        flutedCap.closeSubPath();

        juce::ColourGradient capGrad(juce::Colour(0xff383634), center.x, center.y - capR,
                                     juce::Colour(0xff141312), center.x, center.y + capR, false);
        g.setGradientFill(capGrad);
        g.fillPath(flutedCap);

        g.setColour(juce::Colours::white.withAlpha(0.14f));
        g.strokePath(flutedCap, juce::PathStrokeType(0.9f));

        // 5. Brushed Brass Center Medallion
        const float medalR = capR * 0.46f;
        juce::ColourGradient brassGrad(juce::Colour(0xffe6b873), center.x, center.y - medalR,
                                       juce::Colour(0xff7a5526), center.x, center.y + medalR, false);
        g.setGradientFill(brassGrad);
        g.fillEllipse(center.x - medalR, center.y - medalR, medalR * 2.0f, medalR * 2.0f);

        // 6. Engraved Pointer Line Across Skirt & Cap
        const float ptrStart = medalR * 0.65f;
        const float ptrEnd   = skirtR - 1.2f;
        const float px1      = center.x + std::sin(angle) * ptrStart;
        const float py1      = center.y - std::cos(angle) * ptrStart;
        const float px2      = center.x + std::sin(angle) * ptrEnd;
        const float py2      = center.y - std::cos(angle) * ptrEnd;

        g.setColour(juce::Colour(0xffffcf87));
        g.drawLine(px1, py1, px2, py2, 2.2f);
    }

    //==========================================================================
    // Brass-Trimmed Vintage Selector Boxes & Buttons
    //==========================================================================
    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.2f);

        juce::Path arrow;
        const float cx = static_cast<float>(buttonX) + static_cast<float>(buttonW) * 0.5f;
        const float cy = static_cast<float>(buttonY) + static_cast<float>(buttonH) * 0.5f;
        arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
        arrow.lineTo(cx, cy + 3.0f);
        arrow.lineTo(cx + 4.0f, cy - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.strokePath(arrow, juce::PathStrokeType(1.6f));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        const auto bounds = g.getClipBounds().toFloat().reduced(0.5f);
        auto base = backgroundColour;
        if (shouldDrawButtonAsDown)             base = base.darker(0.25f);
        else if (shouldDrawButtonAsHighlighted) base = base.brighter(0.15f);

        g.setColour(base);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colour(0xff8c683a));
        g.drawRoundedRectangle(bounds, 4.0f, 1.1f);
    }

private:
    juce::Image walnutCache_;
    juce::Image metalCache_;
};

} // namespace GreenOnions::UI
