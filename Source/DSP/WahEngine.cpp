#include "WahEngine.h"

namespace cbaw::dsp
{
namespace
{
    // log(cosh(x)) without overflow: antiderivative of tanh.
    double logCosh (double x) noexcept
    {
        const double a = std::fabs (x);
        return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453;
    }
}

float WahEngine::Adaa::process (float xf) noexcept
{
    const double x  = (double) xf;
    const double f  = logCosh (x);
    if (! primed) { primed = true; x1 = x; f1 = f; return (float) std::tanh (x); }
    const double dx = x - x1;
    const double y  = std::fabs (dx) < 1.0e-5 ? std::tanh (0.5 * (x + x1)) : (f - f1) / dx;
    x1 = x; f1 = f;
    return (float) y;
}

void WahEngine::prepare (double sr, int numChannels)
{
    sampleRate = (float) sr;
    channels   = std::min (std::max (numChannels, 1), kMaxChannels);
    controller.prepare (sampleRate);
    filter.prepare (sampleRate);
    resSmooth.setTime (0.02f, sampleRate);
    driveSmooth.setTime (0.02f, sampleRate);
    toneSmooth.setTime (0.02f, sampleRate);
    dcR = 1.0f - kTwoPi * 20.0f / sampleRate;
    reset();
    resSmooth.reset (params.resonance);
    driveSmooth.reset (params.drive);
    toneSmooth.reset (params.tone);
}

void WahEngine::reset()
{
    controller.reset();
    filter.reset();
    for (int c = 0; c < kMaxChannels; ++c)
    {
        adaa[c].reset();
        dcInX[c] = dcInY[c] = dcOutX[c] = dcOutY[c] = toneZ[c] = 0.0f;
    }
}

void WahEngine::processFrame (const float* in, float* out, int numChannels) noexcept
{
    const int nCh = std::min (numChannels, kMaxChannels);

    // ---- mono detection signal -------------------------------------------
    float mono = 0.0f;
    for (int c = 0; c < nCh; ++c) mono += in[c];
    mono /= (float) std::max (nCh, 1);
    if (! std::isfinite (mono)) mono = 0.0f;

    // ---- virtual foot -------------------------------------------------------
    ControlParams cp;
    cp.mode = params.mode;         cp.position = params.position;
    cp.sensitivity = params.sensitivity; cp.depth = params.depth;
    cp.speed = params.speed;       cp.aventura = params.aventura;

    position = controller.process (mono, cp);
    const float level = controller.getLevel();
    const float av = params.aventura ? 1.0f : 0.0f;

    // ---- voicing --------------------------------------------------------------
    // "Aventura mode": narrower, mid-focused sweep with a slightly more vocal
    // resonance and a touch more bite, closer to the nasal rhythmic wah sound
    // of 2000s bachata guitars.
    const float res   = resSmooth.process (params.resonance);
    const float drv   = clamp01 (driveSmooth.process (params.drive) + 0.08f * av);
    const float tone  = clamp01 (toneSmooth.process (params.tone) + 0.05f * av);

    const float fLow  = av > 0.5f ? 450.0f  : 350.0f;
    const float fHigh = av > 0.5f ? 1900.0f : 2200.0f;
    frequency = WahFilter::positionToFrequency (position, fLow, fHigh);

    // Dynamic response: a harder pick lowers the effective Q a little, like a
    // loaded inductor.
    qCurrent  = (1.5f + 10.5f * std::pow (res, 1.3f)) * (1.0f + 0.10f * av) * (1.0f - 0.18f * level);
    peakGain  = 1.8f + 1.4f * position + 0.4f * av;
    filter.setCoefficients (frequency, qCurrent, peakGain);

    const float satGain = 1.0f + 9.0f * std::pow (drv, 1.5f);
    const float satBias = 0.08f * drv;
    const float satComp = 0.7f / std::sqrt (satGain);
    const float satOffset = std::tanh (satBias); // resting output of the biased stage (removed so silence stays silent)

    const float toneHz   = std::min (3000.0f * std::pow (5.5f, tone), 0.45f * sampleRate);
    const float toneCoef = 1.0f - std::exp (-kTwoPi * toneHz / sampleRate);

    // ---- per channel signal path --------------------------------------------------
    for (int c = 0; c < nCh; ++c)
    {
        float x = in[c];
        if (! std::isfinite (x)) x = 0.0f;

        // input DC blocker (20 Hz)
        float y = x - dcInX[c] + dcR * dcInY[c];
        dcInX[c] = x; dcInY[c] = y;

        y = filter.process (c, y);

        // transistor gain stage: asymmetric tanh, ADAA anti-aliased
        y = (adaa[c].process (y * satGain + satBias) - satOffset) * satComp;

        // tone (post-filter low-pass)
        toneZ[c] += toneCoef * (y - toneZ[c]);
        y = toneZ[c];

        // output DC blocker (removes the saturation bias offset)
        const float o = y - dcOutX[c] + dcR * dcOutY[c];
        dcOutX[c] = y; dcOutY[c] = o;

        if (! std::isfinite (o) || std::fabs (o) > 50.0f)
        {
            reset();
            out[c] = 0.0f;
        }
        else
            out[c] = o;
    }
}

void WahEngine::processBlock (float* const* ch, int numChannels, int numSamples) noexcept
{
    float frame[kMaxChannels] {};
    const int nCh = std::min (numChannels, kMaxChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        for (int c = 0; c < nCh; ++c) frame[c] = ch[c][i];
        processFrame (frame, frame, nCh);
        for (int c = 0; c < nCh; ++c) ch[c][i] = frame[c];
    }
}
} // namespace cbaw::dsp
