// FROZEN - DO NOT MODIFY. Tests common to every effect (T-001 .. T-019) and the building blocks (T-020 .. T-026).
#include "test_support.hpp"

#include <cstdlib>
#include <limits>
#include <new>

using namespace muff;
using Catch::Approx;

namespace {
#include "expected_effects.inc"

// ---- allocation counter for T-011 (counts every global operator new while armed)
long gAllocCount = 0;
bool gAllocArmed = false;
}
// Sanitizer builds define MUFF_NO_ALLOC_HOOK: their runtime owns operator new, so T-011 is compiled out there.
#ifndef MUFF_NO_ALLOC_HOOK
void* operator new (std::size_t n) { if (gAllocArmed) ++gAllocCount; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (std::size_t n) { if (gAllocArmed) ++gAllocCount; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
#endif

TEST_CASE ("T-001 effect tables equal the frozen fixture", "[common][tables]") {
    for (const XEffect& x : kExpected) {
        const EffectInfo& e = effectInfo ((EffectId) x.id);
        INFO ("effect " << x.slug);
        REQUIRE ((int) e.id == x.id);
        REQUIRE (std::strcmp (e.slug, x.slug) == 0);
        REQUIRE (std::strcmp (e.name, x.name) == 0);
        REQUIRE (e.numParams == x.numParams);
        for (int p = 0; p < x.numParams; ++p) {
            const ParamSpec& a = e.params[p]; const XParam& b = x.params[p];
            INFO ("param " << b.id);
            REQUIRE (std::strcmp (a.id, b.id) == 0);
            REQUIRE (std::strcmp (a.name, b.name) == 0);
            REQUIRE (std::strcmp (a.unit, b.unit) == 0);
            REQUIRE (a.min == b.min); REQUIRE (a.max == b.max); REQUIRE (a.def == b.def);
            REQUIRE ((int) a.taper == b.taper);
            REQUIRE (a.numSteps == b.numSteps);
            for (int s = 0; s < b.numSteps; ++s) REQUIRE (std::strcmp (a.stepNames[s], b.steps[s]) == 0);
            if (b.numSteps == 0) REQUIRE (a.stepNames == nullptr);
        }
        REQUIRE (e.numInputs == x.numInputs);
        REQUIRE (e.numOutputs == x.numOutputs);
        REQUIRE (e.numInputs <= kMaxPorts); REQUIRE (e.numOutputs <= kMaxPorts);
        for (int k = 0; k < 2; ++k) {
            const int n = k == 0 ? x.numInputs : x.numOutputs;
            for (int i = 0; i < n; ++i) {
                const PortSpec& a = k == 0 ? e.inputs[i] : e.outputs[i]; const XPort& b = k == 0 ? x.inputs[i] : x.outputs[i];
                INFO ("port " << b.id);
                REQUIRE (std::strcmp (a.id, b.id) == 0);
                REQUIRE (std::strcmp (a.name, b.name) == 0);
                REQUIRE ((int) a.kind == b.kind);
                REQUIRE (a.param == (b.param ? findParam (e, b.param) : -1));
                REQUIRE (a.depth == b.depth);
            }
        }
        REQUIRE (std::strcmp (e.inputs[0].id, "in") == 0);
        REQUIRE (e.numMeters == x.numMeters);
        for (int m = 0; m < x.numMeters; ++m) REQUIRE (std::strcmp (e.meters[m].id, x.meters[m]) == 0);
        REQUIRE (e.rightOutput == (x.rightOutput ? findOutput (e, x.rightOutput) : -1));
        REQUIRE (e.monoSumsRight == x.monoSumsRight);
    }
}

TEST_CASE ("T-002 table lookups reject unknown ids", "[common][tables]") {
    REQUIRE_THROWS_AS (effectInfo ((EffectId) 7), std::out_of_range);
    REQUIRE_THROWS_AS (effectInfo ((EffectId) -1), std::out_of_range);
    REQUIRE_THROWS_AS (createEffect ((EffectId) 99), std::out_of_range);
    const EffectInfo& e = effectInfo (EffectId::Lowpass);
    REQUIRE (findParam (e, "cutoff") == 1);
    REQUIRE (findParam (e, "nope") == -1);
    REQUIRE (findParam (e, nullptr) == -1);
    REQUIRE (findInput (e, "cv_cutoff") == 1);
    REQUIRE (findInput (e, "") == -1);
    REQUIRE (findOutput (e, "env_out") == 1);
    REQUIRE (findOutput (e, nullptr) == -1);
    REQUIRE (findMeter (e, "level") == 0);
    REQUIRE (findMeter (e, "x") == -1);
}

TEST_CASE ("T-003 normalisation maps follow the three tapers", "[common][params]") {
    const EffectInfo& e = effectInfo (EffectId::Lowpass);
    const ParamSpec& cut = e.params[findParam (e, "cutoff")];        // log 15..12000
    const ParamSpec& res = e.params[findParam (e, "resonance")];     // linear 0..10
    const ParamSpec& poles = e.params[findParam (e, "poles")];       // stepped, 2 steps
    const ParamSpec& wave = effectInfo (EffectId::Delay).params[findParam (effectInfo (EffectId::Delay), "lfo_wave")]; // 6 steps
    REQUIRE (toNormalised (cut, 15.0f) == Approx (0.0).margin (1e-6));
    REQUIRE (toNormalised (cut, 12000.0f) == Approx (1.0).margin (1e-6));
    REQUIRE (fromNormalised (cut, 0.5f) == Approx (std::sqrt (15.0 * 12000.0)).epsilon (1e-4));
    REQUIRE (toNormalised (cut, 424.2641f) == Approx (0.5).margin (1e-4));
    REQUIRE (fromNormalised (res, 0.25f) == Approx (2.5).margin (1e-5));
    REQUIRE (toNormalised (res, 7.5f) == Approx (0.75).margin (1e-6));
    REQUIRE (toNormalised (res, 99.0f) == 1.0f);            // clamped
    REQUIRE (toNormalised (res, -3.0f) == 0.0f);
    REQUIRE (fromNormalised (res, 2.0f) == 10.0f);
    REQUIRE (fromNormalised (res, -1.0f) == 0.0f);
    REQUIRE (toNormalised (poles, 1.0f) == 1.0f);
    REQUIRE (fromNormalised (poles, 0.49f) == 0.0f);
    REQUIRE (fromNormalised (poles, 0.51f) == 1.0f);
    REQUIRE (fromNormalised (wave, 0.6f) == 3.0f);
    REQUIRE (toNormalised (wave, 2.4f) == Approx (0.4).margin (1e-6));   // rounds to step 2
    const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
    REQUIRE (toNormalised (res, nan) == Approx (toNormalised (res, res.def)).margin (1e-6));
    REQUIRE (toNormalised (cut, inf) == Approx (toNormalised (cut, cut.def)).margin (1e-6));
    REQUIRE (fromNormalised (cut, nan) == cut.def);
    for (const XEffect& x : kExpected) {        // round trip on every continuous parameter
        const EffectInfo& ei = effectInfo ((EffectId) x.id);
        for (int p = 0; p < ei.numParams; ++p) {
            const ParamSpec& s = ei.params[p];
            for (float n : { 0.0f, 0.1f, 0.37f, 0.5f, 0.93f, 1.0f }) {
                const float v = fromNormalised (s, n);
                REQUIRE (v >= s.min); REQUIRE (v <= s.max);
                if (s.taper != Taper::Stepped) REQUIRE (toNormalised (s, v) == Approx (n).margin (2e-4));
            }
        }
    }
}

TEST_CASE ("T-004 parameters start at defaults, clamp, round and ignore bad values", "[common][params]") {
    const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
    for (EffectId id : ts::kAllEffects) {
        auto fx = createEffect (id);
        const EffectInfo& e = fx->info();
        REQUIRE (&e == &effectInfo (id));
        for (int p = 0; p < e.numParams; ++p) {
            const ParamSpec& s = e.params[p];
            INFO (e.slug << "." << s.id);
            REQUIRE (fx->getParam (p) == s.def);
            fx->setParam (p, s.max + 1000.0f); REQUIRE (fx->getParam (p) == s.max);
            fx->setParam (p, s.min - 1000.0f); REQUIRE (fx->getParam (p) == s.min);
            fx->setParam (p, nan); REQUIRE (fx->getParam (p) == s.min);
            fx->setParam (p, inf); REQUIRE (fx->getParam (p) == s.min);
            fx->setParam (p, -inf); REQUIRE (fx->getParam (p) == s.min);
            if (s.taper == Taper::Stepped && s.numSteps > 1) { fx->setParam (p, 0.6f); REQUIRE (fx->getParam (p) == 1.0f); }
            else { const float mid = 0.5f * (s.min + s.max); fx->setParam (p, mid); REQUIRE (fx->getParam (p) == mid); }
        }
        fx->setParam (-1, 1.0f); fx->setParam (e.numParams, 1.0f);      // ignored, no crash
        REQUIRE (fx->getParam (-1) == 0.0f);
        REQUIRE (fx->getParam (e.numParams) == 0.0f);
        REQUIRE (fx->getMeter (-1) == 0.0f);
        REQUIRE (fx->getMeter (e.numMeters) == 0.0f);
        fx->setInputConnected (-1, true); fx->setInputConnected (e.numInputs, true);   // ignored, no crash
    }
}

TEST_CASE ("T-005 prepare validates the sample rate", "[common][lifecycle]") {
    const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
    for (EffectId id : ts::kAllEffects) {
        auto fx = createEffect (id);
        for (double bad : { 0.0, -48000.0, 7999.0, 768001.0, nan, inf }) REQUIRE_THROWS_AS (fx->prepare (bad), std::invalid_argument);
        REQUIRE_NOTHROW (fx->prepare (8000.0));
        REQUIRE_NOTHROW (fx->prepare (768000.0));
        REQUIRE_NOTHROW (fx->prepare (48000.0));    // repeated prepare is allowed
    }
}

TEST_CASE ("T-006 process before prepare writes zeros", "[common][lifecycle]") {
    for (EffectId id : ts::kAllEffects) {
        auto fx = createEffect (id);
        float x[kMaxPorts], y[kMaxPorts];
        for (int i = 0; i < kMaxPorts; ++i) { x[i] = 0.7f; y[i] = 123.0f; }
        fx->process (x, y);
        for (int o = 0; o < fx->info().numOutputs; ++o) REQUIRE (y[o] == 0.0f);
    }
}

TEST_CASE ("T-007 outputs stay finite and bounded for any parameters, inputs and sample rate", "[common][robustness][security]") {
    const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
    for (EffectId id : ts::kAllEffects) {
        for (double fs : { 8000.0, 44100.0, 48000.0, 96000.0, 192000.0, 768000.0 }) {
            auto fx = createEffect (id);
            fx->prepare (fs);
            const EffectInfo& e = fx->info();
            INFO (e.slug << " at " << fs);
            ts::Rng rng (0xC0FFEEu + (std::uint32_t) id * 7919u + (std::uint32_t) fs);
            for (int i = 0; i < e.numInputs; ++i) fx->setInputConnected (i, true);
            float x[kMaxPorts], y[kMaxPorts];
            bool ok = true;
            const long total = fs > 200000.0 ? 60000 : 40000;
            for (long n = 0; n < total && ok; ++n) {
                if (n % 2000 == 0) {    // new random parameter set; every fourth set is an extreme corner
                    const long set = n / 2000;
                    for (int p = 0; p < e.numParams; ++p) {
                        const ParamSpec& s = e.params[p];
                        float v = s.min + rng.uni() * (s.max - s.min);
                        if (set % 4 == 1) v = s.max; else if (set % 4 == 3) v = s.min;
                        fx->setParam (p, v);
                    }
                    if (set % 5 == 4) fx->reset();
                }
                for (int i = 0; i < e.numInputs; ++i) {
                    const std::uint32_t r = rng.next() >> 16;
                    float v = rng.bip() * (i == 0 ? 2.0f : 1.5f);
                    if (r % 997 == 0) v = nan; else if (r % 991 == 0) v = inf; else if (r % 983 == 0) v = -inf; else if (r % 977 == 0) v = 1.0e30f; else if (r % 971 == 0) v = -1.0e30f;
                    x[i] = v;
                }
                fx->process (x, y);
                for (int o = 0; o < e.numOutputs; ++o) ok = ok && std::isfinite (y[o]) && std::fabs (y[o]) <= kOutputLimit;
                for (int m = 0; m < e.numMeters; ++m) { const float mv = fx->getMeter (m); ok = ok && mv >= 0.0f && mv <= 1.0f; }
            }
            REQUIRE (ok);
        }
    }
}

TEST_CASE ("T-008 processing is deterministic and reset restores the initial audio state", "[common][lifecycle]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig a (id, 48000.0), b (id, 48000.0);
        INFO (a.info->slug);
        const auto src = std::map<int, ts::Source> { { 0, ts::sine (220.0, 48000.0, 0.5) } };
        const auto ya = a.run (6000, src), yb = b.run (6000, src);
        REQUIRE (ya == yb);
        a.fx->reset(); a.clock = 0;
        const auto yc = a.run (6000, src);
        REQUIRE (yc == ya);
        a.fx->prepare (48000.0); a.clock = 0;       // prepare implies reset
        REQUIRE (a.run (6000, src) == ya);
    }
}

TEST_CASE ("T-009 silence in gives silence at the main output with default settings", "[common]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig r (id, 48000.0);
        INFO (r.info->slug);
        const auto y = r.run (48000);
        REQUIRE (ts::peak (y[0], 0, 48000) < 1.0e-3);
        if (r.info->rightOutput >= 0) REQUIRE (ts::peak (y[(size_t) r.info->rightOutput], 0, 48000) < 1.0e-3);
    }
}

