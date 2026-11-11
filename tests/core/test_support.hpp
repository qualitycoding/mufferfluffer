// FROZEN - DO NOT MODIFY. Shared helpers of the frozen core tests.
#pragma once

#include "muff/adapters.hpp"
#include "muff/blocks.hpp"
#include "muff/core.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace ts {

constexpr double kPi = 3.14159265358979323846;
using Signal = std::vector<float>;
using Source = std::function<float (long)>;   // sample index -> value

inline double dB (double ratio) { return 20.0 * std::log10 (std::max (ratio, 1.0e-12)); }
inline Source sine (double freq, double fs, double amp, double phase = 0.0) {
    return [=] (long n) { return (float) (amp * std::sin (2.0 * kPi * freq * (double) n / fs + phase)); };
}
inline Source constant (float v) { return [=] (long) { return v; }; }
inline Source impulseAt (long at, float amp) { return [=] (long n) { return n == at ? amp : 0.0f; }; }
// sine burst from sample `from` (inclusive) to `to` (exclusive)
inline Source burst (double freq, double fs, double amp, long from, long to) {
    return [=] (long n) { return (n >= from && n < to) ? (float) (amp * std::sin (2.0 * kPi * freq * (double) (n - from) / fs)) : 0.0f; };
}
// one-sample-wide trigger pulses of 1.0 CV unit (5 V), each held for `width` samples
inline Source pulsesAt (std::vector<long> at, long width) {
    return [=] (long n) { for (long a : at) if (n >= a && n < a + width) return 1.0f; return 0.0f; };
}

struct Rng {    // deterministic test noise
    std::uint32_t s;
    explicit Rng (std::uint32_t seed) : s (seed) {}
    std::uint32_t next() { s = s * 1664525u + 1013904223u; return s; }
    float uni() { return (float) ((next() >> 8) * (1.0 / 16777216.0)); }             // [0, 1)
    float bip() { return 2.0f * uni() - 1.0f; }                                       // [-1, 1)
};

struct Rig {
    std::unique_ptr<muff::Effect> fx;
    const muff::EffectInfo* info = nullptr;
    double fs = 0.0;
    long clock = 0;     // samples processed so far (sources see a running index)

    Rig (muff::EffectId id, double sampleRate) : fx (muff::createEffect (id)), fs (sampleRate) {
        info = &fx->info();
        fx->prepare (fs);
    }
    int param (const char* id) const { const int i = muff::findParam (*info, id); REQUIRE (i >= 0); return i; }
    int in (const char* id) const { const int i = muff::findInput (*info, id); REQUIRE (i >= 0); return i; }
    int out (const char* id) const { const int i = muff::findOutput (*info, id); REQUIRE (i >= 0); return i; }
    int meter (const char* id) const { const int i = muff::findMeter (*info, id); REQUIRE (i >= 0); return i; }
    void set (const char* id, float v) { fx->setParam (param (id), v); }
    // sets a continuous parameter by its normalised knob position
    void setNorm (const char* id, float n) { const int p = param (id); fx->setParam (p, muff::fromNormalised (info->params[p], n)); }
    void connect (const char* inputId, bool c = true) { fx->setInputConnected (in (inputId), c); }

    // Processes n frames. `src` maps input index -> source (missing inputs read 0). Returns one Signal per output.
    std::vector<Signal> run (long n, const std::map<int, Source>& src = {}) {
        std::vector<Signal> outs ((size_t) info->numOutputs, Signal ((size_t) n));
        float x[muff::kMaxPorts], y[muff::kMaxPorts];
        for (long i = 0; i < n; ++i) {
            for (int k = 0; k < info->numInputs; ++k) x[k] = 0.0f;
            for (const auto& kv : src) x[kv.first] = kv.second (clock);
            fx->process (x, y);
            for (int o = 0; o < info->numOutputs; ++o) outs[(size_t) o][(size_t) i] = y[o];
            ++clock;
        }
        return outs;
    }
    long samples (double seconds) const { return (long) std::llround (seconds * fs); }
};

