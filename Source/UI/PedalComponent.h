#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "CryBabyLookAndFeel.h"
#include "../Parameters.h"

namespace cbaw::ui
{
// The hero of the GUI: a black hammertone wah pedal drawn entirely in code
// (vector paths, gradients, procedural metal texture, no bitmaps).
//  * the treadle tilts with the LIVE wah position coming from the DSP;
//  * dragging the treadle (or using the mouse wheel) sets the WAH POSITION
//    parameter (the resting / sweep-start position);
//  * clicking the chrome footswitch toggles the real BYPASS.
class PedalComponent : public juce::Component
{
public:
    static constexpr float kW = 380.0f, kH = 356.0f;

    explicit PedalComponent (juce::AudioProcessorValueTreeState& apvts)
        : positionParam (*apvts.getParameter (ids::position)),
          positionAttach (positionParam, [this] (float v) { basePosition = v; repaint(); }),
          bypassAttach (*apvts.getParameter (ids::bypass), [this] (float v) { bypassed = v > 0.5f; repaint(); }),
          texture (makeNoiseTexture (192, 20260811u))
    {
        positionAttach.sendInitialUpdate();
        bypassAttach.sendInitialUpdate();
        liveValid = false;
    }

    // Called from the editor timer with the DSP's current pedal position.
    void setLivePosition (float p)
    {
        if (! liveValid || std::abs (p - livePosition) > 0.002f)
        {
            livePosition = p; liveValid = true; repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float s = (float) getWidth() / kW;
        g.addTransform (juce::AffineTransform::scale (s));

        const float pos = juce::jlimit (0.0f, 1.0f, dragging || ! liveValid ? basePosition : juce::jmax (livePosition, 0.0f));
        paintPedal (g, pos);

        // caption
        g.setColour (juce::Colour (0xffd8c27a));
        g.setFont (makeFont (12.0f, true));
        g.drawText ("WAH POSITION  " + juce::String (juce::roundToInt (basePosition * 100.0f)) + " %   -   drag the treadle",
                    juce::Rectangle<int> (0, 336, (int) kW, 18), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = toVirtual (e.position);
        if (switchHit.contains (p))
        {
            bypassAttach.setValueAsCompleteGesture (bypassed ? 0.0f : 1.0f);
            return;
        }
        if (p.y < 255.0f)
        {
            dragging = true;
            dragStartY = e.position.y;
            dragStartValue = basePosition;
            positionAttach.beginGesture();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const float s = (float) getWidth() / kW;
        const float dy = (e.position.y - dragStartY) / s;           // drag down = toe down = brighter
        positionAttach.setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, dragStartValue + dy / 170.0f));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging) { positionAttach.endGesture(); dragging = false; repaint(); }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (! switchHit.contains (toVirtual (e.position)))
            positionAttach.setValueAsCompleteGesture (positionParam.convertFrom0to1 (positionParam.getDefaultValue()));
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        positionAttach.setValueAsCompleteGesture (juce::jlimit (0.0f, 1.0f, basePosition - w.deltaY * 0.15f));
    }

    juce::MouseCursor getMouseCursor() override { return juce::MouseCursor::PointingHandCursor; }

private:
    juce::Point<float> toVirtual (juce::Point<float> p) const { const float s = (float) getWidth() / kW; return { p.x / s, p.y / s }; }

    void texturedFill (juce::Graphics& g, const juce::Path& path, float alpha)
    {
        g.saveState();
        g.reduceClipRegion (path);
        g.setTiledImageFill (texture, 0, 0, alpha);
        g.fillPath (path);
        g.restoreState();
    }

    static juce::ColourGradient metal (float x1, float y1, float x2, float y2, bool bright = false)
    {
        return juce::ColourGradient (bright ? juce::Colour (0xff5a5d65) : juce::Colour (0xff3b3e44), x1, y1,
                                     juce::Colour (0xff0c0d0f), x2, y2, false);
    }

    void paintPedal (juce::Graphics& g, float pos)
    {
        // ---- soft studio shadow on the "table" --------------------------------
        for (int i = 0; i < 8; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.10f));
            g.fillEllipse (14.0f - (float) i * 2.0f, 308.0f - (float) i, 352.0f + (float) i * 4.0f, 30.0f + (float) i * 2.0f);
        }

