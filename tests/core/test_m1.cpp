// FROZEN - DO NOT MODIFY. Milestone 1 behaviour: Lowpass (T-101..), RingMod (T-201..), Phaser (T-301..).
#include "test_support.hpp"

using namespace muff;
using Catch::Approx;

namespace {
const double kFs = 48000.0;

// steady-state gain of output `o` for a small sine at `freq`
double sineGain (ts::Rig& r, double freq, int o = 0, double amp = 0.05) {
    r.fx->reset(); r.clock = 0;
    const long n = r.samples (0.6);
    const auto y = r.run (n, { { 0, ts::sine (freq, r.fs, amp) } });
    return ts::toneAmp (y[(size_t) o], n / 2, n, freq, r.fs) / amp;
}
// gain of an N-pole chain of identical trapezoidal one-poles (pole at fc) at frequency f
double onePoleChainDb (double f, double fc, int poles, double fs) {
    const double w = std::tan (ts::kPi * f / fs) / std::tan (ts::kPi * fc / fs);
    return -10.0 * poles * std::log10 (1.0 + w * w);
}
}

// ============================================================ Lowpass

TEST_CASE ("T-101 Lowpass 4-pole response without resonance", "[lowpass][m1]") {
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("cutoff", 1000.0f); r.set ("resonance", 0.0f); r.set ("poles", 1.0f); r.set ("mix", 1.0f);
    REQUIRE (ts::dB (sineGain (r, 100.0)) == Approx (onePoleChainDb (100.0, 1000.0, 4, kFs)).margin (0.3));     // -0.17 dB
    REQUIRE (ts::dB (sineGain (r, 1000.0)) == Approx (-12.04).margin (0.5));
    REQUIRE (ts::dB (sineGain (r, 4000.0)) == Approx (onePoleChainDb (4000.0, 1000.0, 4, kFs)).margin (1.5));  // about -49.9 dB
}

TEST_CASE ("T-102 Lowpass 2-pole response without resonance", "[lowpass][m1]") {
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("cutoff", 1000.0f); r.set ("resonance", 0.0f); r.set ("poles", 0.0f); r.set ("mix", 1.0f);
    REQUIRE (ts::dB (sineGain (r, 1000.0)) == Approx (-6.02).margin (0.5));
    REQUIRE (ts::dB (sineGain (r, 4000.0)) == Approx (onePoleChainDb (4000.0, 1000.0, 2, kFs)).margin (1.5));  // about -24.9 dB
}

TEST_CASE ("T-103 Lowpass self-oscillates at the cutoff frequency when resonance is 10", "[lowpass][m1]") {
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (double fc : { 100.0, 1000.0, 5000.0 }) {
            ts::Rig r (EffectId::Lowpass, fs);
            INFO ("fs " << fs << " cutoff " << fc);
            r.set ("cutoff", (float) fc); r.set ("resonance", 10.0f); r.set ("mix", 1.0f); r.fx->reset();
            const long n = r.samples (3.0);
            const auto y = r.run (n)[0];        // no input at all
            const double amp = ts::peak (y, r.samples (2.5), n);
            REQUIRE (amp > 0.05); REQUIRE (amp < 2.5);
            REQUIRE (ts::zcFreq (y, r.samples (2.5), n, fs) == Approx (fc).epsilon (0.02));
        }
}

TEST_CASE ("T-104 Lowpass does not self-oscillate at resonance 7", "[lowpass][m1]") {
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("cutoff", 1000.0f); r.set ("resonance", 7.0f); r.set ("mix", 1.0f); r.fx->reset();
    const long n = r.samples (2.0);
    const auto y = r.run (n, { { 0, ts::burst (1000.0, kFs, 0.5, 0, 4800) } })[0];
    REQUIRE (ts::peak (y, 0, 9600) > 0.1);
    REQUIRE (ts::peak (y, r.samples (1.0), n) < 1.0e-3);
}

