#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Audio/CompressorDSP.h"

namespace ParameterID {
    inline const juce::ParameterID threshold       { "threshold", 1 };
    inline const juce::ParameterID ratio           { "ratio", 1 };
    inline const juce::ParameterID attack          { "attack", 1 };
    inline const juce::ParameterID release         { "release", 1 };
    inline const juce::ParameterID knee            { "knee", 1 };
    inline const juce::ParameterID makeup          { "makeup", 1 };
    inline const juce::ParameterID mix             { "mix", 1 };
    inline const juce::ParameterID sidechainHpf    { "sidechain_hpf", 1 };
    inline const juce::ParameterID detectionMode   { "detection_mode", 1 };
    inline const juce::ParameterID bypass          { "bypass", 1 };
}

class AuraCompressorAudioProcessor : public juce::AudioProcessor {
public:
    AuraCompressorAudioProcessor();
    ~AuraCompressorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Aura Compressor"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    AuraCompressor::CompressorDSP& getDSP() { return compressorDSP; }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    juce::AudioProcessorValueTreeState apvts;
    AuraCompressor::CompressorDSP compressorDSP;

    // Cached atomic parameter pointers for real-time thread safety
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* kneeParam = nullptr;
    std::atomic<float>* makeupParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* sidechainHpfParam = nullptr;
    std::atomic<float>* detectionModeParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuraCompressorAudioProcessor)
};
