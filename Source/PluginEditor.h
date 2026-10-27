#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/RotaryKnob.h"
#include "UI/MeterComponent.h"
#include "UI/TransferCurveComponent.h"

class AuraCompressorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit AuraCompressorAudioProcessorEditor(AuraCompressorAudioProcessor&);
    ~AuraCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    AuraCompressorAudioProcessor& audioProcessor;
    AuraCompressor::CustomLookAndFeel customLookAndFeel;

    // Header controls
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::ComboBox detectionModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectionModeAttachment;
    juce::Label detectorLabel;

    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Visualizers
    AuraCompressor::TransferCurveComponent transferCurve;
    AuraCompressor::GainReductionMeter grMeter;
    AuraCompressor::LevelMeter inMeter;
    AuraCompressor::LevelMeter outMeter;
    juce::Label grMeterLabel;
    juce::Label inMeterLabel;
    juce::Label outMeterLabel;
    juce::Label grValueReadout;

    // Rotary Knobs
    std::unique_ptr<AuraCompressor::RotaryKnob> thresholdKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> ratioKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> kneeKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> attackKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> releaseKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> sidechainHpfKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> makeupKnob;
    std::unique_ptr<AuraCompressor::RotaryKnob> mixKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuraCompressorAudioProcessorEditor)
};