TEST_CASE ("T-105 Lowpass resonance: loop gain k = resonance / 2", "[lowpass][m1]") {
    // 4-pole ladder with feedback k: gain at the cutoff is (1/4)/(1 - k/4), DC gain is 1/(1 + k).
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("cutoff", 1000.0f); r.set ("resonance", 6.0f); r.set ("mix", 1.0f);        // k = 3
    REQUIRE (ts::dB (sineGain (r, 1000.0, 0, 0.01)) == Approx (0.0).margin (1.0));
    REQUIRE (ts::dB (sineGain (r, 20.0, 0, 0.01)) == Approx (-12.04).margin (0.5));
    r.set ("resonance", 4.0f);                                                          // k = 2
    REQUIRE (ts::dB (sineGain (r, 1000.0, 0, 0.01)) == Approx (-6.02).margin (1.0));
    REQUIRE (ts::dB (sineGain (r, 20.0, 0, 0.01)) == Approx (-9.54).margin (0.5));
}

TEST_CASE ("T-106 Lowpass envelope follower: level, speeds and cutoff modulation", "[lowpass][m1]") {
    const int envOut = 1;
    auto envTrace = [&] (float speed, double amp) {
        ts::Rig r (EffectId::Lowpass, kFs);
        r.set ("env_speed", speed); r.fx->reset();
        return r.run (r.samples (3.0), { { 0, ts::burst (1000.0, kFs, amp, 0, r.samples (1.0)) } })[(size_t) envOut];
    };
    const ts::Signal fast = envTrace (1.0f, 1.0), smooth = envTrace (0.0f, 1.0), quiet = envTrace (1.0f, 0.25);
    const long s1 = 48000;
    const double fFinal = ts::rms (fast, s1 - 4800, s1), sFinal = ts::rms (smooth, s1 - 4800, s1);
    REQUIRE (fFinal > 0.7); REQUIRE (fFinal < 0.95);            // a full-scale sine (0.92 after the soft limiter) reads a little under its peak
    REQUIRE (sFinal == Approx (fFinal).epsilon (0.1));
    REQUIRE (ts::rms (quiet, s1 - 4800, s1) == Approx (0.22).margin (0.04));
    REQUIRE (fast[(size_t) (0.020 * kFs)] > 0.9 * fFinal);      // Fast: attack 2 ms
    REQUIRE (smooth[(size_t) (0.010 * kFs)] < 0.6 * sFinal);    // Smooth: attack 20 ms
    REQUIRE (smooth[(size_t) (0.200 * kFs)] > 0.9 * sFinal);
    REQUIRE (fast[(size_t) (s1 + 0.250 * kFs)] < 0.1 * fFinal); // Fast: release 50 ms
    REQUIRE (smooth[(size_t) (s1 + 0.250 * kFs)] > 0.2 * sFinal);   // Smooth: release 300 ms
    REQUIRE (smooth[(size_t) (s1 + 1.5 * kFs)] < 0.05 * sFinal);
    for (float v : fast) { REQUIRE (v >= 0.0f); REQUIRE (v <= 2.0f); }
    // the envelope opens the filter: Amount 5 octaves lets a loud 1 kHz tone through a 200 Hz cutoff
    auto gain = [&] (float amount) {
        ts::Rig r (EffectId::Lowpass, kFs);
        r.set ("cutoff", 200.0f); r.set ("env_amount", amount); r.set ("mix", 1.0f); r.fx->reset();
        const auto y = r.run (48000, { { 0, ts::sine (1000.0, kFs, 1.0) } })[0];
        return ts::dB (ts::toneAmp (y, 24000, 48000, 1000.0, kFs));
    };
    REQUIRE (gain (5.0f) > -4.0);
    REQUIRE (gain (0.0f) < -40.0);
    // Envelope Out does not depend on Amount
    ts::Rig a (EffectId::Lowpass, kFs), b (EffectId::Lowpass, kFs);
    b.set ("env_amount", 5.0f); b.fx->reset();
    const auto src = std::map<int, ts::Source> { { 0, ts::sine (500.0, kFs, 0.5) } };
    REQUIRE (a.run (9600, src)[(size_t) envOut] == b.run (9600, src)[(size_t) envOut]);
}

TEST_CASE ("T-107 Lowpass Mix crossfades from the driven dry signal to the filter", "[lowpass][m1]") {
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("cutoff", 200.0f); r.set ("mix", 0.0f); r.fx->reset();
    const auto y = r.run (9600, { { 0, ts::sine (2000.0, kFs, 0.3) } })[0];
    double worst = 0.0;
    for (long n = 2400; n < 9600; ++n) worst = std::max (worst, (double) std::fabs (y[(size_t) n] - 2.0f * std::tanh (0.5f * (float) (0.3 * std::sin (2.0 * ts::kPi * 2000.0 * n / kFs)))));
    REQUIRE (worst < 1.0e-3);
    r.set ("mix", 0.5f);
    const double half = sineGain (r, 2000.0);       // the filter contributes almost nothing at 2 kHz (-80 dB)
    REQUIRE (half == Approx (0.5).epsilon (0.03));
}

