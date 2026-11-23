#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace cbaw::ui
{
inline juce::Font makeFont (float height, bool bold = false)
{
    juce::Font f (juce::FontOptions { height });
    return bold ? f.boldened() : f;
}

// Brushed-metal noise tile used as a "hammertone" texture.
inline juce::Image makeNoiseTexture (int size, juce::uint32 seed)
{
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Random rnd ((juce::int64) seed);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float r = rnd.nextFloat();
            if (r < 0.30f)      img.setPixelAt (x, y, juce::Colours::white.withAlpha (0.05f + 0.20f * rnd.nextFloat()));
            else if (r < 0.65f) img.setPixelAt (x, y, juce::Colours::black.withAlpha (0.10f + 0.30f * rnd.nextFloat()));
        }
    return img;
}

inline void drawPhillipsScrew (juce::Graphics& g, float cx, float cy, float r, float angle)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - r + 0.8f, cy - r + 1.2f, r * 2.0f, r * 2.0f);

    juce::ColourGradient grad (juce::Colour (0xfff4f4f6), cx - r * 0.6f, cy - r * 0.7f,
                               juce::Colour (0xff55575c), cx + r, cy + r, true);
    g.setGradientFill (grad);
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colour (0xff202124));
    g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);

    juce::Path cross;
    cross.addRectangle (-r * 0.75f, -r * 0.13f, r * 1.5f, r * 0.26f);
    cross.addRectangle (-r * 0.13f, -r * 0.75f, r * 0.26f, r * 1.5f);
    g.setColour (juce::Colour (0xff2a2b2e));
    g.fillPath (cross, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

class CryBabyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CryBabyLookAndFeel()
    {
        const juce::Colour cream (0xffe9d9a6), amber (0xffd8c27a);
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0d0d0f));
        setColour (juce::Slider::textBoxTextColourId, cream);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xffc9c9cd));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffb9b9bd));
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xfffff1c2));
        setColour (juce::ComboBox::textColourId, cream);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff141416));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff5a5c62));
        setColour (juce::ComboBox::arrowColourId, amber);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141416));
        setColour (juce::PopupMenu::textColourId, cream);
        setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xff8a8a90));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a3320));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1b1c1f));
        setColour (juce::AlertWindow::textColourId, cream);
        setColour (juce::AlertWindow::outlineColourId, juce::Colour (0xff5a5c62));
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0b0c));
        setColour (juce::TextEditor::textColourId, cream);
        setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff5a5c62));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStart, float rotaryEnd, juce::Slider&) override
    {
        const float d  = (float) juce::jmin (width, height) - 6.0f;
        const float r  = d * 0.5f;
        const float cx = (float) x + (float) width * 0.5f;
        const float cy = (float) y + (float) height * 0.5f;
        const float angle = rotaryStart + sliderPos * (rotaryEnd - rotaryStart);

        // engraved tick marks
        for (int i = 0; i <= 10; ++i)
        {
            const float a = rotaryStart + (float) i / 10.0f * (rotaryEnd - rotaryStart);
            const float s = std::sin (a), c = -std::cos (a);
            g.setColour (juce::Colours::white.withAlpha (i == 0 || i == 10 ? 0.85f : 0.45f));
            g.drawLine (cx + s * r * 0.90f, cy + c * r * 0.90f, cx + s * r, cy + c * r, i % 5 == 0 ? 1.8f : 1.1f);
        }

        const float kr = r * 0.76f;

        // cast shadow
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.12f));
            g.fillEllipse (cx - kr - (float) i + 1.5f, cy - kr - (float) i + 4.0f, (kr + (float) i) * 2.0f, (kr + (float) i) * 2.0f);
        }

        // chrome skirt
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6f6f8), cx - kr, cy - kr,
                                                 juce::Colour (0xff3d3f44), cx + kr, cy + kr, false));
        g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);

        // black knob body
        const float br = kr * 0.88f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b4d54), cx - br * 0.35f, cy - br * 0.5f,
                                                 juce::Colour (0xff09090a), cx + br * 0.3f, cy + br * 0.6f, true));
        g.fillEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f);

        // fluting
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        for (int i = 0; i < 28; ++i)
        {
            const float a = (float) i / 28.0f * juce::MathConstants<float>::twoPi;
            g.drawLine (cx + std::sin (a) * br * 0.80f, cy - std::cos (a) * br * 0.80f,
                        cx + std::sin (a) * br * 0.98f, cy - std::cos (a) * br * 0.98f, 1.0f);
        }

        // brushed cap
        const float cr = br * 0.62f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e3035), cx - cr, cy - cr,
                                                 juce::Colour (0xff0c0c0e), cx + cr, cy + cr, false));
        g.fillEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f, 1.0f);

        // pointer
        juce::Path p;
        p.startNewSubPath (cx + std::sin (angle) * cr * 0.25f, cy - std::cos (angle) * cr * 0.25f);
        p.lineTo (cx + std::sin (angle) * br * 0.95f, cy - std::cos (angle) * br * 0.95f);
        g.setColour (juce::Colour (0xfff4f1e6));
        g.strokePath (p, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();

        auto top = juce::Colour (0xff35373c), bottom = juce::Colour (0xff131416);
        if (down)        std::swap (top, bottom);
        else if (highlighted) top = top.brighter (0.15f);

        g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xff66686e).withAlpha (0.85f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        if ((bool) b.getProperties().getWithDefault ("led", false))
        {
            const auto ledCol = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("ledColour", (int) 0xffffa31a));
            const float rad = juce::jmin (5.0f, r.getHeight() * 0.18f);
            const float lx = r.getX() + rad + 8.0f, ly = r.getCentreY();
            if (on)
            {
                g.setColour (ledCol.withAlpha (0.25f));
                g.fillEllipse (lx - rad * 2.0f, ly - rad * 2.0f, rad * 4.0f, rad * 4.0f);
            }
            g.setGradientFill (juce::ColourGradient (on ? ledCol.brighter (0.6f) : juce::Colour (0xff3a2a1e), lx - rad * 0.4f, ly - rad * 0.5f,
                                                     on ? ledCol.darker (0.3f) : juce::Colour (0xff160f0a), lx + rad, ly + rad, true));
            g.fillEllipse (lx - rad, ly - rad, rad * 2.0f, rad * 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawEllipse (lx - rad, ly - rad, rad * 2.0f, rad * 2.0f, 0.8f);
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return makeFont (juce::jmin (15.0f, (float) buttonHeight * 0.46f), true);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0a0b), 0.0f, 0.0f, juce::Colour (0xff1c1d20), 0.0f, (float) h, false));
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (juce::Colour (0xff66686e));
        g.drawRoundedRectangle (b, 4.0f, 1.0f);

        const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
        juce::Path p;
        p.addTriangle (ax - 5.0f, ay - 3.0f, ax + 5.0f, ay - 3.0f, ax, ay + 4.0f);
        g.setColour (juce::Colour (0xffd8c27a));
        g.fillPath (p);
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        return makeFont (juce::jmin (16.0f, (float) box.getHeight() * 0.5f), true);
    }

    juce::Font getPopupMenuFont() override { return makeFont (15.0f, true); }
};
} // namespace cbaw::ui
