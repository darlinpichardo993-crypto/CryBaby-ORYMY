#pragma once
#include "AttackDetector.h"

namespace cbaw::dsp
{
enum class WahMode : int { Auto = 0, Envelope = 1, Cyclic = 2, Touch = 3 };

struct ControlParams
{
    WahMode mode        = WahMode::Auto;
    float   position    = 0.30f;   // resting "foot" position / sweep start
    float   sensitivity = 0.50f;
    float   depth       = 0.60f;   // sweep amount
    float   speed       = 0.50f;
    bool    aventura    = false;
};

// The "virtual foot". Looks at the guitar signal and decides, sample by sample,
// where the wah pedal should be (0..1). No DAW automation is involved.
class AutoWahController
{
public:
    void prepare (float sr) noexcept
    {
        sampleRate = sr;
        envFast.prepare (sr); envFast.setTimes (0.003f, 0.090f);
        envMode.prepare (sr); envMode.setTimes (0.004f, 0.20f);
        detector.prepare (sr);
        presAtk = timeToCoef (0.015f, sr);  presRel = timeToCoef (0.25f, sr);
        driftCoef = timeToCoef (0.30f, sr);
        susAtk = timeToCoef (0.60f, sr);    susRel = timeToCoef (0.15f, sr);
        touchAtk = timeToCoef (0.005f, sr);
        kickCoef = std::exp (-1.0f / (0.16f * sr));
        touchDecay = std::exp (-1.0f / (0.2f * sr));
        reset();
    }

    void reset() noexcept
    {
        envFast.reset(); envMode.reset(); detector.reset();
        presence = kick = drift = driftTarget = sustainAmp = sweepPhase = 0.0f;
        cyclePhase = touch = touchPeak = 0.0f;
        touchRising = false; sinceAttack = 10.0f; driftTimer = 0;
        level = 0.0f; position = 0.0f; lastRel = -1.0f; lastFootTime = -1.0f;
        lastSpeed = -1.0f; cycleRate = 1.0f; sweepRate = 0.5f;
        foot1.reset(); foot2.reset();
    }

    float getLevel() const noexcept    { return level; }
    float getPosition() const noexcept { return position; }