TEST_CASE ("T-108 Drive spans -7 dB to +28 dB into a soft limiter", "[lowpass][m1][drive]") {
    ts::Rig r (EffectId::Lowpass, kFs);
    r.set ("mix", 0.0f);
    r.set ("drive", -7.0f);
    REQUIRE (sineGain (r, 500.0, 0, 0.01) == Approx (std::pow (10.0, -7.0 / 20.0)).epsilon (0.01));
    r.set ("drive", 28.0f);
    REQUIRE (sineGain (r, 500.0, 0, 0.0005) == Approx (std::pow (10.0, 28.0 / 20.0)).epsilon (0.02));
    r.fx->reset(); r.clock = 0;
    const auto y = r.run (9600, { { 0, ts::sine (500.0, kFs, 1.0) } })[0];
    const double pk = ts::peak (y, 4800, 9600);
    REQUIRE (pk > 1.9); REQUIRE (pk <= 2.0);       // limits at +/-2
}

// ============================================================ RingMod

namespace {
float ringKnobFor (double hz) { return (float) (std::log (hz / 30.0) / std::log (4000.0 / 30.0)); }   // Hi range
}

TEST_CASE ("T-201 RingMod produces only sum and difference frequencies", "[ringmod][m1]") {
    ts::Rig r (EffectId::RingMod, kFs);
    r.set ("range", 1.0f); r.set ("frequency", ringKnobFor (100.0)); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f); r.fx->reset();
    const long n = r.samples (1.0);
    const auto y = r.run (n, { { 0, ts::sine (500.0, kFs, 0.1) } })[0];
    const double in = 2.0 * std::tanh (0.05);
    REQUIRE (ts::toneAmp (y, n / 2, n, 400.0, kFs) == Approx (0.5 * in).epsilon (0.05));
    REQUIRE (ts::toneAmp (y, n / 2, n, 600.0, kFs) == Approx (0.5 * in).epsilon (0.05));
    REQUIRE (ts::dB (ts::toneAmp (y, n / 2, n, 500.0, kFs) / in) < -50.0);      // no input feedthrough
    REQUIRE (ts::dB (ts::toneAmp (y, n / 2, n, 100.0, kFs) / in) < -50.0);      // no carrier feedthrough
}

TEST_CASE ("T-202 RingMod carrier ranges: Lo 0.6-80 Hz, Hi 30 Hz-4 kHz; Carrier Out is +/-1.25 V", "[ringmod][m1]") {
    struct Case { float range, knob; double hz, fs, seconds; };
    for (const Case& c : { Case { 0, 0.0f, 0.6, 8000.0, 12.0 }, Case { 0, 1.0f, 80.0, 8000.0, 2.0 }, Case { 0, 0.5f, std::sqrt (0.6 * 80.0), 8000.0, 6.0 },
                           Case { 1, 0.0f, 30.0, 48000.0, 2.0 }, Case { 1, 1.0f, 4000.0, 48000.0, 1.0 }, Case { 1, 0.5f, std::sqrt (30.0 * 4000.0), 48000.0, 1.0 } }) {
        ts::Rig r (EffectId::RingMod, c.fs);
        INFO ("range " << c.range << " knob " << c.knob);
        r.set ("range", c.range); r.set ("frequency", c.knob); r.set ("lfo_amount", 0.0f); r.fx->reset();
        const long n = r.samples (c.seconds);
        const auto car = r.run (n)[(size_t) r.out ("carrier_out")];
        REQUIRE (ts::zcFreq (car, n / 10, n, c.fs) == Approx (c.hz).epsilon (0.01));
        REQUIRE (ts::peak (car, n / 10, n) == Approx (0.25).epsilon (0.02));
    }
}

