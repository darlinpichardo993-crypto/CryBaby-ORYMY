#pragma once
#include "EnvelopeFollower.h"

namespace cbaw::dsp
{
// Detects pick attacks by comparing a fast and a slow envelope.
// A refractory period prevents one note from firing several triggers.
class AttackDetector
{
public:
    struct Result { bool triggered = false; float strength = 0.0f; };

    void prepare (float sr) noexcept
    {
        sampleRate = sr;
        fast.prepare (sr); fast.setTimes (0.001f, 0.015f);
        slow.prepare (sr); slow.setTimes (0.020f, 0.120f);
        refractory = (int) (0.075f * sr);
        cooldown = 0;
    }

    void reset() noexcept { fast.reset(); slow.reset(); cooldown = 0; }

    Result process (float x, float sensitivity) noexcept
    {
        const float f = fast.process (x);
        const float s = slow.process (x);
        if (cooldown > 0) --cooldown;

        Result r;
        const float floorLevel = lerp (0.025f, 0.003f, sensitivity);
        const float ratio      = lerp (2.4f, 1.35f, sensitivity);

        if (cooldown == 0 && f > floorLevel && f > s * ratio + 1.0e-4f)
        {
            r.triggered = true;
            r.strength  = clamp01 (f * 4.0f);
            cooldown    = refractory;
        }
        return r;
    }

private:
    float sampleRate = 44100.0f;
    EnvelopeFollower fast, slow;
    int refractory = 3000, cooldown = 0;
};
} // namespace cbaw::dsp
