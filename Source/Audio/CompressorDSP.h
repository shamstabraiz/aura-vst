#pragma once

#include "SidechainFilter.h"
#include <cmath>
#include <algorithm>
#include <atomic>

namespace AuraCompressor {

enum class DetectionMode {
    Peak = 0,
    RMS = 1
};

struct CompressorParameters {
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 15.0f;
    float releaseMs = 100.0f;
    float kneeDb = 4.0f;
    float makeupDb = 0.0f;
    float mixPercent = 100.0f;
    float sidechainHpfHz = 20.0f;
    DetectionMode detectionMode = DetectionMode::Peak;
    bool bypassed = false;
};

class CompressorDSP {
public:
    CompressorDSP() = default;

    void prepare(double newSampleRate) {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        sidechainFilter.prepare(sampleRate);
        reset();
        updateTimeConstants();
    }

    void reset() {
        sidechainFilter.reset();
        envelopeDb = 0.0f;
        rmsState = 0.0f;
        currentGainReductionDb.store(0.0f, std::memory_order_relaxed);
        inputLevelDb.store(-100.0f, std::memory_order_relaxed);
        outputLevelDb.store(-100.0f, std::memory_order_relaxed);
    }

    void setParameters(const CompressorParameters& params) {
        parameters = params;
        sidechainFilter.setCutoff(parameters.sidechainHpfHz);
        updateTimeConstants();
    }

    void processBlock(float* const* channelData, int numChannels, int numSamples) {
        if (numChannels < 1 || numSamples < 1) return;

        float* channelLeft = channelData[0];
        float* channelRight = (numChannels > 1) ? channelData[1] : channelData[0];

        if (parameters.bypassed) {
            currentGainReductionDb.store(0.0f, std::memory_order_relaxed);
            // Measure input/output levels even when bypassed
            float maxPeak = 0.0f;
            for (int i = 0; i < numSamples; ++i) {
                maxPeak = std::max({ maxPeak, std::abs(channelLeft[i]), std::abs(channelRight[i]) });
            }
            float levelDb = toDb(maxPeak);
            inputLevelDb.store(levelDb, std::memory_order_relaxed);
            outputLevelDb.store(levelDb, std::memory_order_relaxed);
            return;
        }

        const float thresh = parameters.thresholdDb;
        const float rat = std::max(1.0f, parameters.ratio);
        const float knee = std::max(0.0f, parameters.kneeDb);
        const float halfKnee = knee * 0.5f;
        const float makeupLin = std::pow(10.0f, parameters.makeupDb / 20.0f);
        const float wet = parameters.mixPercent / 100.0f;
        const float dry = 1.0f - wet;
        const bool isRms = (parameters.detectionMode == DetectionMode::RMS);

        float blockMaxInPeak = 0.0f;
        float blockMaxOutPeak = 0.0f;
        float blockMaxGrDb = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            const float inL = channelLeft[i];
            const float inR = channelRight[i];

            blockMaxInPeak = std::max({ blockMaxInPeak, std::abs(inL), std::abs(inR) });

            // 1. Sidechain filtering on detector path
            float scL = inL;
            float scR = inR;
            sidechainFilter.processSample(inL, inR, scL, scR);

            // 2. Detection (Peak vs RMS)
            float detectorSignal = 0.0f;
            if (isRms) {
                const float power = 0.5f * (scL * scL + scR * scR);
                rmsState += alphaRms * (power - rmsState);
                detectorSignal = std::sqrt(std::max(0.0f, rmsState));
            } else {
                detectorSignal = std::max(std::abs(scL), std::abs(scR));
            }

            // 3. Convert detector signal to dB
            const float detectorDb = toDb(detectorSignal);

            // 4. Static Characteristic with Soft Knee
            float targetGainReductionDb = 0.0f;
            if (knee <= 0.05f) {
                // Hard knee
                if (detectorDb > thresh) {
                    const float over = detectorDb - thresh;
                    targetGainReductionDb = (over / rat) - over; // negative dB
                }
            } else {
                // Soft knee
                const float lowerKnee = thresh - halfKnee;
                const float upperKnee = thresh + halfKnee;

                if (detectorDb <= lowerKnee) {
                    targetGainReductionDb = 0.0f;
                } else if (detectorDb >= upperKnee) {
                    const float over = detectorDb - thresh;
                    targetGainReductionDb = (over / rat) - over;
                } else {
                    // Parabolic soft-knee transition
                    const float delta = detectorDb - thresh + halfKnee;
                    const float kneeCompression = ((1.0f / rat) - 1.0f) * (delta * delta) / (2.0f * knee);
                    targetGainReductionDb = kneeCompression;
                }
            }

            // 5. Ballistics (Attack / Release smoothing in dB domain)
            if (targetGainReductionDb < envelopeDb) {
                // Attack phase (more compression)
                envelopeDb += alphaAttack * (targetGainReductionDb - envelopeDb);
            } else {
                // Release phase (gain recovery)
                envelopeDb += alphaRelease * (targetGainReductionDb - envelopeDb);
            }

            // Protect against numerical denormals or anomalies
            if (std::abs(envelopeDb) < 1e-5f) envelopeDb = 0.0f;

            // 6. Linear Gain & Makeup
            const float grLinear = std::pow(10.0f, envelopeDb / 20.0f);
            const float compGain = grLinear * makeupLin;

            const float compL = inL * compGain;
            const float compR = inR * compGain;

            // 7. Dry/Wet Mix
            const float outL = dry * inL + wet * compL;
            const float outR = dry * inR + wet * compR;

            channelLeft[i] = outL;
            if (numChannels > 1) {
                channelRight[i] = outR;
            }

            blockMaxOutPeak = std::max({ blockMaxOutPeak, std::abs(outL), std::abs(outR) });
            blockMaxGrDb = std::max(blockMaxGrDb, -envelopeDb);
        }