TEST_CASE ("T-203 RingMod LFO: 0.1-25 Hz, sine or square, LFO Out is +/-1.5 V whatever the Amount", "[ringmod][m1]") {
    const double fs = 8000.0;
    for (float amount : { 0.0f, 3.0f }) {
        ts::Rig r (EffectId::RingMod, fs);
        r.set ("lfo_rate", 25.0f); r.set ("lfo_amount", amount); r.fx->reset();
        const long n = r.samples (2.0);
        const auto l = r.run (n)[(size_t) r.out ("lfo_out")];
        REQUIRE (ts::zcFreq (l, 800, n, fs) == Approx (25.0).epsilon (0.01));
        REQUIRE (ts::peak (l, 800, n) == Approx (0.3).epsilon (0.02));
    }
    ts::Rig slow (EffectId::RingMod, fs);
    slow.set ("lfo_rate", 0.1f); slow.fx->reset();
    const long n = slow.samples (35.0);
    const auto l = slow.run (n)[(size_t) slow.out ("lfo_out")];
    REQUIRE (ts::zcFreq (l, 0, n, fs) == Approx (0.1).epsilon (0.01));
    ts::Rig sq (EffectId::RingMod, fs);
    sq.set ("lfo_rate", 5.0f); sq.set ("lfo_wave", 1.0f); sq.fx->reset();
    const auto q = sq.run (sq.samples (1.0))[(size_t) sq.out ("lfo_out")];
    long hi = 0, lo = 0;
    for (float v : q) { if (std::fabs (v - 0.3f) < 0.003f) ++hi; else if (std::fabs (v + 0.3f) < 0.003f) ++lo; }
    REQUIRE (hi + lo == (long) q.size());
    REQUIRE (std::labs (hi - lo) < (long) q.size() / 50);
}

TEST_CASE ("T-204 RingMod LFO Amount is the peak-to-peak carrier sweep in octaves", "[ringmod][m1]") {
    ts::Rig r (EffectId::RingMod, kFs);
    r.set ("range", 1.0f); r.set ("frequency", ringKnobFor (400.0)); r.set ("lfo_wave", 1.0f); r.set ("lfo_rate", 1.0f); r.set ("lfo_amount", 3.0f); r.fx->reset();
    const long n = r.samples (1.0);
    const auto car = r.run (n)[(size_t) r.out ("carrier_out")];
    // square LFO: first half second high, second half second low (5 ms smoothing at the edges is skipped)
    REQUIRE (ts::zcFreq (car, r.samples (0.05), r.samples (0.45), kFs) == Approx (400.0 * std::pow (2.0, 1.5)).epsilon (0.02));
    REQUIRE (ts::zcFreq (car, r.samples (0.55), r.samples (0.95), kFs) == Approx (400.0 * std::pow (2.0, -1.5)).epsilon (0.02));
}

TEST_CASE ("T-205 RingMod external carrier replaces the internal oscillator in the multiplier", "[ringmod][m1]") {
    ts::Rig r (EffectId::RingMod, kFs);
    r.set ("range", 1.0f); r.set ("frequency", ringKnobFor (1000.0)); r.set ("mix", 1.0f); r.connect ("carrier_in"); r.fx->reset();
    const long n = r.samples (1.0);
    const auto y = r.run (n, { { 0, ts::sine (500.0, kFs, 0.1) }, { r.in ("carrier_in"), ts::sine (200.0, kFs, 0.14142) } });   // 0.5 V RMS
    const double in = 2.0 * std::tanh (0.05);
    REQUIRE (ts::toneAmp (y[0], n / 2, n, 300.0, kFs) == Approx (0.5 * in).epsilon (0.05));
    REQUIRE (ts::toneAmp (y[0], n / 2, n, 700.0, kFs) == Approx (0.5 * in).epsilon (0.05));
    REQUIRE (ts::dB (ts::toneAmp (y[0], n / 2, n, 1500.0, kFs) / in) < -50.0);     // nothing from the internal 1 kHz carrier
    REQUIRE (ts::zcFreq (y[(size_t) r.out ("carrier_out")], n / 2, n, kFs) == Approx (1000.0).epsilon (0.01));   // Carrier Out keeps the internal oscillator
}

TEST_CASE ("T-206 RingMod Mix at zero is the driven dry signal", "[ringmod][m1]") {
    ts::Rig r (EffectId::RingMod, kFs);
    r.set ("mix", 0.0f); r.fx->reset();
    const auto y = r.run (9600, { { 0, ts::sine (500.0, kFs, 0.3) } })[0];
    double worst = 0.0;
    for (long n = 2400; n < 9600; ++n) worst = std::max (worst, (double) std::fabs (y[(size_t) n] - 2.0f * std::tanh (0.5f * (float) (0.3 * std::sin (2.0 * ts::kPi * 500.0 * n / kFs)))));
    REQUIRE (worst < 1.0e-3);
}

