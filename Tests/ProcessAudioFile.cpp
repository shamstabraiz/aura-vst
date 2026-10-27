#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include "../Source/Audio/CompressorDSP.h"

// Minimal 16-bit PCM WAV file writer
void writeWav(const std::string& filename, const std::vector<float>& left, const std::vector<float>& right, int sampleRate) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }

    uint32_t numSamples = static_cast<uint32_t>(left.size());
    uint16_t numChannels = 2;
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    uint32_t dataChunkSize = numSamples * numChannels * (bitsPerSample / 8);
    uint32_t totalChunkSize = 36 + dataChunkSize;

    // RIFF Header
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&totalChunkSize), 4);
    file.write("WAVE", 4);

    // fmt subchunk
    file.write("fmt ", 4);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data subchunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataChunkSize), 4);

    for (size_t i = 0; i < numSamples; ++i) {
        float lClamped = std::max(-1.0f, std::min(1.0f, left[i]));
        float rClamped = std::max(-1.0f, std::min(1.0f, right[i]));
        int16_t sampleL = static_cast<int16_t>(lClamped * 32767.0f);
        int16_t sampleR = static_cast<int16_t>(rClamped * 32767.0f);
        file.write(reinterpret_cast<const char*>(&sampleL), 2);
        file.write(reinterpret_cast<const char*>(&sampleR), 2);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================\n";
    std::cout << "   Aura Compressor Audio Demonstration & Test Utility   \n";
    std::cout << "=========================================================\n\n";

    const int sampleRate = 48000;
    const float durationSec = 4.0f;
    const int numSamples = static_cast<int>(sampleRate * durationSec);

    std::vector<float> inputL(numSamples, 0.0f);
    std::vector<float> inputR(numSamples, 0.0f);

    // Generate dynamic audio: rhythmic bursts simulating drums/transients
    // Section 1: Loud transient pulse every 0.5s (0.95 amplitude ~ -0.5 dBFS)
    // Section 2: Quiet sustained chords (0.15 amplitude ~ -16 dBFS)
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Base tone (440 Hz chord)
        float quietTone = 0.15f * std::sin(2.0f * M_PI * 440.0f * t)
                        + 0.10f * std::sin(2.0f * M_PI * 554.37f * t);

        // Periodic loud drums/bursts every 0.5 seconds
        float beatPhase = std::fmod(t, 0.5f);
        float kickTransient = 0.0f;
        if (beatPhase < 0.15f) {
            float env = std::exp(-beatPhase * 35.0f); // fast decaying kick pulse
            kickTransient = 0.85f * env * std::sin(2.0f * M_PI * 80.0f * beatPhase);
        }

        float sample = quietTone + kickTransient;
        inputL[i] = sample;
        inputR[i] = sample;
    }

    // Configure Compressor
    AuraCompressor::CompressorDSP comp;
    comp.prepare(sampleRate);

    AuraCompressor::CompressorParameters params;
    params.thresholdDb = -18.0f;
    params.ratio = 4.0f;
    params.attackMs = 10.0f;
    params.releaseMs = 80.0f;
    params.kneeDb = 6.0f;
    params.makeupDb = 4.0f; // 4 dB makeup
    params.mixPercent = 100.0f;
    params.sidechainHpfHz = 20.0f;
    params.detectionMode = AuraCompressor::DetectionMode::Peak;
    params.bypassed = false;
    comp.setParameters(params);

    std::vector<float> outputL = inputL;
    std::vector<float> outputR = inputR;
    float* channels[2] = { outputL.data(), outputR.data() };

    // Process in realistic 256-sample blocks
    const int blockSize = 256;
    float maxGrSeen = 0.0f;
    for (int i = 0; i < numSamples; i += blockSize) {
        int samplesToProcess = std::min(blockSize, numSamples - i);
        float* blockChannels[2] = { outputL.data() + i, outputR.data() + i };
        comp.processBlock(blockChannels, 2, samplesToProcess);
        maxGrSeen = std::max(maxGrSeen, comp.getGainReductionDb());
    }

    // Calculate Peak & RMS stats
    auto getStats = [](const std::vector<float>& buf) {
        float peak = 0.0f;
        double sumSq = 0.0;
        for (float s : buf) {
            float a = std::abs(s);
            if (a > peak) peak = a;
            sumSq += s * s;
        }
        float rms = static_cast<float>(std::sqrt(sumSq / buf.size()));
        float peakDb = peak > 1e-5f ? 20.0f * std::log10(peak) : -100.0f;
        float rmsDb = rms > 1e-5f ? 20.0f * std::log10(rms) : -100.0f;
        return std::make_pair(peakDb, rmsDb);
    };

    auto inStats = getStats(inputL);
    auto outStats = getStats(outputL);

    writeWav("Tests/input_uncompressed.wav", inputL, inputR, sampleRate);
    writeWav("Tests/output_compressed.wav", outputL, outputR, sampleRate);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Compressor Settings:\n";
    std::cout << "  - Threshold:   " << params.thresholdDb << " dB\n";
    std::cout << "  - Ratio:       " << params.ratio << ":1\n";
    std::cout << "  - Attack:      " << params.attackMs << " ms\n";
    std::cout << "  - Release:     " << params.releaseMs << " ms\n";
    std::cout << "  - Knee:        " << params.kneeDb << " dB\n";
    std::cout << "  - Makeup:     +" << params.makeupDb << " dB\n\n";

    std::cout << "Audio Measurements:\n";
    std::cout << "  Input Signal:   Peak = " << inStats.first << " dBFS, RMS = " << inStats.second << " dBFS\n";
    std::cout << "  Output Signal:  Peak = " << outStats.first << " dBFS, RMS = " << outStats.second << " dBFS\n";
    std::cout << "  Max Reduction:  " << maxGrSeen << " dB\n";
    std::cout << "  Dynamic Crest:  Reduced by " << (inStats.first - inStats.second) - (outStats.first - outStats.second) << " dB (more uniform energy!)\n\n";

    std::cout << "WAV Audio Files Generated:\n";
    std::cout << "  1. Tests/input_uncompressed.wav\n";
    std::cout << "  2. Tests/output_compressed.wav\n\n";
    std::cout << "To audition the audio on Linux, run:\n";
    std::cout << "  aplay Tests/output_compressed.wav\n";
    std::cout << "  or:\n";
    std::cout << "  ffplay -nodisp -autoexit Tests/output_compressed.wav\n";
    return 0;
}
