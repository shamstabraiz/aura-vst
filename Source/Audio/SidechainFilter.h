#pragma once

#include <cmath>

namespace AuraCompressor {

/**
 * 2nd-order Butterworth High-Pass Filter for detector sidechain.
 * Cleans low rumble / bass below cutoff frequency to prevent pumping.
 */
class SidechainFilter {
public:
    SidechainFilter() = default;

    void prepare(double newSampleRate) {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
        updateCoefficients();
    }

    void reset() {
        x1_L = x2_L = y1_L = y2_L = 0.0f;
        x1_R = x2_R = y1_R = y2_R = 0.0f;
    }

    void setCutoff(float cutoffHz) {
        if (std::abs(cutoffHz - currentCutoff) > 0.1f) {
            currentCutoff = cutoffHz;
            updateCoefficients();
        }
    }

    // Process a single stereo sample (in place or returned)
    void processSample(float inL, float inR, float& outL, float& outR) {
        if (currentCutoff <= 20.5f) {
            // Bypass filter at minimal setting
            outL = inL;
            outR = inR;
            return;
        }

        // Left channel biquad
        float outLeft = b0 * inL + b1 * x1_L + b2 * x2_L - a1 * y1_L - a2 * y2_L;
        x2_L = x1_L;
        x1_L = inL;
        y2_L = y1_L;
        y1_L = outLeft;
        outL = outLeft;

        // Right channel biquad
        float outRight = b0 * inR + b1 * x1_R + b2 * x2_R - a1 * y1_R - a2 * y2_R;
        x2_R = x1_R;
        x1_R = inR;
        y2_R = y1_R;
        y1_R = outRight;
        outR = outRight;
    }

private:
    void updateCoefficients() {
        if (currentCutoff <= 20.5f) return;

        // Standard 2nd order Butterworth highpass
        const double omega = 2.0 * M_PI * static_cast<double>(currentCutoff) / sampleRate;
        const double sinOmega = std::sin(omega);
        const double cosOmega = std::cos(omega);
        const double alpha = sinOmega / (2.0 * 0.7071067811865475); // Q = 1/sqrt(2)

        const double a0 = 1.0 + alpha;
        b0 = static_cast<float>((1.0 + cosOmega) / (2.0 * a0));
        b1 = static_cast<float>(-(1.0 + cosOmega) / a0);
        b2 = static_cast<float>((1.0 + cosOmega) / (2.0 * a0));
        a1 = static_cast<float>((-2.0 * cosOmega) / a0);
        a2 = static_cast<float>((1.0 - alpha) / a0);
    }

    double sampleRate = 44100.0;
    float currentCutoff = 20.0f;

    // Filter coefficients
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Filter state
    float x1_L = 0.0f, x2_L = 0.0f, y1_L = 0.0f, y2_L = 0.0f;
    float x1_R = 0.0f, x2_R = 0.0f, y1_R = 0.0f, y2_R = 0.0f;
};

} // namespace AuraCompressor