// ============================================================ Phaser

namespace {
// impulse response of a Phaser output with the LFO parked
ts::Signal phaserIr (ts::Rig& r, const char* output, float sweepIn = 0.0f, bool sweepConnected = false) {
    r.set ("lfo_amount", 0.0f);
    if (sweepConnected) r.connect ("sweep_in");
    r.fx->reset(); r.clock = 0;
    std::map<int, ts::Source> src { { 0, ts::impulseAt (0, 0.01f) } };
    if (sweepConnected) src[r.in ("sweep_in")] = ts::constant (sweepIn);
    return r.run (16384, src)[(size_t) r.out (output)];
}
// frequencies of the response dips (local minima below -20 dB) between 20 Hz and 20 kHz
std::vector<double> dips (const ts::Signal& h, double fs) {
    std::vector<double> f, g, found;
    for (int i = 0; i <= 1200; ++i) { f.push_back (20.0 * std::pow (1000.0, i / 1200.0)); g.push_back (ts::dB (ts::irGain (h, f.back(), fs, 0.01))); }
    for (size_t i = 1; i + 1 < f.size(); ++i) if (g[i] < g[i - 1] && g[i] <= g[i + 1] && g[i] < -20.0) found.push_back (f[i]);
    return found;
}
// frequency at which one all-pass stage with corner fc has phase -2*theta
double stageFreq (double fc, double thetaDeg, double fs) { return fs / ts::kPi * std::atan (std::tan (ts::kPi * fc / fs) * std::tan (thetaDeg * ts::kPi / 180.0)); }
}

TEST_CASE ("T-301 Phaser: three dips with 6 stages, six with 12, at the all-pass chain's frequencies", "[phaser][m1]") {
    const double fs = 96000.0, fc = 1100.0;     // Sweep 5 puts the stage corner at 1.1 kHz
    ts::Rig r (EffectId::Phaser, fs);
    r.set ("sweep", 5.0f); r.set ("resonance", 0.0f); r.set ("stages", 0.0f);
    const auto d6 = dips (phaserIr (r, "out"), fs);
    REQUIRE (d6.size() == 3);
    const double want6[3] = { 15.0, 45.0, 75.0 };
    for (int i = 0; i < 3; ++i) REQUIRE (d6[(size_t) i] == Approx (stageFreq (fc, want6[i], fs)).epsilon (0.03));
    r.set ("stages", 1.0f);
    const auto d12 = dips (phaserIr (r, "out"), fs);
    REQUIRE (d12.size() == 6);
    const double want12[6] = { 7.5, 22.5, 37.5, 52.5, 67.5, 82.5 };
    for (int i = 0; i < 6; ++i) REQUIRE (d12[(size_t) i] == Approx (stageFreq (fc, want12[i], fs)).epsilon (0.03));
    // away from the dips the response returns to 0 dB (half dry plus half all-pass)
    r.set ("stages", 0.0f);
    REQUIRE (ts::dB (ts::irGain (phaserIr (r, "out"), 30.0, fs, 0.01)) == Approx (0.0).margin (0.5));
}

TEST_CASE ("T-302 Phaser Aux Out has peaks where Audio Out has dips", "[phaser][m1]") {
    const double fs = 96000.0;
    ts::Rig r (EffectId::Phaser, fs);
    r.set ("sweep", 5.0f); r.set ("resonance", 0.0f); r.set ("stages", 0.0f);
    const ts::Signal main = phaserIr (r, "out"), aux = phaserIr (r, "aux_out");
    for (double f : { 100.0, stageFreq (1100.0, 15.0, fs), 500.0, 1100.0, 2500.0, stageFreq (1100.0, 75.0, fs), 9000.0 }) {
        const double a = ts::irGain (main, f, fs, 0.01), b = ts::irGain (aux, f, fs, 0.01);
        INFO ("f " << f);
        REQUIRE (a * a + b * b == Approx (1.0).margin (0.03));      // power-complementary
    }
    REQUIRE (ts::dB (ts::irGain (aux, 1100.0, fs, 0.01)) == Approx (0.0).margin (0.3));
    REQUIRE (ts::dB (ts::irGain (aux, 10.0, fs, 0.01)) < -20.0);
}

