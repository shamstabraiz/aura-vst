#include "PluginProcessor.h"
#include "PluginEditor.h"

AuraCompressorAudioProcessor::AuraCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    thresholdParam      = apvts.getRawParameterValue("threshold");
    ratioParam          = apvts.getRawParameterValue("ratio");
    attackParam         = apvts.getRawParameterValue("attack");
    releaseParam        = apvts.getRawParameterValue("release");
    kneeParam           = apvts.getRawParameterValue("knee");
    makeupParam         = apvts.getRawParameterValue("makeup");
    mixParam            = apvts.getRawParameterValue("mix");
    sidechainHpfParam   = apvts.getRawParameterValue("sidechain_hpf");
    detectionModeParam  = apvts.getRawParameterValue("detection_mode");
    bypassParam         = apvts.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout AuraCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Threshold: -60 dB to 0 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::threshold,
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Ratio: 1:1 to 20:1
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::ratio,
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
        4.0f,
        juce::AudioParameterFloatAttributes().withLabel(":1")
    ));

    // Attack: 0.1 ms to 200 ms (skewed for fine control in fast range)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::attack,
        "Attack",
        juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.35f),
        15.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Release: 10 ms to 1000 ms
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::release,
        "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.4f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Knee: 0 dB to 24 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::knee,
        "Knee",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
        4.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Makeup Gain: -12 dB to +24 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::makeup,
        "Makeup",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Mix (Dry / Wet): 0% to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::mix,
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Sidechain HPF: 20 Hz to 500 Hz
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::sidechainHpf,
        "SC HPF",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.35f),
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // Detection Mode: Peak / RMS
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterID::detectionMode,
        "Detector",
        juce::StringArray { "Peak", "RMS" },
        0
    ));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParameterID::bypass,
        "Bypass",
        false
    ));

    return { params.begin(), params.end() };
}

void AuraCompressorAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    compressorDSP.prepare(sampleRate);
    updateParameters();
}

void AuraCompressorAudioProcessor::releaseResources()
{
    compressorDSP.reset();
}

bool AuraCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AuraCompressorAudioProcessor::updateParameters()
{
    AuraCompressor::CompressorParameters params;
    params.thresholdDb   = thresholdParam      ? thresholdParam->load(std::memory_order_relaxed) : -18.0f;
    params.ratio         = ratioParam          ? ratioParam->load(std::memory_order_relaxed) : 4.0f;
    params.attackMs      = attackParam         ? attackParam->load(std::memory_order_relaxed) : 15.0f;
    params.releaseMs     = releaseParam        ? releaseParam->load(std::memory_order_relaxed) : 100.0f;
    params.kneeDb        = kneeParam           ? kneeParam->load(std::memory_order_relaxed) : 4.0f;
    params.makeupDb      = makeupParam         ? makeupParam->load(std::memory_order_relaxed) : 0.0f;
    params.mixPercent    = mixParam            ? mixParam->load(std::memory_order_relaxed) : 100.0f;
    params.sidechainHpfHz= sidechainHpfParam   ? sidechainHpfParam->load(std::memory_order_relaxed) : 20.0f;

    const int modeIdx    = detectionModeParam  ? static_cast<int>(detectionModeParam->load(std::memory_order_relaxed)) : 0;
    params.detectionMode = (modeIdx == 1) ? AuraCompressor::DetectionMode::RMS : AuraCompressor::DetectionMode::Peak;

    params.bypassed      = bypassParam         ? (bypassParam->load(std::memory_order_relaxed) > 0.5f) : false;

    compressorDSP.setParameters(params);
}

void AuraCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples == 0 || totalNumInputChannels == 0) return;

    updateParameters();

    compressorDSP.processBlock(buffer.getArrayOfWritePointers(), totalNumInputChannels, numSamples);
}

juce::AudioProcessorEditor* AuraCompressorAudioProcessor::createEditor()
{
    return new AuraCompressorAudioProcessorEditor(*this);
}

void AuraCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AuraCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuraCompressorAudioProcessor();
}
