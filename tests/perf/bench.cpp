// FROZEN - DO NOT MODIFY. Performance benchmark (T-PERF-1): fraction of one core used by each effect at 48 kHz, 64-sample blocks.
// usage: muff_bench [--limit <percent>] [--seconds <audio seconds per effect>]
// Prints one line per effect: "<slug> <percent of real time>". Exit 0 if every effect is under the limit
// (default 5.0), 1 otherwise, 2 on a usage error, 3 if the core throws (for example the not-implemented stubs).
// With --limit 0 the limit is not enforced (measure only).
#include "muff/adapters.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <vector>

static int run (int argc, char** argv) {
    double limit = 5.0, seconds = 20.0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp (argv[i], "--limit") == 0 && i + 1 < argc) limit = std::atof (argv[++i]);
        else if (std::strcmp (argv[i], "--seconds") == 0 && i + 1 < argc) seconds = std::atof (argv[++i]);
        else { std::fprintf (stderr, "usage: muff_bench [--limit <percent>] [--seconds <s>]\n"); return 2; }
    }
    if (! (seconds > 0.0) || limit < 0.0) { std::fprintf (stderr, "bad argument\n"); return 2; }
    const double fs = 48000.0; const int block = 64;
    const long blocks = (long) (seconds * fs / block);
    bool ok = true;
    for (int id = 0; id < muff::kNumEffects; ++id) {
        muff::BlockRunner br ((muff::EffectId) id);
        br.prepare (fs);
        muff::Effect& fx = br.effect(); const muff::EffectInfo& e = fx.info();
        // worst-case-leaning settings: modulation and feedback at mid travel, 12 stages, animated pattern
        for (const char* pid : { "resonance", "lfo_amount", "feedback", "env_amount", "fm_amount" }) { const int p = muff::findParam (e, pid); if (p >= 0) fx.setParam (p, 0.5f * (e.params[p].min + e.params[p].max)); }
        for (const char* pid : { "stages", "pattern", "lfo", "sync" }) { const int p = muff::findParam (e, pid); if (p >= 0) fx.setParam (p, e.params[p].max); }
        std::vector<float> l ((size_t) block), r ((size_t) block);
        double best = 1.0e30; long n = 0;
        for (int rep = 0; rep < 3; ++rep) {     // best of three passes
            const auto t0 = std::chrono::steady_clock::now();
            for (long b = 0; b < blocks; ++b) {
                for (int i = 0; i < block; ++i, ++n) l[(size_t) i] = r[(size_t) i] = (float) (0.3 * std::sin (6.283185307 * 220.0 * (double) n / fs));
                const float* ins[2] = { l.data(), r.data() }; float* outs[2] = { l.data(), r.data() };
                br.process (ins, 2, outs, 2, block);
            }
            best = std::min (best, std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
        }
        const double percent = 100.0 * best / (blocks * block / fs);
        std::printf ("%s %.3f\n", e.slug, percent);
        if (limit > 0.0 && percent >= limit) ok = false;
    }
    return ok ? 0 : 1;
}

int main (int argc, char** argv) {
    try { return run (argc, argv); }
    catch (const std::exception& e) { std::fprintf (stderr, "error: %s\n", e.what()); return 3; }
}