        // Update real-time meters (with ballistics/peak-hold decay for smooth UI)
        currentGainReductionDb.store(blockMaxGrDb, std::memory_order_relaxed);
        inputLevelDb.store(toDb(blockMaxInPeak), std::memory_order_relaxed);
        outputLevelDb.store(toDb(blockMaxOutPeak), std::memory_order_relaxed);
    }

    // Getters for UI Metering
    float getGainReductionDb() const {
        return currentGainReductionDb.load(std::memory_order_relaxed);
    }

    float getInputLevelDb() const {
        return inputLevelDb.load(std::memory_order_relaxed);
    }

    float getOutputLevelDb() const {
        return outputLevelDb.load(std::memory_order_relaxed);
    }

    // Helper to evaluate static transfer curve for display
    static float evaluateStaticCurve(float inDb, float thresh, float rat, float knee) {
        const float halfKnee = std::max(0.0f, knee) * 0.5f;
        if (knee <= 0.05f) {
            if (inDb <= thresh) return inDb;
            return thresh + (inDb - thresh) / rat;
        }

        const float lowerKnee = thresh - halfKnee;
        const float upperKnee = thresh + halfKnee;
        if (inDb <= lowerKnee) {
            return inDb;
        } else if (inDb >= upperKnee) {
            return thresh + (inDb - thresh) / rat;
        } else {
            const float delta = inDb - thresh + halfKnee;
            return inDb + ((1.0f / rat) - 1.0f) * (delta * delta) / (2.0f * knee);
        }
    }

private:
    static inline float toDb(float linear) {
        if (linear <= 1e-5f) return -100.0f;
        return 20.0f * std::log10(linear);
    }

    void updateTimeConstants() {
        const double fs = sampleRate;
        const double attackSec = std::max(0.00005, static_cast<double>(parameters.attackMs) * 0.001);
        const double releaseSec = std::max(0.001, static_cast<double>(parameters.releaseMs) * 0.001);

        alphaAttack = static_cast<float>(1.0 - std::exp(-1.0 / (attackSec * fs)));
        alphaRelease = static_cast<float>(1.0 - std::exp(-1.0 / (releaseSec * fs)));

        // RMS window averaging (~10ms)
        const double rmsSec = 0.010;
        alphaRms = static_cast<float>(1.0 - std::exp(-1.0 / (rmsSec * fs)));
    }

    double sampleRate = 44100.0;
    CompressorParameters parameters;
    SidechainFilter sidechainFilter;

    float envelopeDb = 0.0f;
    float rmsState = 0.0f;

    float alphaAttack = 0.1f;
    float alphaRelease = 0.01f;
    float alphaRms = 0.01f;

    // Real-time meter outputs
    std::atomic<float> currentGainReductionDb { 0.0f };
    std::atomic<float> inputLevelDb { -100.0f };
    std::atomic<float> outputLevelDb { -100.0f };
};

} // namespace AuraCompressor
