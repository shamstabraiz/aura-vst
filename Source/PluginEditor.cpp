#include "PluginEditor.h"

AuraCompressorAudioProcessorEditor::AuraCompressorAudioProcessorEditor(AuraCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);

    // Header Title
    titleLabel.setText("AURA COMPRESSOR", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::textLight);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("PRECISION DYNAMIC PROCESSOR", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(9.5f)));
    subtitleLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::cyanAccent);
    addAndMakeVisible(subtitleLabel);

    // Detector Mode (Peak / RMS)
    detectorLabel.setText("MODE", juce::dontSendNotification);
    detectorLabel.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    detectorLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::textDim);
    addAndMakeVisible(detectorLabel);

    detectionModeBox.addItem("Peak", 1);
    detectionModeBox.addItem("RMS", 2);
    detectionModeBox.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(detectionModeBox);
    detectionModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "detection_mode", detectionModeBox);

    // Bypass Toggle
    bypassButton.setButtonText("BYPASS");
    bypassButton.setName("bypass");
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    // Visualizers
    addAndMakeVisible(transferCurve);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(inMeter);
    addAndMakeVisible(outMeter);

    grMeterLabel.setText("GR", juce::dontSendNotification);
    grMeterLabel.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    grMeterLabel.setJustificationType(juce::Justification::centred);
    grMeterLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::orangeAccent);
    addAndMakeVisible(grMeterLabel);

    inMeterLabel.setText("IN", juce::dontSendNotification);
    inMeterLabel.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    inMeterLabel.setJustificationType(juce::Justification::centred);
    inMeterLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::textDim);
    addAndMakeVisible(inMeterLabel);

    outMeterLabel.setText("OUT", juce::dontSendNotification);
    outMeterLabel.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    outMeterLabel.setJustificationType(juce::Justification::centred);
    outMeterLabel.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::textDim);
    addAndMakeVisible(outMeterLabel);

    grValueReadout.setText("0.0 dB", juce::dontSendNotification);
    grValueReadout.setFont(juce::Font(juce::FontOptions(11.0f)));
    grValueReadout.setJustificationType(juce::Justification::centred);
    grValueReadout.setColour(juce::Label::textColourId, AuraCompressor::CustomLookAndFeel::orangeAccent);
    addAndMakeVisible(grValueReadout);

    // Knobs
    auto& apvts = audioProcessor.getAPVTS();

    thresholdKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "THRESHOLD", apvts, "threshold", "dB", AuraCompressor::CustomLookAndFeel::cyanAccent);
    addAndMakeVisible(*thresholdKnob);

    ratioKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "RATIO", apvts, "ratio", ":1", AuraCompressor::CustomLookAndFeel::purpleAccent);
    addAndMakeVisible(*ratioKnob);

    kneeKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "KNEE", apvts, "knee", "dB", AuraCompressor::CustomLookAndFeel::purpleAccent);
    addAndMakeVisible(*kneeKnob);

    attackKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "ATTACK", apvts, "attack", "ms", AuraCompressor::CustomLookAndFeel::cyanAccent);
    addAndMakeVisible(*attackKnob);

    releaseKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "RELEASE", apvts, "release", "ms", AuraCompressor::CustomLookAndFeel::orangeAccent);
    addAndMakeVisible(*releaseKnob);

    sidechainHpfKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "SC HPF", apvts, "sidechain_hpf", "Hz", AuraCompressor::CustomLookAndFeel::orangeAccent);
    addAndMakeVisible(*sidechainHpfKnob);

    makeupKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "MAKEUP", apvts, "makeup", "dB", AuraCompressor::CustomLookAndFeel::greenAccent);
    addAndMakeVisible(*makeupKnob);

    mixKnob = std::make_unique<AuraCompressor::RotaryKnob>(
        "MIX", apvts, "mix", "%", AuraCompressor::CustomLookAndFeel::greenAccent);
    addAndMakeVisible(*mixKnob);

    setSize(760, 500);
    startTimerHz(30);
}

AuraCompressorAudioProcessorEditor::~AuraCompressorAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void AuraCompressorAudioProcessorEditor::timerCallback()
{
    auto& dsp = audioProcessor.getDSP();
    const float gr = dsp.getGainReductionDb();
    const float inLevel = dsp.getInputLevelDb();
    const float outLevel = dsp.getOutputLevelDb();

    grMeter.setLevel(gr);
    inMeter.setLevel(inLevel);
    outMeter.setLevel(outLevel);

    grValueReadout.setText("-" + juce::String(gr, 1) + " dB", juce::dontSendNotification);

    auto& apvts = audioProcessor.getAPVTS();
    const float thresh = *apvts.getRawParameterValue("threshold");
    const float ratio  = *apvts.getRawParameterValue("ratio");
    const float knee   = *apvts.getRawParameterValue("knee");

    transferCurve.updateCurve(thresh, ratio, knee, inLevel, outLevel);
}

void AuraCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Subtle dark gradient background
    juce::ColourGradient bgGrad(AuraCompressor::CustomLookAndFeel::bgDark, 0.0f, 0.0f,
                                AuraCompressor::CustomLookAndFeel::bgPanel, 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // Decorative subtle top border accent
    g.setColour(AuraCompressor::CustomLookAndFeel::cyanAccent.withAlpha(0.6f));
    g.fillRect(0, 0, getWidth(), 2);

    // Section bounding cards
    // 1. Visualizer Card
    auto visCard = juce::Rectangle<int>(20, 70, 720, 220);
    g.setColour(AuraCompressor::CustomLookAndFeel::bgPanel.withAlpha(0.5f));
    g.fillRoundedRectangle(visCard.toFloat(), 6.0f);
    g.setColour(AuraCompressor::CustomLookAndFeel::bgPanelBorder.withAlpha(0.5f));
    g.drawRoundedRectangle(visCard.toFloat(), 6.0f, 1.0f);

    // 2. Lower Controls Card
    auto ctrlCard = juce::Rectangle<int>(20, 305, 720, 175);
    g.setColour(AuraCompressor::CustomLookAndFeel::bgPanel.withAlpha(0.5f));
    g.fillRoundedRectangle(ctrlCard.toFloat(), 6.0f);
    g.setColour(AuraCompressor::CustomLookAndFeel::bgPanelBorder.withAlpha(0.5f));
    g.drawRoundedRectangle(ctrlCard.toFloat(), 6.0f, 1.0f);

    // Section group dividers & headings
    g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    g.setColour(AuraCompressor::CustomLookAndFeel::textDim);

    g.drawText("PRIMARY DYNAMICS", 30, 310, 150, 14, juce::Justification::left);
    g.drawText("TIME CONSTANTS", 290, 310, 150, 14, juce::Justification::left);
    g.drawText("OUTPUT STAGE", 540, 310, 150, 14, juce::Justification::left);

    g.setColour(AuraCompressor::CustomLookAndFeel::bgPanelBorder.withAlpha(0.4f));
    g.drawVerticalLine(275, 316.0f, 470.0f);
    g.drawVerticalLine(525, 316.0f, 470.0f);
}

void AuraCompressorAudioProcessorEditor::resized()
{
    // Header layout
    titleLabel.setBounds(24, 14, 220, 24);
    subtitleLabel.setBounds(25, 38, 220, 14);

    bypassButton.setBounds(630, 18, 105, 26);

    detectorLabel.setBounds(465, 22, 40, 18);
    detectionModeBox.setBounds(510, 20, 85, 22);

    // Visualizers layout
    // Transfer curve takes left side of the visualizer card
    transferCurve.setBounds(30, 80, 480, 200);

    // Meters take right side
    const int meterY = 85;
    const int meterH = 170;

    inMeterLabel.setBounds(530, 260, 24, 16);
    inMeter.setBounds(532, meterY, 20, meterH);

    grMeterLabel.setBounds(585, 260, 40, 16);
    grValueReadout.setBounds(575, 276, 60, 14);
    grMeter.setBounds(590, meterY, 30, meterH);

    outMeterLabel.setBounds(655, 260, 30, 16);
    outMeter.setBounds(660, meterY, 20, meterH);

    // Knobs layout in the lower control card
    const int knobY = 335;
    const int knobW = 75;
    const int knobH = 135;

    // Primary dynamics (Threshold, Ratio, Knee)
    thresholdKnob->setBounds(35,  knobY, knobW, knobH);
    ratioKnob->setBounds(115,     knobY, knobW, knobH);
    kneeKnob->setBounds(195,      knobY, knobW, knobH);

    // Time constants (Attack, Release, SC HPF)
    attackKnob->setBounds(285,    knobY, knobW, knobH);
    releaseKnob->setBounds(365,   knobY, knobW, knobH);
    sidechainHpfKnob->setBounds(445, knobY, knobW, knobH);

    // Output stage (Makeup, Mix)
    makeupKnob->setBounds(545,    knobY, knobW, knobH);
    mixKnob->setBounds(635,       knobY, knobW, knobH);
}
