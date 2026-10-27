#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CustomLookAndFeel.h"

namespace AuraCompressor {

/**
 * Vertical Gain Reduction Meter (0 dB to -30 dB)
 */
class GainReductionMeter : public juce::Component {
public:
    GainReductionMeter() = default;

    void setLevel(float grDb) {
        // grDb is positive reduction in dB (0 to 30)
        targetGrDb = juce::jlimit(0.0f, 30.0f, grDb);

        // Instant attack, smooth release decay
        if (targetGrDb > displayedGrDb) {
            displayedGrDb = targetGrDb;
        } else {
            displayedGrDb += (targetGrDb - displayedGrDb) * 0.15f;
        }

        if (displayedGrDb < 0.05f) displayedGrDb = 0.0f;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Background panel
        g.setColour(CustomLookAndFeel::dialDark);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(CustomLookAndFeel::bgPanelBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        auto meterRect = bounds.reduced(3.0f);
        const float maxRangeDb = 30.0f;
        const float prop = juce::jlimit(0.0f, 1.0f, displayedGrDb / maxRangeDb);

        // Fill downward from top (0 dB at top)
        if (prop > 0.001f) {
            auto barRect = meterRect;
            barRect.setHeight(meterRect.getHeight() * prop);

            juce::ColourGradient grad(CustomLookAndFeel::orangeAccent.withAlpha(0.7f), barRect.getCentreX(), barRect.getY(),
                                      CustomLookAndFeel::orangeAccent, barRect.getCentreX(), barRect.getBottom(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(barRect, 3.0f);
        }

        // dB Tick marks
        g.setColour(CustomLookAndFeel::textDim.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));

        const float ticks[] = { 3.0f, 6.0f, 12.0f, 18.0f, 24.0f };
        for (float tick : ticks) {
            const float tickY = meterRect.getY() + meterRect.getHeight() * (tick / maxRangeDb);
            g.drawHorizontalLine(static_cast<int>(tickY), meterRect.getX() + 1.0f, meterRect.getRight() - 1.0f);
        }
    }

private:
    float targetGrDb = 0.0f;
    float displayedGrDb = 0.0f;
};

/**
 * Vertical Audio Level Meter (-60 dB to +6 dB)
 */
class LevelMeter : public juce::Component {
public:
    LevelMeter() = default;

    void setLevel(float levelDb) {
        targetDb = juce::jlimit(-60.0f, 6.0f, levelDb);

        // Instant attack, smooth release
        if (targetDb > displayedDb) {
            displayedDb = targetDb;
        } else {
            displayedDb += (targetDb - displayedDb) * 0.12f;
        }

        if (displayedDb > 0.0f) {
            clipHoldCounter = 30; // hold red clip light
        } else if (clipHoldCounter > 0) {
            --clipHoldCounter;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Meter background
        g.setColour(CustomLookAndFeel::dialDark);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(CustomLookAndFeel::bgPanelBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        auto meterRect = bounds.reduced(3.0f);
        // Normalize -60 dB to +6 dB -> range 66 dB
        const float prop = juce::jlimit(0.0f, 1.0f, (displayedDb + 60.0f) / 66.0f);

        if (prop > 0.01f) {
            const float fillHeight = meterRect.getHeight() * prop;
            auto barRect = meterRect.removeFromBottom(fillHeight);

            // Three-color vertical gradient (green -> amber -> red)
            juce::ColourGradient grad(CustomLookAndFeel::greenAccent, barRect.getCentreX(), meterRect.getBottom(),
                                      CustomLookAndFeel::cyanAccent, barRect.getCentreX(), meterRect.getY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(barRect, 3.0f);
        }

        // Clip indicator dot at the top
        if (clipHoldCounter > 0) {
            g.setColour(CustomLookAndFeel::redAccent);
            g.fillEllipse(bounds.getCentreX() - 3.0f, bounds.getY() + 3.0f, 6.0f, 6.0f);
        }
    }

private:
    float targetDb = -60.0f;
    float displayedDb = -60.0f;
    int clipHoldCounter = 0;
};

} // namespace AuraCompressor
