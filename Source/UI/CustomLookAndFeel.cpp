#include "CustomLookAndFeel.h"

namespace AuraCompressor {

const juce::Colour CustomLookAndFeel::bgDark        = juce::Colour(0xff0d1117);
const juce::Colour CustomLookAndFeel::bgPanel       = juce::Colour(0xff161b22);
const juce::Colour CustomLookAndFeel::bgPanelBorder = juce::Colour(0xff30363d);
const juce::Colour CustomLookAndFeel::dialDark      = juce::Colour(0xff1a1f26);
const juce::Colour CustomLookAndFeel::dialRim       = juce::Colour(0xff2d333b);
const juce::Colour CustomLookAndFeel::textLight     = juce::Colour(0xfff0f6fc);
const juce::Colour CustomLookAndFeel::textDim       = juce::Colour(0xff8b949e);
const juce::Colour CustomLookAndFeel::cyanAccent    = juce::Colour(0xff00e5ff);
const juce::Colour CustomLookAndFeel::orangeAccent  = juce::Colour(0xffff9100);
const juce::Colour CustomLookAndFeel::purpleAccent  = juce::Colour(0xffa855f7);
const juce::Colour CustomLookAndFeel::greenAccent   = juce::Colour(0xff10b981);
const juce::Colour CustomLookAndFeel::redAccent     = juce::Colour(0xffef4444);

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, textLight);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    setColour(juce::Label::textColourId, textLight);
    setColour(juce::ComboBox::backgroundColourId, dialDark);
    setColour(juce::ComboBox::textColourId, textLight);
    setColour(juce::ComboBox::outlineColourId, bgPanelBorder);
    setColour(juce::ComboBox::arrowColourId, cyanAccent);
    setColour(juce::PopupMenu::backgroundColourId, bgPanel);
    setColour(juce::PopupMenu::textColourId, textLight);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, dialRim);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Accent color from slider component properties or default cyan
    juce::Colour trackAccent = cyanAccent;
    if (slider.getProperties().contains("accentColor")) {
        trackAccent = juce::Colour::fromString(slider.getProperties()["accentColor"].toString());
    }

    // 1. Subtle Outer Glow / Shadow
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(rx + 2.0f, ry + 2.0f, rw, rw);

    // 2. Dial Base (gradient dark rim)
    juce::ColourGradient dialGradient(dialRim, centreX, ry, dialDark, centreX, ry + rw, false);
    g.setGradientFill(dialGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // 3. Inner Face
    const float innerInset = 4.0f;
    g.setColour(dialDark.brighter(0.04f));
    g.fillEllipse(rx + innerInset, ry + innerInset, rw - innerInset * 2.0f, rw - innerInset * 2.0f);

    // 4. Background Track Arc (inactive faint groove)
    const float arcRadius = radius - 7.0f;
    const float arcThickness = 3.5f;

    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(bgPanelBorder.withAlpha(0.6f));
    g.strokePath(bgArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 5. Active Glowing Value Arc
    if (sliderPosProportional > 0.001f) {
        juce::Path valArc;
        valArc.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

        // Glow halo
        g.setColour(trackAccent.withAlpha(0.25f));
        g.strokePath(valArc, juce::PathStrokeType(arcThickness + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Crisp solid arc
        g.setColour(trackAccent);
        g.strokePath(valArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 6. Indicator Dot / Pointer Needle
    juce::Path pointer;
    const float pointerLength = radius * 0.55f;
    const float pointerThickness = 2.5f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 8.0f, pointerThickness, pointerLength * 0.45f, 1.0f);

    g.setColour(textLight);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    // Pointer glowing tip
    juce::Point<float> tipPoint = juce::Point<float>(0.0f, -radius + 10.0f)
                                  .transformedBy(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(trackAccent);
    g.fillEllipse(tipPoint.x - 2.5f, tipPoint.y - 2.5f, 5.0f, 5.0f);
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool state = button.getToggleState();

    const float switchWidth = 38.0f;
    const float switchHeight = 20.0f;
    const float switchX = bounds.getX() + 2.0f;
    const float switchY = bounds.getCentreY() - switchHeight * 0.5f;

    // Track
    auto trackRect = juce::Rectangle<float>(switchX, switchY, switchWidth, switchHeight);
    juce::Colour trackCol = state ? cyanAccent : bgPanelBorder;
    if (button.getName() == "bypass" && state) {
        trackCol = redAccent;
    }

    g.setColour(trackCol.withAlpha(0.35f));
    g.fillRoundedRectangle(trackRect, switchHeight * 0.5f);
    g.setColour(trackCol);
    g.drawRoundedRectangle(trackRect, switchHeight * 0.5f, 1.5f);

    // Thumb / Knob
    const float thumbRadius = 7.0f;
    const float thumbX = state ? (switchX + switchWidth - thumbRadius - 4.0f) : (switchX + 4.0f + thumbRadius);
    const float thumbY = switchY + switchHeight * 0.5f;

    g.setColour(state ? trackCol : textLight);
    g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);

    // Text Label
    g.setColour(state ? textLight : textDim);
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
    auto textRect = bounds.withTrimmedLeft(switchWidth + 8.0f);
    g.drawFittedText(button.getButtonText(), textRect.toNearestInt(), juce::Justification::centredLeft, 1);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                     int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, juce::ComboBox& /*box*/)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

    g.setColour(dialDark);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(bgPanelBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Arrow icon
    const float arrowX = width - 16.0f;
    const float arrowY = height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX - 4.0f, arrowY - 2.0f);
    arrow.lineTo(arrowX, arrowY + 3.0f);
    arrow.lineTo(arrowX + 4.0f, arrowY - 2.0f);

    g.setColour(cyanAccent);
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

} // namespace AuraCompressor