TEST_CASE ("T-010 bypass passes the input: drive stays in the path of Lowpass and RingMod only", "[common][bypass]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig r (id, 48000.0);
        INFO (r.info->slug);
        r.set ("active", 0.0f);
        r.set ("drive", 12.0f);
        r.fx->reset();
        const auto y = r.run (9600, { { 0, ts::sine (300.0, 48000.0, 0.2) } });
        const bool driveInBypass = (id == EffectId::Lowpass || id == EffectId::RingMod);
        double worst = 0.0;
        for (long n = 2400; n < 9600; ++n) {
            const float in = (float) (0.2 * std::sin (2.0 * ts::kPi * 300.0 * (double) n / 48000.0));
            const float want = driveInBypass ? 2.0f * std::tanh (0.5f * in * std::pow (10.0f, 12.0f / 20.0f)) : in;
            worst = std::max (worst, (double) std::fabs (y[0][(size_t) n] - want));
        }
        REQUIRE (worst < 2.0e-3);
        REQUIRE (r.fx->getMeter (r.meter ("active")) < 0.01f);
        if (id == EffectId::FilterBank) REQUIRE (ts::peak (y[(size_t) r.out ("out_r")], 2400, 9600) < 1.0e-4);   // nothing at the right output
        if (id == EffectId::Phaser) REQUIRE (ts::rms (y[(size_t) r.out ("aux_out")], 2400, 9600) == Approx (0.2 / std::sqrt (2.0)).epsilon (0.02));
        if (id == EffectId::Chorus) REQUIRE (ts::rms (y[(size_t) r.out ("out_r")], 2400, 9600) == Approx (0.2 / std::sqrt (2.0)).epsilon (0.02));
    }
}

