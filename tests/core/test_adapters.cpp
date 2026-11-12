// FROZEN - DO NOT MODIFY. Host adaptors: BlockRunner (T-801..), RackBridge (T-811..), parity (T-821).
#include "test_support.hpp"

#include <limits>

using namespace muff;
using Catch::Approx;

namespace {
const double kFs = 48000.0;
ts::Signal tone (long n, double f, double amp, double phase = 0.0) { ts::Signal s ((size_t) n); for (long i = 0; i < n; ++i) s[(size_t) i] = (float) (amp * std::sin (2.0 * ts::kPi * f * i / kFs + phase)); return s; }
void livelySettings (Effect& fx) {       // defaults except: resonance, LFO depth and feedback where they exist
    const EffectInfo& e = fx.info();
    for (const char* id : { "resonance", "lfo_amount", "feedback" }) { const int p = findParam (e, id); if (p >= 0) fx.setParam (p, 0.5f * (e.params[p].min + e.params[p].max)); }
    const int pat = findParam (e, "pattern"); if (pat >= 0) fx.setParam (pat, 3.0f);
}
}

TEST_CASE ("T-801 BlockRunner equals per-sample processing and does not depend on block size", "[adapters][blockrunner]") {
    for (EffectId id : ts::kAllEffects) {
        const long n = 9600;
        const ts::Signal in = tone (n, 330.0, 0.4);
        auto fx = createEffect (id); fx->prepare (kFs); livelySettings (*fx);
        const EffectInfo& e = fx->info();
        INFO (e.slug);
        ts::Signal wantL ((size_t) n), wantR ((size_t) n);
        float x[kMaxPorts] {}, y[kMaxPorts] {};
        for (long i = 0; i < n; ++i) { x[0] = in[(size_t) i]; fx->process (x, y); wantL[(size_t) i] = y[0]; wantR[(size_t) i] = e.rightOutput >= 0 ? y[e.rightOutput] : y[0]; }
        for (int block : { 1, 64, 1000 }) {
            BlockRunner br (id); br.prepare (kFs); livelySettings (br.effect());
            ts::Signal L ((size_t) n), R ((size_t) n);
            for (long pos = 0; pos < n; pos += block) {
                const int len = (int) std::min<long> (block, n - pos);
                const float* ins[1] = { in.data() + pos }; float* outs[2] = { L.data() + pos, R.data() + pos };
                br.process (ins, 1, outs, 2, len);
            }
            INFO ("block " << block);
            REQUIRE (L == wantL);
            REQUIRE (R == wantR);
        }
    }
}

TEST_CASE ("T-802 BlockRunner channel rules: stereo in is averaged, mono out sums a split pair, in-place works", "[adapters][blockrunner]") {
    const long n = 4800;
    const ts::Signal a = tone (n, 330.0, 0.4), b = tone (n, 330.0, 0.2), mean = tone (n, 330.0, 0.3), zero ((size_t) n, 0.0f);
    for (EffectId id : ts::kAllEffects) {
        const EffectInfo& e = effectInfo (id);
        INFO (e.slug);
        BlockRunner mono (id), stereo (id), none (id), silent (id), inplace (id), one (id);
        for (BlockRunner* r : { &mono, &stereo, &none, &silent, &inplace, &one }) { r->prepare (kFs); livelySettings (r->effect()); }
        ts::Signal mL ((size_t) n), mR ((size_t) n), sL ((size_t) n), sR ((size_t) n), nL ((size_t) n), nR ((size_t) n), zL ((size_t) n), zR ((size_t) n), oM ((size_t) n);
        { const float* i1[1] = { mean.data() }; float* o[2] = { mL.data(), mR.data() }; mono.process (i1, 1, o, 2, (int) n); }
        { const float* i2[2] = { a.data(), b.data() }; float* o[2] = { sL.data(), sR.data() }; stereo.process (i2, 2, o, 2, (int) n); }
        { float* o[2] = { nL.data(), nR.data() }; none.process (nullptr, 0, o, 2, (int) n); }
        { const float* i1[1] = { zero.data() }; float* o[2] = { zL.data(), zR.data() }; silent.process (i1, 1, o, 2, (int) n); }
        { const float* i1[1] = { mean.data() }; float* o[1] = { oM.data() }; one.process (i1, 1, o, 1, (int) n); }
        double worst = 0.0; for (size_t i = 0; i < (size_t) n; ++i) worst = std::max (worst, (double) std::max (std::fabs (sL[i] - mL[i]), std::fabs (sR[i] - mR[i])));
        REQUIRE (worst < 1.0e-5);                                   // 0.5 * (L + R) feeds the effect
        REQUIRE (nL == zL); REQUIRE (nR == zR);                     // no input channel = silence
        for (size_t i = 0; i < (size_t) n; ++i) {
            const float want = (e.monoSumsRight && e.rightOutput >= 0) ? mL[i] + mR[i] : mL[i];
            if (std::fabs (oM[i] - want) > 1.0e-6f) { FAIL ("mono output rule broken at sample " << i); }
        }
        if (e.rightOutput < 0) REQUIRE (mL == mR);                  // no second output: right copies left
        ts::Signal bufL = a, bufR = b;                              // in-place
        { const float* i2[2] = { bufL.data(), bufR.data() }; float* o[2] = { bufL.data(), bufR.data() }; inplace.process (i2, 2, o, 2, (int) n); }
        REQUIRE (bufL == sL); REQUIRE (bufR == sR);
        // invalid calls do nothing
        ts::Signal guard ((size_t) 16, 7.0f); float* og[2] = { guard.data(), guard.data() }; const float* ig[3] = { a.data(), a.data(), a.data() };
        one.process (ig, 3, og, 2, 16); one.process (ig, 1, og, 3, 16); one.process (ig, 1, og, 0, 16); one.process (ig, 1, og, 2, 0); one.process (ig, 1, og, 2, -5);
        for (float v : guard) REQUIRE (v == 7.0f);
    }
    REQUIRE_THROWS_AS (BlockRunner ((EffectId) 42), std::out_of_range);
    REQUIRE_THROWS_AS (RackBridge ((EffectId) -3), std::out_of_range);
    BlockRunner r (EffectId::Phaser);
    REQUIRE_THROWS_AS (r.prepare (10.0), std::invalid_argument);
    REQUIRE (&r.effect().info() == &effectInfo (EffectId::Phaser));
}

