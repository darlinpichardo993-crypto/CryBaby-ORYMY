#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "CryBabyLookAndFeel.h"
#include "../DSP/WahFilter.h"

namespace cbaw::ui
{
// Read-only "scope" that shows the live band-pass response of the wah.
// It reflects what the DSP is doing; it is NOT an editable automation lane.
class FilterDisplay : public juce::Component
{
public:
    void setState (float freq, float q, float peakGain)
    {
        if (std::abs (freq - fc) > 0.5f || std::abs (q - qv) > 0.01f || std::abs (peakGain - pg) > 0.005f)
        {
            fc = freq; qv = q; pg = peakGain;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b100c), 0.0f, 0.0f, juce::Colour (0xff050806), 0.0f, b.getBottom(), false));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff6c6e74));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.2f);

        auto plot = b.reduced (8.0f, 6.0f);
        constexpr float fMin = 60.0f, fMax = 8000.0f, dbMin = -30.0f, dbMax = 24.0f;
        auto xOf = [&] (float f)  { return plot.getX() + plot.getWidth() * std::log (f / fMin) / std::log (fMax / fMin); };
        auto yOf = [&] (float db) { return plot.getBottom() - plot.getHeight() * (db - dbMin) / (dbMax - dbMin); };

        // grid
        g.setColour (juce::Colour (0xff2a3a2c));
        for (float f : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f })
            g.drawVerticalLine ((int) xOf (f), plot.getY(), plot.getBottom());
        for (float db : { -20.0f, -10.0f, 0.0f, 10.0f, 20.0f })
            g.drawHorizontalLine ((int) yOf (db), plot.getX(), plot.getRight());
        g.setColour (juce::Colour (0xff4a6a4e));
        g.drawHorizontalLine ((int) yOf (0.0f), plot.getX(), plot.getRight());

        // response curve
        juce::Path curve;
        constexpr int points = 160;
        for (int i = 0; i <= points; ++i)
        {
            const float f = fMin * std::pow (fMax / fMin, (float) i / (float) points);
            const float db = juce::jlimit (dbMin, dbMax, 20.0f * std::log10 (cbaw::dsp::WahFilter::magnitudeAt (f, fc, qv, pg) + 1.0e-6f));
            if (i == 0) curve.startNewSubPath (xOf (f), yOf (db)); else curve.lineTo (xOf (f), yOf (db));
        }
        juce::Path fill (curve);
        fill.lineTo (plot.getRight(), plot.getBottom());
        fill.lineTo (plot.getX(), plot.getBottom());
        fill.closeSubPath();

        const juce::Colour phosphor (0xffffb340);
        g.setGradientFill (juce::ColourGradient (phosphor.withAlpha (0.30f), 0.0f, plot.getY(), phosphor.withAlpha (0.02f), 0.0f, plot.getBottom(), false));
        g.fillPath (fill);
        g.setColour (phosphor);
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // centre frequency marker
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawVerticalLine ((int) xOf (juce::jlimit (fMin, fMax, fc)), plot.getY(), plot.getBottom());

        g.setColour (juce::Colour (0xffb7e4bd));
        g.setFont (makeFont (11.0f, true));
        g.drawText (juce::String (juce::roundToInt (fc)) + " Hz   Q " + juce::String (qv, 1),
                    plot.toNearestInt(), juce::Justification::topRight, false);
    }

private:
    float fc = 350.0f, qv = 5.0f, pg = 2.0f;
};
} // namespace cbaw::ui