#ifndef MUFF_NO_ALLOC_HOOK
TEST_CASE ("T-011 the audio path performs no heap allocation", "[common][realtime]") {
    for (EffectId id : ts::kAllEffects) {
        auto fx = createEffect (id);
        fx->prepare (48000.0);
        const EffectInfo& e = fx->info();
        for (int i = 0; i < e.numInputs; ++i) fx->setInputConnected (i, true);
        float x[kMaxPorts] {}, y[kMaxPorts] {};
        ts::Rng rng (42u + (std::uint32_t) id);
        gAllocCount = 0; gAllocArmed = true;
        for (long n = 0; n < 20000; ++n) {
            if (n % 500 == 0) for (int p = 0; p < e.numParams; ++p) fx->setParam (p, e.params[p].min + rng.uni() * (e.params[p].max - e.params[p].min));
            for (int i = 0; i < e.numInputs; ++i) x[i] = rng.bip();
            fx->process (x, y);
            (void) fx->getMeter (0); (void) fx->getParam (0); (void) fx->latencySamples(); (void) fx->tailSeconds();
            if (n == 10000) fx->reset();
            if (n % 3000 == 0) fx->setInputConnected (e.numInputs - 1, (n / 3000) % 2 == 0);
        }
        gAllocArmed = false;
        INFO (e.slug);
        REQUIRE (gAllocCount == 0);
    }
}

