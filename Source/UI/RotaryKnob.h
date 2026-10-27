#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CustomLookAndFeel.h"

namespace AuraCompressor {

class RotaryKnob : public juce::Component {
public:
    RotaryKnob(const juce::String& labelText,
               juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramID,
               const juce::String& unitSuffix = "",
               juce::Colour accentColour = CustomLookAndFeel::cyanAccent)
        : suffix(unitSuffix), accent(accentColour)
    {
        titleLabel.setText(labelText, juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        titleLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textDim);
        addAndMakeVisible(titleLabel);

        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.getProperties().set("accentColor", accent.toString());
        slider.setDoubleClickReturnValue(true, 0.0);
        slider.onValueChange = [this]() { updateValueLabel(); };
        addAndMakeVisible(slider);

        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
        valueLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textLight);
        addAndMakeVisible(valueLabel);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
        updateValueLabel();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        titleLabel.setBounds(bounds.removeFromTop(18));
        valueLabel.setBounds(bounds.removeFromBottom(18));
        slider.setBounds(bounds.reduced(2));
    }

    void setAccentColour(juce::Colour newAccent)
    {
        accent = newAccent;
        slider.getProperties().set("accentColor", accent.toString());
        slider.repaint();
    }

    juce::Slider& getSlider() { return slider; }

private:
    void updateValueLabel()
    {
        const double val = slider.getValue();
        juce::String str;

        if (std::abs(val) < 10.0 && suffix != "%") {
            str = juce::String(val, 1);
        } else {
            str = juce::String(juce::roundToInt(val));
        }

        if (suffix.isNotEmpty()) {
            str += " " + suffix;
        }

        valueLabel.setText(str, juce::dontSendNotification);
    }

    juce::Label titleLabel;
    juce::Label valueLabel;
    juce::Slider slider;
    juce::String suffix;
    juce::Colour accent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

} // namespace AuraCompressor