TEST_CASE ("T-811 RackBridge scales volts: 5 V is one core unit on every port", "[adapters][rack]") {
    for (EffectId id : ts::kAllEffects) {
        RackBridge rb (id); rb.prepare (kFs); livelySettings (rb.effect());
        auto fx = createEffect (id); fx->prepare (kFs); livelySettings (*fx);
        const EffectInfo& e = fx->info();
        INFO (e.slug);
        for (int i = 1; i < e.numInputs; ++i) fx->setInputConnected (i, i % 2 == 1);
        ts::Rng rng (7u + (std::uint32_t) id);
        RackIO io {};
        for (int o = 0; o < kMaxPorts; ++o) io.outConnected[o] = true;
        double worst = 0.0;
        float x[kMaxPorts] {}, y[kMaxPorts] {};
        for (long n = 0; n < 9600; ++n) {
            for (int i = 0; i < e.numInputs; ++i) {
                const float core = i == 0 ? (float) (0.4 * std::sin (2.0 * ts::kPi * 330.0 * n / kFs)) : 0.3f * rng.bip();
                x[i] = core; io.inVolts[i] = 5.0f * core; io.inConnected[i] = (i == 0) || (i % 2 == 1);
            }
            fx->process (x, y); rb.process (io);
            for (int o = 0; o < e.numOutputs; ++o) worst = std::max (worst, (double) std::fabs (io.outVolts[o] - 5.0f * y[o]));
        }
        REQUIRE (worst < 1.0e-5);
    }
}

