#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace AuraCompressor {

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;

    // Custom color palette
    static const juce::Colour bgDark;
    static const juce::Colour bgPanel;
    static const juce::Colour bgPanelBorder;
    static const juce::Colour dialDark;
    static const juce::Colour dialRim;
    static const juce::Colour textLight;
    static const juce::Colour textDim;
    static const juce::Colour cyanAccent;
    static const juce::Colour orangeAccent;
    static const juce::Colour purpleAccent;
    static const juce::Colour greenAccent;
    static const juce::Colour redAccent;
};

} // namespace AuraCompressor