#endif // MUFF_NO_ALLOC_HOOK

TEST_CASE ("T-012 a normalised CV input sweeps depth x CV of the knob range and is clamped", "[common][cv]") {
    // Lowpass cutoff: knob at minimum (15 Hz) plus 0.5 CV units (2.5 V) must equal the knob at half travel (424.26 Hz).
    auto gainAt = [] (float cutoffNorm, float cv, double freq) {
        ts::Rig r (EffectId::Lowpass, 48000.0);
        r.setNorm ("cutoff", cutoffNorm); r.connect ("cv_cutoff"); r.fx->reset();
        const auto y = r.run (48000, { { 0, ts::sine (freq, 48000.0, 0.05) }, { r.in ("cv_cutoff"), ts::constant (cv) } });
        return ts::toneAmp (y[0], 24000, 48000, freq, 48000.0) / 0.05;
    };
    const double viaCv = gainAt (0.0f, 0.5f, 424.2641), viaKnob = gainAt (0.5f, 0.0f, 424.2641);
    REQUIRE (ts::dB (viaKnob) == Approx (-12.04).margin (0.5));
    REQUIRE (ts::dB (viaCv) == Approx (ts::dB (viaKnob)).margin (0.2));
    REQUIRE (ts::dB (gainAt (0.5f, -0.5f, 100.0)) < -40.0);                 // negative CV lowers the cutoff to 15 Hz
    REQUIRE (ts::dB (gainAt (0.9f, 5.0f, 5000.0)) == Approx (ts::dB (gainAt (1.0f, 0.0f, 5000.0))).margin (0.2));   // clamped at the top
    // OscBox wave CV has depth 0.6: knob 0 with CV 1.0 equals knob 0.6
    auto oscOut = [] (float waveNorm, float cv) {
        ts::Rig r (EffectId::OscBox, 48000.0);
        r.set ("wave", waveNorm); r.connect ("cv_wave"); r.fx->reset();
        return r.run (4800, { { r.in ("cv_wave"), ts::constant (cv) } })[(size_t) r.out ("osc_out")];
    };
    const ts::Signal a = oscOut (0.0f, 1.0f), b = oscOut (0.6f, 0.0f);
    double diff = 0.0; for (size_t i = 2400; i < a.size(); ++i) diff   // after the 5 ms smoothing has settled
         = std::max (diff, (double) std::fabs (a[i] - b[i]));
    REQUIRE (diff < 1.0e-4);
}