TEST_CASE ("T-303 Phaser Sweep spans six octaves around 1.1 kHz", "[phaser][m1]") {
    const double fs = 96000.0;
    for (auto [sweep, fc] : { std::pair<float, double> { 0.0f, 137.5 }, { 2.5f, 388.9 }, { 10.0f, 8800.0 } }) {
        ts::Rig r (EffectId::Phaser, fs);
        r.set ("sweep", sweep); r.set ("stages", 0.0f);
        const auto d = dips (phaserIr (r, "out"), fs);
        REQUIRE (d.size() >= 2);
        REQUIRE (d[1] == Approx (stageFreq (fc, 45.0, fs)).epsilon (0.03));
    }
    // Sweep CV: 0.5 CV units on top of Sweep 0 equals Sweep 5
    ts::Rig r (EffectId::Phaser, fs);
    r.set ("sweep", 0.0f); r.set ("stages", 0.0f); r.set ("lfo_amount", 0.0f); r.connect ("cv_sweep"); r.fx->reset();
    r.run (9600, { { r.in ("cv_sweep"), ts::constant (0.5f) } });      // let the smoothed control settle
    const auto h = r.run (16384, { { 0, ts::impulseAt (r.clock, 0.01f) }, { r.in ("cv_sweep"), ts::constant (0.5f) } })[0];
    const auto dcv = dips (h, fs);
    REQUIRE (dcv.size() == 3);
    REQUIRE (dcv[1] == Approx (stageFreq (1100.0, 45.0, fs)).epsilon (0.03));
}

TEST_CASE ("T-304 Phaser LFO: triangle, Lo 0.01-2.5 Hz, Hi 1-250 Hz, LFO Out +/-2.8 V", "[phaser][m1]") {
    const double fs = 8000.0;
    struct Case { float range, knob; double hz; };
    for (const Case& c : { Case { 1, 1.0f, 250.0 }, Case { 1, 0.0f, 1.0 }, Case { 0, 1.0f, 2.5 }, Case { 0, 0.0f, 0.01 }, Case { 0, 0.5f, 0.01 * std::sqrt (250.0) } }) {
        ts::Rig r (EffectId::Phaser, fs);
        INFO ("range " << c.range << " knob " << c.knob);
        r.set ("lfo_range", c.range); r.set ("lfo_rate", c.knob); r.set ("lfo_amount", 0.0f); r.fx->reset();
        const long n = r.samples (2.0);
        const auto l = r.run (n)[(size_t) r.out ("lfo_out")];
        // a triangle of amplitude A and frequency f moves 4*A*f per second: measure the mean absolute slope
        double travel = 0.0;
        for (long i = 801; i < n; ++i) travel += std::fabs (l[(size_t) i] - l[(size_t) (i - 1)]);
        REQUIRE (travel / ((double) (n - 801) / fs) == Approx (4.0 * 0.56 * c.hz).epsilon (0.02));
        REQUIRE (ts::peak (l, 0, n) <= 0.56 * 1.001);
        if (c.hz >= 1.0) REQUIRE (ts::peak (l, 0, n) == Approx (0.56).epsilon (0.02));
    }
}

TEST_CASE ("T-305 Phaser Resonance raises the peaks and leaves the dips in place", "[phaser][m1]") {
    const double fs = 96000.0;
    ts::Rig r (EffectId::Phaser, fs);
    r.set ("sweep", 5.0f); r.set ("stages", 0.0f); r.set ("resonance", 0.0f);
    const ts::Signal flat = phaserIr (r, "out");
    r.set ("resonance", 8.0f);
    const ts::Signal res = phaserIr (r, "out");
    const auto d0 = dips (flat, fs), d8 = dips (res, fs);
    REQUIRE (d8.size() == 3);
    for (int i = 0; i < 3; ++i) REQUIRE (d8[(size_t) i] == Approx (d0[(size_t) i]).epsilon (0.03));
    // between the first two dips the all-pass chain is in phase with the input: gain 1 / (1 - 0.09 * resonance)
    const double fPeak = stageFreq (1100.0, 30.0, fs);
    REQUIRE (ts::dB (ts::irGain (flat, fPeak, fs, 0.01)) == Approx (0.0).margin (0.5));
    REQUIRE (ts::dB (ts::irGain (res, fPeak, fs, 0.01)) == Approx (ts::dB (1.0 / (1.0 - 0.72))).margin (1.5));
    // Resonance 10 stays stable
    r.set ("resonance", 10.0f); r.fx->reset(); r.clock = 0;
    const auto y = r.run (r.samples (2.0), { { 0, ts::burst (700.0, fs, 0.5, 0, 9600) } })[0];
    REQUIRE (ts::peak (y, r.samples (1.5), r.samples (2.0)) < 1.0e-3);
}

