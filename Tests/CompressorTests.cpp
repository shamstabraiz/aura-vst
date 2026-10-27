#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include "../Source/Audio/CompressorDSP.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Aura Compressor DSP Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    const double sampleRate = 48000.0;
    const int numSamples = 4800; // 100ms block

    AuraCompressor::CompressorDSP dsp;
    dsp.prepare(sampleRate);

    // 1. Test Below-Threshold Signal (No Gain Reduction)
    {
        AuraCompressor::CompressorParameters params;
        params.thresholdDb = -12.0f;
        params.ratio = 4.0f;
        params.kneeDb = 0.0f;
        params.makeupDb = 0.0f;
        params.mixPercent = 100.0f;
        dsp.setParameters(params);

        std::vector<float> left(numSamples, 0.1f); // 0.1 = -20 dB (below -12 dB)
        std::vector<float> right(numSamples, 0.1f);
        float* channels[2] = { left.data(), right.data() };

        dsp.processBlock(channels, 2, numSamples);

        const float gr = dsp.getGainReductionDb();
        std::cout << "[PASS] Below Threshold (-20 dB in, -12 dB thresh): GR = " << gr << " dB (expected 0 dB)" << std::endl;
        assert(gr < 0.1f);
    }

    // 2. Test Above-Threshold Signal (Compression Applied)
    {
        AuraCompressor::CompressorParameters params;
        params.thresholdDb = -20.0f;
        params.ratio = 4.0f;
        params.kneeDb = 0.0f;
        params.attackMs = 1.0f;  // fast attack
        params.releaseMs = 50.0f;
        params.makeupDb = 0.0f;
        params.mixPercent = 100.0f;
        dsp.setParameters(params);

        // Input 1.0 = 0 dBFS (20 dB above threshold)
        // With 4:1 ratio, 20 dB above thresh -> 5 dB above thresh -> 15 dB reduction
        std::vector<float> left(numSamples, 1.0f);
        std::vector<float> right(numSamples, 1.0f);
        float* channels[2] = { left.data(), right.data() };

        // Run multiple blocks to let attack envelope settle
        for (int b = 0; b < 10; ++b) {
            std::fill(left.begin(), left.end(), 1.0f);
            std::fill(right.begin(), right.end(), 1.0f);
            dsp.processBlock(channels, 2, numSamples);
        }

        const float gr = dsp.getGainReductionDb();
        std::cout << "[PASS] Above Threshold (0 dB in, -20 dB thresh, 4:1 ratio): GR = " << gr 
                  << " dB (expected ~15 dB reduction)" << std::endl;
        assert(gr > 14.0f && gr < 16.0f);
    }

    // 3. Test Soft-Knee Mathematical Continuity
    {
        const float thresh = -20.0f;
        const float ratio = 4.0f;
        const float knee = 10.0f; // knee extends from -25 dB to -15 dB

        // Exactly at lower knee edge
        float yLower = AuraCompressor::CompressorDSP::evaluateStaticCurve(-25.0f, thresh, ratio, knee);
        // At threshold midpoint
        float yThresh = AuraCompressor::CompressorDSP::evaluateStaticCurve(-20.0f, thresh, ratio, knee);
        // Above knee
        float yAbove = AuraCompressor::CompressorDSP::evaluateStaticCurve(-10.0f, thresh, ratio, knee);

        std::cout << "[PASS] Soft-Knee Check:" << std::endl;
        std::cout << "       in = -25 dB -> out = " << yLower << " dB (expected -25 dB)" << std::endl;
        std::cout << "       in = -20 dB -> out = " << yThresh << " dB (expected smooth transition)" << std::endl;
        std::cout << "       in = -10 dB -> out = " << yAbove << " dB (expected -17.5 dB)" << std::endl;

        assert(std::abs(yLower - (-25.0f)) < 0.01f);
        assert(yThresh < -20.0f); // some reduction active in knee
        assert(std::abs(yAbove - (-17.5f)) < 0.01f);
    }

    // 4. Test Dry/Wet Parallel Compression
    {
        AuraCompressor::CompressorParameters params;
        params.thresholdDb = -20.0f;
        params.ratio = 20.0f; // brickwall squash
        params.attackMs = 0.5f;
        params.releaseMs = 50.0f;
        params.makeupDb = 0.0f;
        params.mixPercent = 0.0f; // 100% Dry
        dsp.setParameters(params);

        std::vector<float> left(numSamples, 0.8f);
        std::vector<float> right(numSamples, 0.8f);
        float* channels[2] = { left.data(), right.data() };
        dsp.processBlock(channels, 2, numSamples);

        // When Mix is 0%, output should remain exactly identical to input
        std::cout << "[PASS] 100% Dry Parallel Mix: input 0.8 -> output " << left[numSamples - 1] << std::endl;
        assert(std::abs(left[numSamples - 1] - 0.8f) < 0.001f);
    }

    // 5. Test Sidechain High-Pass Filter
    {
        AuraCompressor::SidechainFilter filter;
        filter.prepare(sampleRate);
        filter.setCutoff(200.0f); // 200 Hz cutoff

        // 50 Hz sub-bass should be attenuated
        float outL = 0.0f, outR = 0.0f;
        float peakSub = 0.0f;
        for (int i = 0; i < 2000; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            const float sub = std::sin(2.0f * static_cast<float>(M_PI) * 50.0f * t);
            filter.processSample(sub, sub, outL, outR);
            if (i > 1000) peakSub = std::max(peakSub, std::abs(outL));
        }

        std::cout << "[PASS] Sidechain HPF (200 Hz): 50 Hz sub-bass attenuated to amplitude " << peakSub 
                  << " (from 1.0)" << std::endl;
        assert(peakSub < 0.25f);
    }

    std::cout << "\n>>> ALL 5 DSP TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