TEST_CASE ("T-013 a disconnected input reads as zero", "[common][cv]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig a (id, 48000.0), b (id, 48000.0);
        INFO (a.info->slug);
        std::map<int, ts::Source> clean { { 0, ts::sine (330.0, 48000.0, 0.3) } }, dirty = clean;
        for (int i = 1; i < a.info->numInputs; ++i) dirty[i] = ts::sine (3.0 + i, 48000.0, 0.9);    // not connected: must be ignored
        REQUIRE (a.run (9600, clean) == b.run (9600, dirty));
        // input 0 can be disconnected too
        ts::Rig c (id, 48000.0), d (id, 48000.0);
        c.fx->setInputConnected (0, false);
        REQUIRE (c.run (4800, clean) == d.run (4800));
    }
}

TEST_CASE ("T-014 a parameter step is smoothed: no click", "[common][smoothing]") {
    ts::Rig r (EffectId::Phaser, 48000.0);
    r.set ("lfo_amount", 0.0f); r.fx->reset();
    r.run (4800, { { 0, ts::constant (0.5f) } });      // DC passes an all-pass chain unchanged
    r.set ("output", -24.0f);
    const auto y = r.run (4800, { { 0, ts::constant (0.5f) } })[0];
    const double start = 2.0 * std::tanh (0.25), end = start * std::pow (10.0, -24.0 / 20.0);
    double maxJump = std::fabs (y[0] - start);
    for (size_t i = 1; i < y.size(); ++i) maxJump = std::max (maxJump, (double) std::fabs (y[i] - y[i - 1]));
    REQUIRE (maxJump < 0.1 * (start - end));
    REQUIRE (y[(size_t) r.samples (0.05)] == Approx (end).margin (0.01 * start));
}

