// Stand-alone self test of the DSP engine (no JUCE needed).
// Build:  cmake -B build -S . -DCBAW_BUILD_TESTS=ON && cmake --build build --target DspSelfTest
// Run:    ctest --test-dir build -C Release --output-on-failure
#include "../Source/DSP/WahEngine.h"
#include "../Source/Presets/FactoryPresets.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace cbaw::dsp;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("  FAIL: "); std::printf (__VA_ARGS__); std::printf ("\n"); } \
                              else { std::printf ("  ok:   "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static constexpr double kSr = 48000.0;

// Synthetic plucked-string signal: decaying harmonic series, one note every `period` seconds.
static std::vector<float> makeGuitar (double seconds, double period, float amp)
{
    std::vector<float> s ((size_t) (seconds * kSr), 0.0f);
    const double f0[] = { 110.0, 146.8, 164.8, 130.8 };
    for (size_t i = 0; i < s.size(); ++i)
    {
        const double t = (double) i / kSr;
        const int n = (int) (t / period);
        const double tn = t - n * period;
        const double f = f0[n % 4];
        const double env = std::exp (-tn * 3.0) * (tn < 0.002 ? tn / 0.002 : 1.0);
        double v = 0;
        for (int h = 1; h <= 6; ++h) v += std::sin (6.283185307 * f * h * t) / h;
        s[i] = (float) (amp * env * v * 0.6);
    }
    return s;
}

struct Stats { float minPos = 1, maxPos = 0, peak = 0; bool finite = true; };

static Stats run (WahEngine& e, const std::vector<float>& in, std::vector<float>* posOut = nullptr)
{
    Stats st;
    for (float x : in)
    {
        float inF[2] = { x, x }, outF[2];
        e.processFrame (inF, outF, 2);
        st.minPos = std::min (st.minPos, e.getPosition());
        st.maxPos = std::max (st.maxPos, e.getPosition());
        st.peak   = std::max (st.peak, std::fabs (outF[0]));
        if (! std::isfinite (outF[0]) || ! std::isfinite (outF[1])) st.finite = false;
        if (posOut) posOut->push_back (e.getPosition());
    }
    return st;
}

int main()
{
    std::printf ("== Filter magnitude response ==\n");
    {
        // Measure steady-state gain of the digital filter at fc and compare with analytic response.
        const float fc = 1000.0f, q = 5.0f, pg = 3.0f;
        for (float mult : { 0.5f, 1.0f, 2.0f })
        {
            WahFilter f; f.prepare ((float) kSr); f.setCoefficients (fc, q, pg);
            float peak = 0;
            const float freq = fc * mult;
            for (int i = 0; i < (int) (kSr * 0.6); ++i)
            {
                const float y = f.process (0, std::sin (kTwoPi * freq * (float) i / (float) kSr));
                if (i > (int) (kSr * 0.4)) peak = std::max (peak, std::fabs (y));
            }
            const float expected = WahFilter::magnitudeAt (freq, fc, q, pg);
            const float dB = 20.0f * std::log10 (peak / expected);
            CHECK (std::fabs (dB) < 1.5f, "gain at %.0f Hz: measured %.2f vs analytic %.2f (diff %.2f dB)", freq, peak, expected, dB);
        }
    }

    std::printf ("== Attack detector ==\n");
    {
        AttackDetector d; d.prepare ((float) kSr);
        auto g = makeGuitar (4.0, 0.5, 0.3f); // 8 notes
        int triggers = 0;
        for (float x : g) if (d.process (x, 0.5f).triggered) ++triggers;
        CHECK (triggers >= 7 && triggers <= 9, "plucked notes: 8 notes -> %d triggers", triggers);

        AttackDetector d2; d2.prepare ((float) kSr);
        int t2 = 0;
        for (int i = 0; i < (int) (kSr * 3); ++i)
            if (d2.process (0.3f * std::sin (kTwoPi * 220.0f * (float) i / (float) kSr), 0.5f).triggered) ++t2;
        CHECK (t2 <= 1, "steady sine: %d triggers (no repeated firing)", t2);
    }

    std::printf ("== Engine modes ==\n");
    const char* names[] = { "AUTO", "ENVELOPE", "CYCLIC", "TOUCH" };
    for (int m = 0; m < 4; ++m)
    {
        WahEngine e; e.prepare (kSr, 2);
        EngineParams p; p.mode = (WahMode) m; p.position = 0.2f; p.depth = 0.8f; p.speed = 0.5f;
        e.setParams (p);
        auto st = run (e, makeGuitar (6.0, 0.5, 0.3f));
        CHECK (st.finite && st.peak < 6.0f, "%s: output finite, peak %.2f", names[m], st.peak);
        CHECK (st.maxPos - st.minPos > 0.2f, "%s: pedal moves by itself (%.2f..%.2f)", names[m], st.minPos, st.maxPos);
    }

    std::printf ("== Modes sound different ==\n");
    {
        std::vector<std::vector<float>> traces;
        for (int m = 0; m < 4; ++m)
        {
            WahEngine e; e.prepare (kSr, 2);
            EngineParams p; p.mode = (WahMode) m; p.depth = 0.8f; e.setParams (p);
            std::vector<float> pos;
            run (e, makeGuitar (4.0, 0.5, 0.3f), &pos);
            traces.push_back (pos);
        }
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b)
            {
                double diff = 0;
                for (size_t i = 0; i < traces[a].size(); ++i) diff += std::fabs (traces[a][i] - traces[b][i]);
                diff /= (double) traces[a].size();
                CHECK (diff > 0.02, "%s vs %s: mean position difference %.3f", names[a], names[b], diff);
            }
    }

    std::printf ("== Envelope follows intensity ==\n");
    {
        auto meanPos = [] (float amp)
        {
            WahEngine e; e.prepare (kSr, 2);
            EngineParams p; p.mode = WahMode::Envelope; p.position = 0.1f; p.depth = 0.8f; e.setParams (p);
            std::vector<float> pos; run (e, makeGuitar (3.0, 0.5, amp), &pos);
            double s = 0; for (float v : pos) s += v; return s / (double) pos.size();
        };
        const double soft = meanPos (0.03f), hard = meanPos (0.4f);
        CHECK (hard > soft + 0.05, "harder playing opens the wah more (soft %.3f, hard %.3f)", soft, hard);
    }

    std::printf ("== Cyclic speed ==\n");
    {
        WahEngine e; e.prepare (kSr, 2);
        EngineParams p; p.mode = WahMode::Cyclic; p.depth = 0.8f; p.speed = 0.5f; e.setParams (p);
        std::vector<float> pos; run (e, std::vector<float> ((size_t) (kSr * 10), 0.0f), &pos);
        double mean = 0; for (size_t i = (size_t) kSr; i < pos.size(); ++i) mean += pos[i];
        mean /= (double) (pos.size() - (size_t) kSr);
        int up = 0; for (size_t i = (size_t) kSr + 1; i < pos.size(); ++i) if (pos[i - 1] < mean && pos[i] >= mean) ++up;
        const double hz = up / 9.0, expected = 0.15 * std::pow (60.0, 0.5);
        CHECK (std::fabs (hz - expected) < 0.25, "cycle rate %.2f Hz (expected ~%.2f Hz)", hz, expected);
    }

    std::printf ("== Robustness ==\n");
    {
        WahEngine e; e.prepare (kSr, 2);
        EngineParams p; e.setParams (p);
        std::vector<float> sil ((size_t) kSr, 0.0f);
        auto st = run (e, sil);
        CHECK (st.finite && st.peak < 1e-3f, "silence in -> silence out (peak %.6f)", st.peak);

        float bad[2] = { NAN, INFINITY }, out[2];
        e.processFrame (bad, out, 2);
        auto st2 = run (e, makeGuitar (1.0, 0.5, 0.3f));
        CHECK (st2.finite, "NaN/Inf input does not poison the engine");

        bool ok = true;
        for (int mask = 0; mask < 512; ++mask)
        {
            WahEngine x; x.prepare (kSr, 2);
            EngineParams q;
            q.mode = (WahMode) (mask % 4);
            q.position = (mask & 4) ? 1.f : 0.f; q.sensitivity = (mask & 8) ? 1.f : 0.f;
            q.depth = (mask & 16) ? 1.f : 0.f;   q.resonance = (mask & 32) ? 1.f : 0.f;
            q.drive = (mask & 64) ? 1.f : 0.f;   q.tone = (mask & 128) ? 1.f : 0.f;
            q.speed = (mask & 256) ? 1.f : 0.f;  q.aventura = (mask & 2) != 0;
            x.setParams (q);
            auto s = run (x, makeGuitar (0.8, 0.4, 0.9f));
            if (! s.finite || s.peak > 8.0f) { ok = false; std::printf ("  extreme setting %d: peak %.2f\n", mask, s.peak); }
        }
        CHECK (ok, "512 extreme parameter combinations stay finite and bounded");
    }

    std::printf ("== Factory presets ==\n");
    {
        using namespace cbaw::presets;
        bool valid = true;
        for (const auto& p : kFactoryPresets)
        {
            auto in01 = [] (float v) { return v >= 0.f && v <= 1.f; };
            if (! (p.mode >= 0 && p.mode <= 3 && in01 (p.position) && in01 (p.sensitivity) && in01 (p.depth) && in01 (p.resonance)
                   && in01 (p.drive) && in01 (p.tone) && in01 (p.speed) && in01 (p.mix) && p.outputDb >= -24.f && p.outputDb <= 12.f))
                valid = false;
        }
        CHECK (valid, "all %d presets have valid parameter values", kNumFactoryPresets);

        bool distinct = true;
        for (int a = 0; a < kNumFactoryPresets; ++a)
            for (int b = a + 1; b < kNumFactoryPresets; ++b)
                if (std::memcmp (&kFactoryPresets[a].position, &kFactoryPresets[b].position, sizeof (float) * 8) == 0
                    && kFactoryPresets[a].mode == kFactoryPresets[b].mode) distinct = false;
        CHECK (distinct, "no two presets share identical settings");

        // Presets really change the audio.
        double rmsA = 0, rmsB = 0;
        for (int which = 0; which < 2; ++which)
        {
            const auto& pv = kFactoryPresets[which == 0 ? 3 : 6]; // CRY BABY ORIGINAL vs DARK BACHATA
            WahEngine e; e.prepare (kSr, 2);
            EngineParams q; q.mode = (WahMode) pv.mode; q.position = pv.position; q.sensitivity = pv.sensitivity;
            q.depth = pv.depth; q.resonance = pv.resonance; q.drive = pv.drive; q.tone = pv.tone; q.speed = pv.speed; q.aventura = pv.aventura;
            e.setParams (q);
            double acc = 0; auto sig = makeGuitar (3.0, 0.5, 0.3f);
            for (float x : sig) { float i2[2] = { x, x }, o[2]; e.processFrame (i2, o, 2); acc += (double) o[0] * o[0]; }
            (which == 0 ? rmsA : rmsB) = std::sqrt (acc / (double) sig.size());
        }
        CHECK (std::fabs (rmsA - rmsB) > 1e-3, "different presets give different audio (rms %.4f vs %.4f)", rmsA, rmsB);
    }

    std::printf ("== Performance ==\n");
    {
        WahEngine e; e.prepare (kSr, 2);
        EngineParams p; p.mode = WahMode::Auto; e.setParams (p);
        auto g = makeGuitar (10.0, 0.5, 0.3f);
        const auto t0 = std::chrono::steady_clock::now();
        run (e, g);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("  10 s of stereo audio processed in %.3f s (%.1fx real time, unoptimised timing)\n", secs, 10.0 / secs);
        CHECK (secs < 5.0, "runs faster than real time");
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "SELF TEST FAILED" : "SELF TEST PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