TEST_CASE ("T-306 Phaser Sweep In moves the second six stages in 12-stage mode only", "[phaser][m1]") {
    const double fs = 96000.0;
    ts::Rig r (EffectId::Phaser, fs);
    r.set ("sweep", 5.0f); r.set ("stages", 1.0f); r.set ("resonance", 0.0f);
    const ts::Signal plain = phaserIr (r, "out");
    ts::Rig z (EffectId::Phaser, fs);
    z.set ("sweep", 5.0f); z.set ("stages", 1.0f); z.set ("resonance", 0.0f);
    const ts::Signal zero = phaserIr (z, "out", 0.0f, true);
    ts::Rig s (EffectId::Phaser, fs);
    s.set ("sweep", 5.0f); s.set ("stages", 1.0f); s.set ("resonance", 0.0f);
    const ts::Signal moved = phaserIr (s, "out", 0.25f, true);     // +1.25 V = +1.5 octaves on stages 7-12
    auto phase = [&] (double f, double fc) { return -2.0 * std::atan (std::tan (ts::kPi * f / fs) / std::tan (ts::kPi * fc / fs)); };
    for (double f : { 150.0, 400.0, 900.0, 1500.0, 2600.0, 5000.0 }) {
        INFO ("f " << f);
        REQUIRE (ts::irGain (zero, f, fs, 0.01) == Approx (ts::irGain (plain, f, fs, 0.01)).margin (0.01));
        const double total = 6.0 * phase (f, 1100.0) + 6.0 * phase (f, 1100.0 * std::pow (2.0, 1.5));
        const double want = std::fabs (std::cos (0.5 * total));       // |0.5 * (1 + e^{j total})|
        REQUIRE (ts::irGain (moved, f, fs, 0.01) == Approx (want).margin (0.05));
    }
    // 6-stage mode ignores Sweep In
    ts::Rig a (EffectId::Phaser, fs), b (EffectId::Phaser, fs);
    a.set ("stages", 0.0f); b.set ("stages", 0.0f);
    const ts::Signal six = phaserIr (a, "out"), sixSwept = phaserIr (b, "out", 0.4f, true);
    for (double f : { 300.0, 1100.0, 4000.0 }) REQUIRE (ts::irGain (sixSwept, f, fs, 0.01) == Approx (ts::irGain (six, f, fs, 0.01)).margin (0.01));
}

TEST_CASE ("T-307 Phaser LFO Amount sweeps the dips by up to +/-3 octaves; Output scales both outputs", "[phaser][m1]") {
    ts::Rig r (EffectId::Phaser, kFs);
    r.set ("sweep", 5.0f); r.set ("lfo_range", 1.0f); r.set ("lfo_rate", 0.0f); r.set ("lfo_amount", 1.0f); r.set ("stages", 0.0f); r.fx->reset();  // 1 Hz
    // a tone at the parked middle dip (1.1 kHz) is deeply cancelled only while the LFO passes through zero
    const long n = r.samples (2.0);
    const auto y = r.run (n, { { 0, ts::sine (1100.0, kFs, 0.1) } })[0];
    double lo = 1.0e9, hi = 0.0;
    for (long w = r.samples (0.5); w + 480 <= n; w += 480) { const double v = ts::rms (y, w, w + 480); lo = std::min (lo, v); hi = std::max (hi, v); }
    REQUIRE (ts::dB (hi / lo) > 20.0);
    ts::Rig a (EffectId::Phaser, kFs), b (EffectId::Phaser, kFs);
    a.set ("lfo_amount", 0.0f); b.set ("lfo_amount", 0.0f); b.set ("output", -12.0f); a.fx->reset(); b.fx->reset();
    const auto src = std::map<int, ts::Source> { { 0, ts::sine (300.0, kFs, 0.1) } };
    const auto ya = a.run (9600, src), yb = b.run (9600, src);
    for (int o : { 0, 1 }) REQUIRE (ts::dB (ts::rms (yb[(size_t) o], 4800, 9600) / ts::rms (ya[(size_t) o], 4800, 9600)) == Approx (-12.0).margin (0.1));
}