TEST_CASE ("T-015 meters report level and state", "[common][meters]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig r (id, 48000.0);
        INFO (r.info->slug);
        REQUIRE (r.fx->getMeter (r.meter ("level")) == 0.0f);
        r.run (4800, { { 0, ts::sine (500.0, 48000.0, 0.5) } });
        const float lvl = r.fx->getMeter (r.meter ("level"));
        REQUIRE (lvl > 0.4f); REQUIRE (lvl <= 0.5f);            // peak of the post-drive signal at 0 dB drive: 2*tanh(0.25) = 0.49
        REQUIRE (r.fx->getMeter (r.meter ("active")) > 0.99f);
        r.run (48000);
        REQUIRE (r.fx->getMeter (r.meter ("level")) < 0.01f);  // falls back within one second
    }
}

TEST_CASE ("T-016 reported latency matches the dry path", "[common][latency]") {
    for (EffectId id : ts::kAllEffects) {
        ts::Rig r (id, 48000.0);
        INFO (r.info->slug);
        const int lat = r.fx->latencySamples();
        REQUIRE (lat >= 0); REQUIRE (lat <= 64);
        if (muff::findParam (*r.info, "mix") >= 0) r.set ("mix", 0.0f);
        else continue;      // Phaser has no dry-only setting
        r.fx->reset();
        const auto y = r.run (1024, { { 0, ts::impulseAt (100, 0.1f) } })[0];
        REQUIRE (ts::argPeak (y, 0, 1024) == 100 + lat);
        REQUIRE (r.fx->latencySamples() == lat);
    }
}

TEST_CASE ("T-017 tailSeconds is an upper bound and reports self-oscillation", "[common][tail]") {
    {
        ts::Rig r (EffectId::Lowpass, 48000.0);
        r.set ("resonance", 7.9f); REQUIRE (r.fx->tailSeconds() < 1.0e8f);
        r.set ("resonance", 8.0f); REQUIRE (r.fx->tailSeconds() >= 1.0e9f);
    }
    {
        ts::Rig r (EffectId::Chorus, 48000.0);
        r.set ("feedback", -0.8f); REQUIRE (r.fx->tailSeconds() >= 1.0e9f);
        r.set ("feedback", 0.5f); REQUIRE (r.fx->tailSeconds() < 1.0e8f);
    }
    ts::Rig r (EffectId::Delay, 48000.0);
    r.set ("feedback", 8.0f); REQUIRE (r.fx->tailSeconds() >= 1.0e9f);
    r.set ("feedback", 7.99f); REQUIRE (r.fx->tailSeconds() >= 1.0e9f);     // a bound above 60 s is reported as "for ever"
    for (float fb = 0.0f; fb < 8.0f; fb += 0.37f) { r.set ("feedback", fb); const float t = r.fx->tailSeconds(); REQUIRE ((t <= 60.0f || t >= 1.0e9f)); }
    r.set ("feedback", 4.0f); r.set ("time", 1.0f); r.set ("mix", 1.0f); r.fx->reset();     // 400 ms, loop gain 0.5
    const float tail = r.fx->tailSeconds();
    REQUIRE (tail > 0.4f); REQUIRE (tail < 30.0f);
    const long stop = r.samples (0.1), n = stop + r.samples (tail) + r.samples (0.5);
    const auto y = r.run (n, { { 0, ts::burst (500.0, 48000.0, 0.5, 0, stop) } })[0];
    REQUIRE (ts::peak (y, stop, stop + r.samples (0.5)) > 0.05);                 // there was an echo
    REQUIRE (ts::peak (y, stop + r.samples (tail), n) < 1.0e-3);                 // below -60 dB after the bound
}

