#pragma once
#include "DspCommon.h"

namespace cbaw::dsp
{
// Model inspired by the inductor-based Cry Baby wah circuit:
//  * 2nd-order resonant band-pass (the LC tank) solved with a zero-delay
//    feedback state-variable filter (TPT / Simper topology), so the sweep
//    stays stable and artefact-free even when the frequency moves every sample;
//  * a small amount of the low-pass output is kept, because a real wah never
//    removes the low end completely ("heel down" is thick, not thin);
//  * the peak gain rises towards the toe-down position, like the real pedal.
class WahFilter
{
public:
    static constexpr int   kMaxChannels = 2;
    static constexpr float kLowMix      = 0.25f;

    void prepare (float sr) noexcept { sampleRate = sr; reset(); }

    void reset() noexcept
    {
        for (int c = 0; c < kMaxChannels; ++c) ic1[c] = ic2[c] = 0.0f;
    }

    void setCoefficients (float freq, float q, float peakGain) noexcept
    {
        freq = std::min (std::max (freq, 20.0f), 0.45f * sampleRate);
        q    = std::max (q, 0.3f);
        const float g = std::tan (kPi * freq / sampleRate);
        const float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        bandGain = peakGain * k;           // k * band = unity-gain band-pass
    }

    float process (int ch, float v0) noexcept
    {
        const float v3 = v0 - ic2[ch];
        const float v1 = a1 * ic1[ch] + a2 * v3;           // band
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3; // low
        ic1[ch] = 2.0f * v1 - ic1[ch];
        ic2[ch] = 2.0f * v2 - ic2[ch];
        if (std::fabs (ic1[ch]) < 1.0e-20f) ic1[ch] = 0.0f;
        if (std::fabs (ic2[ch]) < 1.0e-20f) ic2[ch] = 0.0f;
        return bandGain * v1 + kLowMix * v2;
    }

    // Foot position (0 = heel down, 1 = toe down) -> resonant frequency.
    // Slightly S-shaped, like the pot taper + geometry of a real pedal.
    static float positionToFrequency (float pos, float fLow, float fHigh) noexcept
    {
        const float p      = clamp01 (pos);
        const float shaped = 0.75f * p + 0.25f * p * p * (3.0f - 2.0f * p);
        return fLow * std::pow (fHigh / fLow, shaped);
    }

    // Analytic magnitude response (linear) of the analogue prototype.
    // Used by the GUI curve and by the unit tests.
    static float magnitudeAt (float f, float fc, float q, float peakGain) noexcept
    {
        const float x = f / fc, k = 1.0f / q;
        const float nRe = kLowMix, nIm = peakGain * k * x;
        const float dRe = 1.0f - x * x, dIm = k * x;
        return std::sqrt ((nRe * nRe + nIm * nIm) / (dRe * dRe + dIm * dIm + 1.0e-12f));
    }

private:
    float sampleRate = 44100.0f;
    float a1 = 0, a2 = 0, a3 = 0, bandGain = 1.0f;
    float ic1[kMaxChannels] {}, ic2[kMaxChannels] {};
};
} // namespace cbaw::dsp
