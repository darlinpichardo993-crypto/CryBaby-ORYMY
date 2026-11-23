#pragma once
#include "DspCommon.h"

namespace cbaw::dsp
{
// Rectifying peak envelope follower with independent attack / release times.
class EnvelopeFollower
{
public:
    void prepare (float sr) noexcept { sampleRate = sr; setTimes (attackSec, releaseSec); reset(); }

    void setTimes (float attack, float release) noexcept
    {
        attackSec = attack; releaseSec = release;
        aA = timeToCoef (attack, sampleRate);
        aR = timeToCoef (release, sampleRate);
    }

    void reset() noexcept { env = 0.0f; }

    float process (float x) noexcept
    {
        x = std::fabs (x);
        env += (x > env ? aA : aR) * (x - env);
        return env;
    }

    float get() const noexcept { return env; }

private:
    float sampleRate = 44100.0f, attackSec = 0.003f, releaseSec = 0.09f;
    float aA = 1.0f, aR = 0.001f, env = 0.0f;
};
} // namespace cbaw::dsp