TEST_CASE ("T-018 behaviour is the same at 44.1 kHz and 192 kHz", "[common][samplerate]") {
    // A sample-rate-independent observable per effect family: the envelope output of the Lowpass and the echo time of the Delay.
    for (double fs : { 44100.0, 192000.0 }) {
        ts::Rig d (EffectId::Delay, fs);
        d.set ("time", 0.5f); d.set ("mix", 1.0f); d.set ("feedback", 0.0f); d.fx->reset();
        const long at = d.samples (0.05);
        const auto y = d.run (d.samples (0.4), { { 0, ts::burst (1000.0, fs, 0.5, at, at + d.samples (0.002)) } })[(size_t) d.out ("delay_out")];
        const double echo = (double) (ts::argPeak (y, at, (long) y.size()) - at) / fs;
        REQUIRE (echo == Approx (0.04 * std::sqrt (10.0) + 0.001).epsilon (0.03));
        ts::Rig l (EffectId::Lowpass, fs);
        l.set ("resonance", 10.0f); l.set ("cutoff", 2000.0f); l.fx->reset();
        const auto o = l.run (l.samples (2.0))[0];
        REQUIRE (ts::zcFreq (o, l.samples (1.5), l.samples (2.0), fs) == Approx (2000.0).epsilon (0.02));
    }
}

// ------------------------------------------------------------------ building blocks

TEST_CASE ("T-020 softClip is 2*tanh(x/2)", "[blocks]") {
    for (float x : { -100.0f, -3.0f, -0.5f, 0.0f, 0.01f, 0.5f, 1.0f, 4.0f, 100.0f })
        REQUIRE (softClip (x) == Approx (2.0 * std::tanh (0.5 * (double) x)).margin (1e-6));
    REQUIRE (softClip (1.0e-3f) == Approx (1.0e-3).epsilon (1e-4));     // unity slope
    REQUIRE (std::fabs (softClip (1.0e6f)) <= 2.0f);
}

TEST_CASE ("T-021 lfoShape produces the six documented shapes", "[blocks][lfo]") {
    REQUIRE (lfoShape (LfoWave::Sine, 0.25f, 0.0f) == Approx (1.0).margin (1e-5));
    REQUIRE (lfoShape (LfoWave::Sine, 0.75f, 0.0f) == Approx (-1.0).margin (1e-5));
    REQUIRE (lfoShape (LfoWave::Sine, 0.0f, 0.0f) == Approx (0.0).margin (1e-5));
    REQUIRE (lfoShape (LfoWave::Triangle, 0.0f, 0.0f) == Approx (-1.0).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Triangle, 0.5f, 0.0f) == Approx (1.0).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Triangle, 0.25f, 0.0f) == Approx (0.0).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Triangle, 0.875f, 0.0f) == Approx (-0.5).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Square, 0.1f, 0.0f) == 1.0f);
    REQUIRE (lfoShape (LfoWave::Square, 0.5f, 0.0f) == -1.0f);
    REQUIRE (lfoShape (LfoWave::Square, 0.9f, 0.0f) == -1.0f);
    REQUIRE (lfoShape (LfoWave::Sawtooth, 0.0f, 0.0f) == Approx (-1.0).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Sawtooth, 0.75f, 0.0f) == Approx (0.5).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Ramp, 0.0f, 0.0f) == Approx (1.0).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::Ramp, 0.75f, 0.0f) == Approx (-0.5).margin (1e-6));
    REQUIRE (lfoShape (LfoWave::RandomStep, 0.3f, 0.42f) == 0.42f);
    REQUIRE (lfoShape (LfoWave::RandomStep, 0.9f, -0.7f) == -0.7f);
    REQUIRE (lfoShape (LfoWave::Sawtooth, 1.25f, 0.0f) == Approx (-0.5).margin (1e-5));     // wrapped
    REQUIRE (lfoShape (LfoWave::Sawtooth, -0.25f, 0.0f) == Approx (0.5).margin (1e-5));
    REQUIRE (lfoShape (LfoWave::Sine, std::numeric_limits<float>::quiet_NaN(), 0.3f) == 0.0f);
    for (int w = 0; w < 5; ++w) for (int i = 0; i < 100; ++i) { const float v = lfoShape ((LfoWave) w, (float) i / 100.0f, 0.0f); REQUIRE (v >= -1.0f); REQUIRE (v <= 1.0f); }
}