inline double rms (const Signal& x, long from, long to) {
    double s = 0.0; for (long i = from; i < to; ++i) s += (double) x[(size_t) i] * x[(size_t) i];
    return std::sqrt (s / (double) std::max (1L, to - from));
}
inline double peak (const Signal& x, long from, long to) {
    double p = 0.0; for (long i = from; i < to; ++i) p = std::max (p, (double) std::fabs (x[(size_t) i]));
    return p;
}
inline long argPeak (const Signal& x, long from, long to) {
    long at = from; double p = -1.0;
    for (long i = from; i < to; ++i) if (std::fabs (x[(size_t) i]) > p) { p = std::fabs (x[(size_t) i]); at = i; }
    return at;
}
inline bool allFinite (const Signal& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }

// Amplitude of the sinusoidal component at `freq` in x[from, to), Hann-windowed.
inline double toneAmp (const Signal& x, long from, long to, double freq, double fs) {
    double re = 0.0, im = 0.0, wsum = 0.0;
    const long n = to - from;
    for (long i = 0; i < n; ++i) {
        const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) (n - 1));
        const double ph = 2.0 * kPi * freq * (double) i / fs;
        re += w * x[(size_t) (from + i)] * std::cos (ph); im -= w * x[(size_t) (from + i)] * std::sin (ph); wsum += w;
    }
    return 2.0 * std::sqrt (re * re + im * im) / wsum;
}
// Complex-free magnitude of the transfer function at `freq` from an impulse response h (impulse amplitude amp), rectangular window.
inline double irGain (const Signal& h, double freq, double fs, double amp) {
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < h.size(); ++i) { const double ph = 2.0 * kPi * freq * (double) i / fs; re += h[i] * std::cos (ph); im -= h[i] * std::sin (ph); }
    return std::sqrt (re * re + im * im) / amp;
}
// Mean frequency from rising zero crossings (linear interpolation) in x[from, to); 0 if fewer than two crossings.
inline double zcFreq (const Signal& x, long from, long to, double fs) {
    double first = -1.0, last = -1.0; long count = 0;
    for (long i = from + 1; i < to; ++i) {
        const float a = x[(size_t) (i - 1)], b = x[(size_t) i];
        if (a < 0.0f && b >= 0.0f) { const double t = (double) (i - 1) + (double) (-a) / (double) (b - a); if (first < 0.0) first = t; last = t; ++count; }
    }
    return count < 2 ? 0.0 : (double) (count - 1) * fs / (last - first);
}
// Intervals (in samples) between successive rising zero crossings in x[from, to).
inline std::vector<double> zcIntervals (const Signal& x, long from, long to) {
    std::vector<double> t, d;
    for (long i = from + 1; i < to; ++i) {
        const float a = x[(size_t) (i - 1)], b = x[(size_t) i];
        if (a < 0.0f && b >= 0.0f) t.push_back ((double) (i - 1) + (double) (-a) / (double) (b - a));
    }
    for (size_t i = 1; i < t.size(); ++i) d.push_back (t[i] - t[i - 1]);
    return d;
}
// Frequency in [lo, hi] (searched on `steps` log-spaced points, then refined) at which an impulse response has its largest gain.
inline double irPeakFreq (const Signal& h, double lo, double hi, double fs, int steps = 400) {
    double best = lo, bg = -1.0;
    for (int i = 0; i <= steps; ++i) { const double f = lo * std::pow (hi / lo, (double) i / steps); const double g = irGain (h, f, fs, 1.0); if (g > bg) { bg = g; best = f; } }
    const double r = std::pow (hi / lo, 1.0 / steps);
    double a = best / r, b = best * r;
    for (int i = 0; i <= 200; ++i) { const double f = a + (b - a) * i / 200.0; const double g = irGain (h, f, fs, 1.0); if (g > bg) { bg = g; best = f; } }
    return best;
}

inline const muff::EffectId kAllEffects[7] = { muff::EffectId::Lowpass, muff::EffectId::RingMod, muff::EffectId::Phaser, muff::EffectId::Delay,
                                               muff::EffectId::FilterBank, muff::EffectId::OscBox, muff::EffectId::Chorus };
} // namespace ts
