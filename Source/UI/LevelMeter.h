#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace cbaw::ui
{
// Horizontal segmented LED meter (-60 dB .. 0 dB).
class LevelMeter : public juce::Component
{
public:
    void setLevelDb (float db)
    {
        db = juce::jlimit (-60.0f, 0.0f, db);
        if (std::abs (db - levelDb) > 0.05f) { levelDb = db; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff060607));
        g.fillRoundedRectangle (area, 3.0f);

        constexpr int n = 24;
        const float segW = (area.getWidth() - 6.0f) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const float thresholdDb = -60.0f + 60.0f * (float) (i + 1) / (float) n;
            const bool lit = levelDb >= thresholdDb - 2.5f;
            juce::Colour c = thresholdDb > -3.0f ? juce::Colour (0xffff3b30)
                           : thresholdDb > -12.0f ? juce::Colour (0xffffb340)
                                                  : juce::Colour (0xff5ee27a);
            g.setColour (lit ? c : c.withAlpha (0.12f));
            g.fillRect (3.0f + (float) i * segW + 0.5f, area.getY() + 3.0f, segW - 1.5f, area.getHeight() - 6.0f);
        }
    }

private:
    float levelDb = -60.0f;
};
} // namespace cbaw::ui
