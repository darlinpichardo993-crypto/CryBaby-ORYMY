#pragma once
// Pure C++17 helpers shared by the DSP code. No JUCE dependency on purpose,
// so the whole DSP engine can be unit-tested with any compiler.
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cbaw::dsp
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clamp01 (float x) noexcept            { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

// One-pole smoothing coefficient for a given time constant.
inline float timeToCoef (float seconds, float sampleRate) noexcept
{
    return seconds <= 0.0f ? 1.0f : 1.0f - std::exp (-1.0f / (seconds * sampleRate));
}

class OnePole
{
public:
    void setTime (float seconds, float sampleRate) noexcept { coef = timeToCoef (seconds, sampleRate); }
    void reset (float value = 0.0f) noexcept { z = value; }
    float process (float x) noexcept { z += coef * (x - z); return z; }
    float get() const noexcept { return z; }

private:
    float coef = 1.0f, z = 0.0f;
};

// Tiny deterministic xorshift generator (real-time safe, no allocations).
class Rng
{
public:
    explicit Rng (uint32_t seed = 0x1badf00du) noexcept : s (seed ? seed : 1u) {}
    float next01() noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s >> 8) * (1.0f / 16777216.0f);
    }
private:
    uint32_t s;
};
} // namespace cbaw::dsp