    float process (float x, const ControlParams& p) noexcept
    {
        const float av = p.aventura ? 1.0f : 0.0f;

        // ---- level detection (sensitivity = input gain of the detector) ----
        const float gain = 3.0f * std::pow (13.3f, p.sensitivity);
        const float rel  = 0.06f + 0.40f * p.speed;
        if (rel != lastRel) { envMode.setTimes (0.004f, rel); lastRel = rel; }
        const float eFast = envFast.process (x);
        const float eMode = envMode.process (x);
        const float levelAuto = 1.0f - std::exp (-1.6f * eFast * gain);
        const float levelEnv  = 1.0f - std::exp (-1.6f * eMode * gain);
        level = levelAuto;

        const auto att = detector.process (x, p.sensitivity);
        sinceAttack += 1.0f / sampleRate;

        // ---- organic drift: slow random walk shared by all modes ----------
        if (--driftTimer <= 0)
        {
            driftTarget = rng.next01() * 2.0f - 1.0f;
            driftTimer  = (int) ((0.25f + 0.65f * rng.next01()) * sampleRate / (0.5f + 1.5f * p.speed));
            sweepRate   = (0.30f + 0.55f * rng.next01()) * (0.5f + 1.5f * p.speed);
        }
        drift += (driftTarget - drift) * driftCoef;

        if (p.speed != lastSpeed) { cycleRate = 0.15f * std::pow (60.0f, p.speed); lastSpeed = p.speed; }

        float m = 0.0f, lo = 0.0f, hi = 1.0f;
        float footTime = 0.004f;

        switch (p.mode)
        {
            case WahMode::Auto:
            {
                range (p, false, lo, hi);
                footTime = 0.006f;
                const float presTarget = levelAuto > 0.05f ? 1.0f : 0.0f;
                presence += (presTarget - presence) * (presTarget > presence ? presAtk : presRel);

                if (att.triggered)
                {
                    kick = std::max (kick, clamp01 (att.strength * (0.55f + 0.5f * p.sensitivity)));
                    sinceAttack = 0.0f;
                    const float tau = (0.14f + 0.08f * rng.next01()) * (1.0f - 0.3f * av);
                    kickCoef = std::exp (-1.0f / (tau * sampleRate));
                }
                kick *= kickCoef;

                const float gate = (sinceAttack > 0.18f && levelAuto > 0.12f) ? 1.0f : 0.0f;
                sustainAmp += (gate - sustainAmp) * (gate > sustainAmp ? susAtk : susRel);
                sweepPhase += sweepRate / sampleRate;
                if (sweepPhase >= 1.0f) sweepPhase -= 1.0f;
                const float sweep = 0.5f - 0.5f * std::cos (kTwoPi * sweepPhase);

                m = 0.42f * levelAuto
                  + (0.55f + 0.12f * av) * kick
                  + presence * 0.12f * drift
                  + sustainAmp * 0.40f * (sweep - 0.5f);
                break;
            }
            case WahMode::Envelope:
            {
                range (p, false, lo, hi);
                footTime = 0.004f;
                m = levelEnv;
                break;
            }
            case WahMode::Cyclic:
            {
                range (p, true, lo, hi);
                footTime = 0.002f;
                cyclePhase += cycleRate * (1.0f + 0.04f * drift) / sampleRate;
                if (cyclePhase >= 1.0f) cyclePhase -= 1.0f;
                const float u = cyclePhase + 0.07f * std::sin (kTwoPi * cyclePhase);
                m = 0.5f - 0.5f * std::cos (kTwoPi * u);
                break;
            }
            case WahMode::Touch:
            {
                range (p, false, lo, hi);
                footTime = 0.003f;
                if (att.triggered)
                {
                    touchPeak = clamp01 (0.40f + 0.60f * att.strength * (0.4f + p.sensitivity));
                    touchRising = true;
                    const float tau = (0.05f + 0.55f * p.speed) * (1.0f - 0.3f * av);
                    touchDecay = std::exp (-1.0f / (tau * sampleRate));
                }
                if (touchRising)
                {
                    touch += (touchPeak - touch) * touchAtk;
                    if (touch >= 0.96f * touchPeak) touchRising = false;
                }
                else
                    touch *= touchDecay;
                m = touch;
                break;
            }
        }

        const float target = lo + (hi - lo) * clamp01 (m);

        // ---- foot inertia (the pedal has mass) -----------------------------
        if (footTime != lastFootTime)
        {
            foot1.setTime (footTime, sampleRate);
            foot2.setTime (footTime, sampleRate);
            lastFootTime = footTime;
        }
        position = clamp01 (foot2.process (foot1.process (target)));
        return position;
    }

private:
    static void range (const ControlParams& p, bool centred, float& lo, float& hi) noexcept
    {
        const float d = clamp01 (p.depth);
        if (centred)
        {
            lo = std::min (std::max (p.position - d * 0.5f, 0.0f), 1.0f - d);
            hi = lo + d;
        }
        else
        {
            lo = clamp01 (p.position);
            hi = std::min (1.0f, lo + d);
        }
    }

    float sampleRate = 44100.0f;
    EnvelopeFollower envFast, envMode;
    AttackDetector detector;
    Rng rng { 0x2545f491u };
    OnePole foot1, foot2;

    float presAtk = 0.001f, presRel = 0.001f, driftCoef = 0.001f, susAtk = 0.001f, susRel = 0.001f;
    float touchAtk = 0.01f, kickCoef = 0.999f, touchDecay = 0.999f;

    float presence = 0, kick = 0, drift = 0, driftTarget = 0, sustainAmp = 0, sweepPhase = 0;
    float cyclePhase = 0, touch = 0, touchPeak = 0, sinceAttack = 10.0f;
    float level = 0, position = 0, lastRel = -1.0f, lastFootTime = -1.0f, lastSpeed = -1.0f;
    float cycleRate = 1.0f, sweepRate = 0.5f;
    bool  touchRising = false;
    int   driftTimer = 0;
};
} // namespace cbaw::dsp
