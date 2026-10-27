#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Audio/CompressorDSP.h"
#include "CustomLookAndFeel.h"

namespace AuraCompressor {

class TransferCurveComponent : public juce::Component {
public:
    TransferCurveComponent() = default;

    void updateCurve(float newThresholdDb, float newRatio, float newKneeDb, float currentInDb, float currentOutDb)
    {
        threshold = newThresholdDb;
        ratio = newRatio;
        knee = newKneeDb;
        inDb = currentInDb;
        outDb = currentOutDb;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Background Panel
        g.setColour(CustomLookAndFeel::dialDark);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(CustomLookAndFeel::bgPanelBorder);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        auto plotArea = bounds.reduced(18.0f, 14.0f);
        if (plotArea.getWidth() <= 10.0f || plotArea.getHeight() <= 10.0f) return;

        const float minDb = -60.0f;
        const float maxDb = 0.0f;
        const float dbRange = maxDb - minDb;

        auto mapX = [&](float db) -> float {
            return plotArea.getX() + plotArea.getWidth() * ((db - minDb) / dbRange);
        };

        auto mapY = [&](float db) -> float {
            // High dB at top (y=plotArea.getY()), low dB at bottom (y=plotArea.getBottom())
            return plotArea.getBottom() - plotArea.getHeight() * ((db - minDb) / dbRange);
        };

        // 2. Grid lines & dB labels
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(CustomLookAndFeel::bgPanelBorder.withAlpha(0.7f));

        const float gridValues[] = { -48.0f, -36.0f, -24.0f, -12.0f, 0.0f };
        for (float val : gridValues) {
            const float gx = mapX(val);
            const float gy = mapY(val);

            // Vertical line
            g.drawVerticalLine(static_cast<int>(gx), plotArea.getY(), plotArea.getBottom());
            // Horizontal line
            g.drawHorizontalLine(static_cast<int>(gy), plotArea.getX(), plotArea.getRight());

            // dB labels
            g.setColour(CustomLookAndFeel::textDim.withAlpha(0.6f));
            g.drawText(juce::String(static_cast<int>(val)), static_cast<int>(gx - 14.0f), static_cast<int>(plotArea.getBottom() + 1.0f), 28, 12, juce::Justification::centred);
            g.setColour(CustomLookAndFeel::bgPanelBorder.withAlpha(0.7f));
        }

        // 3. Unity 1:1 line (dashed line from -60 dB to 0 dB)
        juce::Path unityPath;
        unityPath.startNewSubPath(mapX(minDb), mapY(minDb));
        unityPath.lineTo(mapX(maxDb), mapY(maxDb));

        juce::PathStrokeType dashedStroke(1.0f);
        const float dashLengths[] = { 4.0f, 4.0f };
        dashedStroke.createDashedStroke(unityPath, unityPath, dashLengths, 2);
        g.setColour(CustomLookAndFeel::textDim.withAlpha(0.3f));
        g.strokePath(unityPath, dashedStroke);

        // 4. Threshold Marker (vertical subtle guide)
        const float threshX = mapX(threshold);
        g.setColour(CustomLookAndFeel::purpleAccent.withAlpha(0.4f));
        g.drawVerticalLine(static_cast<int>(threshX), plotArea.getY(), plotArea.getBottom());

        // 5. Transfer Curve Path
        juce::Path curvePath;
        juce::Path fillPath;
        const int steps = 80;

        for (int i = 0; i <= steps; ++i) {
            const float xDb = minDb + (static_cast<float>(i) / static_cast<float>(steps)) * dbRange;
            const float yDb = juce::jlimit(minDb, maxDb, CompressorDSP::evaluateStaticCurve(xDb, threshold, ratio, knee));

            const float px = mapX(xDb);
            const float py = mapY(yDb);

            if (i == 0) {
                curvePath.startNewSubPath(px, py);
                fillPath.startNewSubPath(px, plotArea.getBottom());
                fillPath.lineTo(px, py);
            } else {
                curvePath.lineTo(px, py);
                fillPath.lineTo(px, py);
            }
        }
        fillPath.lineTo(plotArea.getRight(), plotArea.getBottom());
        fillPath.closeSubPath();

        // Fill area under curve
        juce::ColourGradient fillGrad(CustomLookAndFeel::cyanAccent.withAlpha(0.18f), plotArea.getCentreX(), plotArea.getY(),
                                      CustomLookAndFeel::cyanAccent.withAlpha(0.02f), plotArea.getCentreX(), plotArea.getBottom(), false);
        g.setGradientFill(fillGrad);
        g.fillPath(fillPath);

        // Glow stroke
        g.setColour(CustomLookAndFeel::cyanAccent.withAlpha(0.3f));
        g.strokePath(curvePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Crisp curve stroke
        g.setColour(CustomLookAndFeel::cyanAccent);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 6. Dynamic Operating Point (Real-time animated point)
        if (inDb > minDb + 1.0f) {
            const float ptIn = juce::jlimit(minDb, maxDb, inDb);
            const float ptOut = juce::jlimit(minDb, maxDb, outDb);
            const float ptX = mapX(ptIn);
            const float ptY = mapY(ptOut);

            // Halo
            g.setColour(CustomLookAndFeel::orangeAccent.withAlpha(0.4f));
            g.fillEllipse(ptX - 6.0f, ptY - 6.0f, 12.0f, 12.0f);

            // Bright Core
            g.setColour(juce::Colours::white);
            g.fillEllipse(ptX - 3.0f, ptY - 3.0f, 6.0f, 6.0f);
        }

        // Title and readout overlay
        g.setColour(CustomLookAndFeel::textDim);
        g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
        g.drawText("TRANSFER CURVE", plotArea.getX() + 6, plotArea.getY() + 4, 120, 14, juce::Justification::left);
    }

private:
    float threshold = -18.0f;
    float ratio = 4.0f;
    float knee = 4.0f;
    float inDb = -60.0f;
    float outDb = -60.0f;
};

} // namespace AuraCompressor