TEST_CASE ("T-812 RackBridge connection rules and bad voltages", "[adapters][rack]") {
    // FilterBank: with nothing patched into the right output, the left output carries both halves
    RackBridge a (EffectId::FilterBank), b (EffectId::FilterBank);
    a.prepare (kFs); b.prepare (kFs);
    RackIO ia {}, ib {};
    ia.inConnected[0] = ib.inConnected[0] = true;
    ia.outConnected[0] = ib.outConnected[0] = true; ia.outConnected[1] = true; ib.outConnected[1] = false;
    double worst = 0.0, energyR = 0.0;
    for (long n = 0; n < 9600; ++n) {
        ia.inVolts[0] = ib.inVolts[0] = (float) (2.0 * std::sin (2.0 * ts::kPi * 300.0 * n / kFs) + 2.0 * std::sin (2.0 * ts::kPi * 1000.0 * n / kFs));
        a.process (ia); b.process (ib);
        worst = std::max (worst, (double) std::fabs (ib.outVolts[0] - (ia.outVolts[0] + ia.outVolts[1])));
        energyR += (double) ia.outVolts[1] * ia.outVolts[1];
    }
    REQUIRE (worst < 1.0e-5);
    REQUIRE (energyR > 1.0);
    // Chorus does not sum its right output
    RackBridge c (EffectId::Chorus), d (EffectId::Chorus);
    c.prepare (kFs); d.prepare (kFs);
    RackIO ic {}, id {};
    ic.inConnected[0] = id.inConnected[0] = true; ic.outConnected[0] = id.outConnected[0] = true; ic.outConnected[1] = true; id.outConnected[1] = false;
    for (long n = 0; n < 4800; ++n) { ic.inVolts[0] = id.inVolts[0] = (float) (2.0 * std::sin (2.0 * ts::kPi * 300.0 * n / kFs)); c.process (ic); d.process (id); if (ic.outVolts[0] != id.outVolts[0]) FAIL ("left output changed"); }
    // an unconnected input is ignored whatever its voltage; non-finite voltages are harmless
    RackBridge e (EffectId::Lowpass), f (EffectId::Lowpass);
    e.prepare (kFs); f.prepare (kFs);
    RackIO ie {}, jf {};
    ie.inConnected[0] = jf.inConnected[0] = true;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    bool same = true, finite = true;
    for (long n = 0; n < 4800; ++n) {
        ie.inVolts[0] = jf.inVolts[0] = (float) (1.0 * std::sin (2.0 * ts::kPi * 300.0 * n / kFs));
        jf.inVolts[1] = 9.0f; jf.inVolts[2] = nan;      // not connected
        e.process (ie); f.process (jf);
        same = same && ie.outVolts[0] == jf.outVolts[0];
    }
    REQUIRE (same);
    jf.inConnected[1] = jf.inConnected[2] = true;
    for (long n = 0; n < 4800; ++n) { jf.inVolts[0] = n % 7 == 0 ? nan : 3.0f; jf.inVolts[1] = n % 5 == 0 ? std::numeric_limits<float>::infinity() : 12.0f; f.process (jf); finite = finite && std::isfinite (jf.outVolts[0]) && std::fabs (jf.outVolts[0]) <= 12.0f; }
    REQUIRE (finite);
    // outputs never exceed +/-12 V, even when the effect is driven to its limit (Lowpass self-oscillating into +28 dB of drive)
    RackBridge loud (EffectId::Lowpass); loud.prepare (kFs);
    loud.effect().setParam (findParam (effectInfo (EffectId::Lowpass), "drive"), 28.0f);
    loud.effect().setParam (findParam (effectInfo (EffectId::Lowpass), "resonance"), 9.0f);
    RackIO il {}; il.inConnected[0] = true; il.outConnected[0] = true;
    float top = 0.0f;
    for (long n = 0; n < 48000; ++n) { il.inVolts[0] = (float) (10.0 * std::sin (2.0 * ts::kPi * 700.0 * n / kFs)); loud.process (il); top = std::max (top, std::fabs (il.outVolts[0])); }
    REQUIRE (top <= 12.0f);
    REQUIRE (top > 5.0f);
    // a trigger at the Rack standard of 10 V (and a 5 V one) taps the tempo; 0.5 V does not
    for (float volts : { 10.0f, 5.0f, 0.5f }) {
        RackBridge t (EffectId::Chorus); t.prepare (kFs);
        const EffectInfo& ci = effectInfo (EffectId::Chorus);
        const int tapIn = findInput (ci, "tap_in"), rate = findMeter (ci, "rate");
        t.effect().setParam (findParam (ci, "lfo_rate"), 10.0f);
        RackIO io {}; io.inConnected[0] = true; io.inConnected[tapIn] = true; io.outConnected[0] = true;
        int edges = 0; float prev = 0.0f;
        for (long n = 0; n < 5 * 48000; ++n) {
            const long m = n % 24000;       // a pulse every 0.5 s for the first 1.5 s
            io.inVolts[tapIn] = (n < 72000 && m < 480) ? volts : 0.0f;
            t.process (io);
            const float mv = t.effect().getMeter (rate);
            if (n >= 96000 && mv > 0.5f && prev <= 0.5f) ++edges;
            prev = mv;
        }
        INFO ("trigger volts " << volts);
        if (volts >= 1.0f) { REQUIRE (edges >= 5); REQUIRE (edges <= 7); }      // 2 Hz for 3 s
        else { REQUIRE (edges >= 29); REQUIRE (edges <= 31); }                   // still the knob's 10 Hz
    }
}

TEST_CASE ("T-821 the VST3 path and the Rack path give the same audio", "[adapters][parity]") {
    for (EffectId id : ts::kAllEffects) {
        const long n = 9600;
        const ts::Signal in = tone (n, 250.0, 0.4);
        BlockRunner br (id); br.prepare (kFs); livelySettings (br.effect());
        RackBridge rb (id); rb.prepare (kFs); livelySettings (rb.effect());
        const EffectInfo& e = effectInfo (id);
        ts::Signal L ((size_t) n), R ((size_t) n);
        const float* ins[1] = { in.data() }; float* outs[2] = { L.data(), R.data() };
        br.process (ins, 1, outs, 2, (int) n);
        RackIO io {}; io.inConnected[0] = true; for (int o = 0; o < kMaxPorts; ++o) io.outConnected[o] = true;
        double worst = 0.0;
        for (long i = 0; i < n; ++i) {
            io.inVolts[0] = 5.0f * in[(size_t) i];
            rb.process (io);
            worst = std::max (worst, (double) std::fabs (io.outVolts[0] / 5.0f - L[(size_t) i]));
            if (e.rightOutput >= 0) worst = std::max (worst, (double) std::fabs (io.outVolts[e.rightOutput] / 5.0f - R[(size_t) i]));
        }
        INFO (e.slug);
        REQUIRE (worst <= 1.0e-6);
    }
}
