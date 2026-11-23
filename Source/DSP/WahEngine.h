#pragma once
#include "AutoWahController.h"
#include "WahFilter.h"

namespace cbaw::dsp
{
struct EngineParams
{
    WahMode mode        = WahMode::Auto;
    float   position    = 0.30f;
    float   sensitivity = 0.50f;
    float   depth       = 0.60f;
    float   resonance   = 0.50f;
    float   drive       = 0.20f;
    float   tone        = 0.50f;
    float   speed       = 0.50f;
    bool    aventura    = false;
};

// Complete wah voice: virtual foot -> resonant filter -> transistor-style
// saturation (anti-aliased with 1st-order ADAA, so no oversampling latency)
// -> tone. Mono/stereo, allocation-free, safe for the audio thread.
class WahEngine
{
public:
    static constexpr int kMaxChannels = WahFilter::kMaxChannels;

    void prepare (double sampleRate, int numChannels);
    void reset();
    void setParams (const EngineParams& p) noexcept { params = p; }

    // in / out hold one sample per channel. They may alias.
    void processFrame (const float* in, float* out, int numChannels) noexcept;

    // Convenience block API (in place) used by the tests.
    void processBlock (float* const* channels, int numChannels, int numSamples) noexcept;

    float getPosition() const noexcept  { return position; }
    float getFrequency() const noexcept { return frequency; }
    float getQ() const noexcept         { return qCurrent; }
    float getPeakGain() const noexcept  { return peakGain; }

private:
    struct Adaa
    {
        double x1 = 0.0, f1 = 0.0;
        bool primed = false;
        float process (float xf) noexcept;
        void reset() noexcept { x1 = f1 = 0.0; primed = false; }
    };

    float sampleRate = 44100.0f;
    int   channels   = 2;
    EngineParams params;
    AutoWahController controller;
    WahFilter filter;
    OnePole resSmooth, driveSmooth, toneSmooth;
    Adaa  adaa[kMaxChannels];
    float dcInX[kMaxChannels] {}, dcInY[kMaxChannels] {};
    float dcOutX[kMaxChannels] {}, dcOutY[kMaxChannels] {};
    float toneZ[kMaxChannels] {};
    float dcR = 0.997f;

    float position = 0, frequency = 350.0f, qCurrent = 5.0f, peakGain = 2.0f;
};
} // namespace cbaw::dsp