        // ---- rear housing (behind the treadle) ------------------------------------
        juce::Path rear;
        rear.addRoundedRectangle (22.0f, 36.0f, 336.0f, 70.0f, 10.0f);
        g.setGradientFill (metal (0.0f, 36.0f, 0.0f, 106.0f, true));
        g.fillPath (rear);
        texturedFill (g, rear, 0.55f);
        g.setColour (juce::Colour (0xff7b7e86).withAlpha (0.7f));
        g.strokePath (rear, juce::PathStrokeType (1.0f));

        // ---- base: ledge + front face ---------------------------------------------
        juce::Path ledge;
        ledge.startNewSubPath (30.0f, 236.0f); ledge.lineTo (350.0f, 236.0f);
        ledge.lineTo (366.0f, 266.0f);         ledge.lineTo (14.0f, 266.0f);
        ledge.closeSubPath();
        ledge = ledge.createPathWithRoundedCorners (6.0f);
        g.setGradientFill (metal (0.0f, 236.0f, 0.0f, 266.0f, true));
        g.fillPath (ledge);
        texturedFill (g, ledge, 0.6f);

        juce::Path front;
        front.startNewSubPath (14.0f, 266.0f); front.lineTo (366.0f, 266.0f);
        front.lineTo (356.0f, 318.0f);         front.lineTo (24.0f, 318.0f);
        front.closeSubPath();
        front = front.createPathWithRoundedCorners (8.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2e33), 0.0f, 266.0f, juce::Colour (0xff08090a), 0.0f, 318.0f, false));
        g.fillPath (front);
        texturedFill (g, front, 0.65f);
        g.setColour (juce::Colour (0xff8a8d95).withAlpha (0.55f));
        g.drawLine (16.0f, 266.5f, 364.0f, 266.5f, 1.2f);          // bevel highlight
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (front, juce::PathStrokeType (1.0f));

        // ---- engraved label plate ---------------------------------------------------
        juce::Rectangle<float> plate (96.0f, 274.0f, 188.0f, 36.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (plate, 5.0f);
        g.setColour (juce::Colour (0xff5b5e66));
        g.drawRoundedRectangle (plate.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.setFont (makeFont (21.0f, true));
        g.drawText ("CRY BABY", plate.translated (1.0f, 1.0f).withHeight (23.0f).toNearestInt(), juce::Justification::centred, false);
        g.setColour (juce::Colour (0xfff2f2f2));
        g.drawText ("CRY BABY", plate.withHeight (23.0f).toNearestInt(), juce::Justification::centred, false);
        g.setColour (juce::Colour (0xffd8c27a));
        g.setFont (makeFont (10.0f, true));
        g.drawText ("A U T O   W A H", plate.withTrimmedTop (22.0f).toNearestInt(), juce::Justification::centred, false);

        // ---- screws ---------------------------------------------------------------------
        drawPhillipsScrew (g, 36.0f, 252.0f, 5.5f, 0.3f);
        drawPhillipsScrew (g, 344.0f, 252.0f, 5.5f, 1.1f);
        drawPhillipsScrew (g, 52.0f, 298.0f, 5.5f, 2.0f);
        drawPhillipsScrew (g, 328.0f, 298.0f, 5.5f, 0.7f);

        // ---- rubber feet ------------------------------------------------------------------
        for (float fx : { 40.0f, 340.0f })
        {
            g.setColour (juce::Colour (0xff050505));
            g.fillEllipse (fx - 14.0f, 316.0f, 28.0f, 10.0f);
            g.setColour (juce::Colour (0xff2a2a2c));
            g.drawEllipse (fx - 14.0f, 316.0f, 28.0f, 10.0f, 0.8f);
        }

        // ---- treadle ------------------------------------------------------------------------------
        const float nearY = 196.0f + 28.0f * pos;     // toe (front edge)
        const float farY  = 84.0f - 34.0f * pos;      // heel (back edge) rises when the toe goes down
        const float lipH  = 16.0f;

        juce::Path top;
        top.startNewSubPath (68.0f, farY); top.lineTo (312.0f, farY);
        top.lineTo (346.0f, nearY);        top.lineTo (34.0f, nearY);
        top.closeSubPath();
        top = top.createPathWithRoundedCorners (7.0f);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        {   // contact shadow under the treadle
            juce::Path sh (top);
            g.fillPath (sh, juce::AffineTransform::translation (0.0f, 10.0f));
        }

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff53565d), 80.0f, farY, juce::Colour (0xff17181b), 190.0f, nearY, false));
        g.fillPath (top);
        texturedFill (g, top, 0.75f);

        // studio light sweep
        g.saveState();
        g.reduceClipRegion (top);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), 110.0f, farY + 4.0f,
                                                 juce::Colours::transparentWhite, 210.0f, (farY + nearY) * 0.5f, true));
        g.fillPath (top);

        // rubber ribs (perspective-correct spacing)
        for (int i = 1; i <= 9; ++i)
        {
            const float t  = 0.08f + 0.10f * (float) (i - 1);
            const float y  = farY + (nearY - farY) * t;
            const float xl = 68.0f + (34.0f - 68.0f) * t + 14.0f;
            const float xr = 312.0f + (346.0f - 312.0f) * t - 14.0f;
            g.setColour (juce::Colour (0xff050506));
            g.fillRoundedRectangle (xl, y - 2.2f, xr - xl, 4.4f * (0.6f + 0.7f * t), 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawLine (xl + 2.0f, y - 2.4f, xr - 2.0f, y - 2.4f, 0.8f);
        }
        g.restoreState();

        g.setColour (juce::Colour (0xff9a9da6).withAlpha (0.7f));
        g.drawLine (70.0f, farY + 0.5f, 310.0f, farY + 0.5f, 1.2f);   // top edge highlight
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.strokePath (top, juce::PathStrokeType (1.0f));

        // treadle front lip
        juce::Path lip;
        lip.addRoundedRectangle (34.0f, nearY, 312.0f, lipH, 4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2d32), 0.0f, nearY, juce::Colour (0xff050506), 0.0f, nearY + lipH, false));
        g.fillPath (lip);
        texturedFill (g, lip, 0.6f);
        g.setColour (juce::Colour (0xff7b7e86).withAlpha (0.6f));
        g.drawLine (38.0f, nearY + 0.8f, 342.0f, nearY + 0.8f, 1.0f);

        // chrome pivot bolts at the heel
        for (float bx : { 78.0f, 302.0f })
            drawPhillipsScrew (g, bx, farY + 10.0f, 5.0f, bx * 0.02f);

        // ---- chrome footswitch + LED (on the rear housing) ------------------------------------------
        const float sx = 330.0f, sy = 76.0f;
        switchHit = juce::Rectangle<float> (sx - 26.0f, 30.0f, 52.0f, 62.0f);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (sx - 20.0f, sy - 2.0f, 42.0f, 14.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3f3f5), sx - 18.0f, sy, juce::Colour (0xff4a4c52), sx + 18.0f, sy + 8.0f, false));
        g.fillEllipse (sx - 19.0f, sy - 6.0f, 38.0f, 14.0f);                   // hex nut
        const float stem = bypassed ? 30.0f : 26.0f;                           // switch moves when pressed
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff777a82), sx - 9.0f, 0.0f, juce::Colour (0xfffafafa), sx - 1.0f, 0.0f, false));
        g.fillRect (sx - 9.0f, sy - stem, 18.0f, stem);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffafafa), sx - 4.0f, sy - stem - 3.0f, juce::Colour (0xff6a6d74), sx + 9.0f, sy - stem + 3.0f, false));
        g.fillEllipse (sx - 9.0f, sy - stem - 4.0f, 18.0f, 8.0f);              // chrome cap
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (sx - 9.0f, sy - stem - 4.0f, 18.0f, 8.0f, 0.8f);

        const juce::Colour led = bypassed ? juce::Colour (0xff3b0d0d) : juce::Colour (0xffff3626);
        const float lx = 296.0f, ly = 66.0f;
        if (! bypassed)
        {
            g.setColour (led.withAlpha (0.22f));
            g.fillEllipse (lx - 14.0f, ly - 14.0f, 28.0f, 28.0f);
        }
        g.setColour (juce::Colour (0xff9a9da6));
        g.drawEllipse (lx - 7.0f, ly - 7.0f, 14.0f, 14.0f, 2.0f);
        g.setGradientFill (juce::ColourGradient (led.brighter (bypassed ? 0.1f : 0.7f), lx - 2.0f, ly - 3.0f, led.darker (0.4f), lx + 5.0f, ly + 5.0f, true));
        g.fillEllipse (lx - 5.5f, ly - 5.5f, 11.0f, 11.0f);
    }

    juce::RangedAudioParameter& positionParam;
    juce::ParameterAttachment positionAttach, bypassAttach;
    juce::Image texture;

    float basePosition = 0.3f, livePosition = 0.0f;
    bool  bypassed = false, dragging = false, liveValid = false;
    float dragStartY = 0.0f, dragStartValue = 0.0f;
    juce::Rectangle<float> switchHit;
};
} // namespace cbaw::ui