TEST_CASE ("T-022 TapTempo needs three taps, averages, and restarts after five idle seconds", "[blocks][tap]") {
    TapTempo t;
    REQUIRE (t.periodSeconds() == 0.0);
    REQUIRE_FALSE (t.tap (10.0));
    REQUIRE_FALSE (t.tap (10.5));
    REQUIRE (t.periodSeconds() == 0.0);
    REQUIRE (t.tap (11.0));
    REQUIRE (t.periodSeconds() == Approx (0.5).margin (1e-9));
    REQUIRE (t.tap (11.8));                                            // mean of 0.5, 0.5, 0.8
    REQUIRE (t.periodSeconds() == Approx (0.6).margin (1e-9));
    REQUIRE_FALSE (t.tap (16.9));                                      // 5.1 s idle: a new run starts ...
    REQUIRE (t.periodSeconds() == Approx (0.6).margin (1e-9));         // ... and the old period stays available
    REQUIRE_FALSE (t.tap (17.1));
    REQUIRE (t.tap (17.3));
    REQUIRE (t.periodSeconds() == Approx (0.2).margin (1e-9));
    REQUIRE (t.tap (22.3));                                            // exactly 5.0 s is not idle: still the same run
    REQUIRE (t.periodSeconds() == Approx ((0.2 + 0.2 + 5.0) / 3.0).margin (1e-9));
    t.reset();
    REQUIRE (t.periodSeconds() == 0.0);
    REQUIRE_FALSE (t.tap (30.0));
    REQUIRE_FALSE (t.tap (31.0));
}

TEST_CASE ("T-023 filterBankEnvelopeTimes interpolates the frozen table", "[blocks][filterbank]") {
    auto at = [] (float e) { float a = -1.0f, d = -1.0f; filterBankEnvelopeTimes (e, a, d); return std::pair<float, float> (a, d); };
    const float E[6] = { 0, 2, 5, 6, 8, 10 }, A[6] = { 0.01f, 0.01f, 1.0f, 2.0f, 1.0f, 2.0f }, D[6] = { 0.25f, 1.0f, 1.0f, 2.0f, 0.02f, 0.02f };
    for (int i = 0; i < 6; ++i) { REQUIRE (at (E[i]).first == Approx (A[i]).margin (1e-5)); REQUIRE (at (E[i]).second == Approx (D[i]).margin (1e-5)); }
    REQUIRE (at (1.0f).second == Approx (0.625).margin (1e-5));
    REQUIRE (at (3.5f).first == Approx (0.505).margin (1e-5));
    REQUIRE (at (7.0f).first == Approx (1.5).margin (1e-5));
    REQUIRE (at (7.0f).second == Approx (1.01).margin (1e-5));
    REQUIRE (at (9.0f).first == Approx (1.5).margin (1e-5));
    REQUIRE (at (-5.0f) == at (0.0f));
    REQUIRE (at (50.0f) == at (10.0f));
    REQUIRE (at (std::numeric_limits<float>::quiet_NaN()) == at (2.0f));
}

TEST_CASE ("T-024 the frozen pattern set is well formed", "[blocks][filterbank]") {
    // Guards the fixture itself (and fails against stubs through the table lookup below).
    REQUIRE (effectInfo (EffectId::FilterBank).params[findParam (effectInfo (EffectId::FilterBank), "pattern")].numSteps == 12);
    for (int p = 0; p < 24; ++p) for (int c = 0; c < 8; ++c) {
        const char* s = kExpectedPatterns[p][c];
        const size_t n = std::strlen (s);
        if (p % 12 == 0) { REQUIRE (std::strcmp (s, "-") == 0); continue; }
        REQUIRE (n >= 1); REQUIRE (n <= 64);
        bool any = false;
        for (size_t i = 0; i < n; ++i) { REQUIRE ((s[i] == 'x' || s[i] == '.')); any = any || s[i] == 'x'; }
        REQUIRE (any);
        REQUIRE (std::strcmp (s, kExpectedPatterns[(p + 12) % 24][7 - c]) == 0);   // the Mids bank mirrors the Bass bank
    }
}
