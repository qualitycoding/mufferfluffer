// FROZEN - DO NOT MODIFY. Milestone 3 behaviour: FilterBank (T-501..), OscBox (T-601..).
#include "test_support.hpp"

using namespace muff;
using Catch::Approx;

namespace {
#include "expected_effects.inc"
const double kFs = 48000.0;
const double kMids[8] = { 200, 300, 450, 675, 1000, 1500, 2200, 3400 };
const double kBass[8] = { 110, 160, 240, 350, 525, 775, 1200, 1800 };

void soloFilter (ts::Rig& r, int c) {
    for (int i = 0; i < 8; ++i) { const std::string id = "level" + std::to_string (i + 1); r.set (id.c_str(), i == c ? 1.0f : 0.0f); }
}
// impulse response (both outputs summed) of one filter with the static pattern
ts::Signal filterIr (float voicing, int c, float sweep = 0.5f) {
    ts::Rig r (EffectId::FilterBank, kFs);
    r.set ("voicing", voicing); r.set ("pattern", 0.0f); r.set ("mix", 1.0f); r.set ("lfo", 0.0f); r.set ("sweep", sweep); soloFilter (r, c); r.fx->reset();
    const auto y = r.run (48000, { { 0, ts::impulseAt (0, 0.01f) } });
    ts::Signal h (y[0].size());
    for (size_t i = 0; i < h.size(); ++i) h[i] = (y[0][i] + y[1][i]) / 0.01f;
    return h;
}
}

// ============================================================ FilterBank

TEST_CASE ("T-501 FilterBank centre frequencies, unity centre gain and Q = 5", "[filterbank][m3]") {
    for (int c = 0; c < 8; ++c) {
        const ts::Signal h = filterIr (1.0f, c);
        const double fc = kMids[c];
        INFO ("mids filter " << c + 1);
        REQUIRE (ts::irPeakFreq (h, fc / 2.0, fc * 2.0, kFs) == Approx (fc).epsilon (0.02));
        REQUIRE (ts::dB (ts::irGain (h, fc, kFs, 1.0)) == Approx (0.0).margin (0.5));
        // -3 dB points of a band-pass with Q = 5 lie at fc * (sqrt(1 + 1/(4 Q^2)) -/+ 1/(2 Q))
        REQUIRE (ts::dB (ts::irGain (h, fc * 0.905, kFs, 1.0)) == Approx (-3.0).margin (1.0));
        REQUIRE (ts::dB (ts::irGain (h, fc * 1.105, kFs, 1.0)) == Approx (-3.0).margin (1.0));
        REQUIRE (ts::dB (ts::irGain (h, fc * 1.5, kFs, 1.0)) < -10.0);       // the neighbouring band is rejected
    }
    for (int c = 1; c < 8; ++c) {
        const ts::Signal h = filterIr (0.0f, c);
        INFO ("bass filter " << c + 1);
        REQUIRE (ts::irPeakFreq (h, kBass[c] / 2.0, kBass[c] * 2.0, kFs) == Approx (kBass[c]).epsilon (0.02));
        REQUIRE (ts::dB (ts::irGain (h, kBass[c], kFs, 1.0)) == Approx (0.0).margin (0.5));
    }
    const ts::Signal lp = filterIr (0.0f, 0);       // Bass filter 1 is a two-pole low-pass at 110 Hz
    REQUIRE (ts::dB (ts::irGain (lp, 20.0, kFs, 1.0)) == Approx (0.0).margin (0.5));
    REQUIRE (ts::dB (ts::irGain (lp, 110.0, kFs, 1.0)) == Approx (-3.0).margin (0.5));
    REQUIRE (ts::dB (ts::irGain (lp, 440.0, kFs, 1.0)) == Approx (-24.1).margin (1.5));
}

TEST_CASE ("T-502 FilterBank stereo split: odd filters left, even filters right; sliders scale linearly", "[filterbank][m3]") {
    for (int c = 0; c < 8; ++c) {
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("pattern", 0.0f); r.set ("mix", 1.0f); soloFilter (r, c); r.fx->reset();
        const auto y = r.run (24000, { { 0, ts::sine (kMids[c], kFs, 0.1) } });
        const double l = ts::rms (y[0], 12000, 24000), rr = ts::rms (y[1], 12000, 24000);
        INFO ("filter " << c + 1);
        if (c % 2 == 0) { REQUIRE (l > 0.05); REQUIRE (rr < 1.0e-5); } else { REQUIRE (rr > 0.05); REQUIRE (l < 1.0e-5); }
    }
    ts::Rig r (EffectId::FilterBank, kFs);
    r.set ("pattern", 0.0f); r.set ("mix", 1.0f); soloFilter (r, 4); r.fx->reset();
    const double full = ts::rms (r.run (24000, { { 0, ts::sine (1000.0, kFs, 0.1) } })[0], 12000, 24000);
    r.set ("level5", 0.25f);
    const double quarter = ts::rms (r.run (24000, { { 0, ts::sine (1000.0, kFs, 0.1) } })[0], 12000, 24000);
    REQUIRE (quarter / full == Approx (0.25).epsilon (0.02));
    // Mix 0: the driven dry signal at both outputs
    r.set ("mix", 0.0f); r.run (4800);
    const auto d = r.run (4800, { { 0, ts::sine (1000.0, kFs, 0.1) } });
    double lr = 0.0; for (size_t i = 0; i < d[0].size(); ++i) lr = std::max (lr, (double) std::fabs (d[0][i] - d[1][i]));
    REQUIRE (lr < 1.0e-5);
    REQUIRE (ts::rms (d[0], 0, 4800) == Approx (2.0 * std::tanh (0.05) / std::sqrt (2.0)).epsilon (0.02));
}

TEST_CASE ("T-503 FilterBank sequencer: the first step falls on the first sample after reset, steps follow Rate", "[filterbank][m3]") {
    // Bass bank, pattern 2 ("Stairs up"): filter c fires on step c of 8. Rate 8 steps per second, Envelope 2 (attack 0.01, decay 1 step).
    const long T = 6000;        // one step at 8 Hz
    for (int c : { 1, 4, 7 }) {
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("voicing", 0.0f); r.set ("pattern", 1.0f); r.set ("rate", 8.0f); r.set ("envelope", 2.0f); r.set ("mix", 1.0f); soloFilter (r, c); r.fx->reset();
        const auto y = r.run (24 * T, { { 0, ts::sine (kBass[c], kFs, 0.1) } });
        ts::Signal s (y[0].size()); for (size_t i = 0; i < s.size(); ++i) s[i] = y[0][i] + y[1][i];
        INFO ("filter " << c + 1);
        for (int cycle = 0; cycle < 2; ++cycle) {
            const long on = (long) (cycle * 8 + c) * T;
            const double loud = ts::rms (s, on + T / 10, on + T / 2);
            REQUIRE (loud > 0.02);
            REQUIRE (ts::rms (s, on + (long) (1.3 * T), on + (long) (7.7 * T)) < 0.02 * loud);     // silent until its next turn
            if (on >= T) REQUIRE (ts::rms (s, on - T / 2, on - T / 10) < 0.02 * loud);
        }
    }
}

TEST_CASE ("T-504 FilterBank envelope shapes follow the Envelope control", "[filterbank][m3]") {
    // Bass pattern 12 ("Pulse"): every filter fires on step 0 of 4. Rate 4 Hz -> step T = 12000 samples, cycle 48000.
    const long T = 12000;
    auto trace = [&] (float envelope) {
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("voicing", 0.0f); r.set ("pattern", 11.0f); r.set ("rate", 4.0f); r.set ("envelope", envelope); r.set ("mix", 1.0f); soloFilter (r, 4); r.fx->reset();
        const auto y = r.run (8 * T, { { 0, ts::sine (kBass[4], kFs, 0.1) } })[0];
        std::vector<double> e;      // RMS in 5 ms windows over the second cycle
        for (long w = 4 * T; w + 240 <= 8 * T; w += 240) e.push_back (ts::rms (y, w, w + 240));
        return e;
    };
    auto at = [&] (const std::vector<double>& e, double steps) { return e[(size_t) std::min ((double) e.size() - 1.0, steps * T / 240.0)]; };
    const double full = 2.0 * std::tanh (0.05) / std::sqrt (2.0);
    const auto e2 = trace (2.0f);       // sharp attack, decay over one step
    REQUIRE (at (e2, 0.05) > 0.8 * full);
    REQUIRE (at (e2, 0.5) == Approx (0.5 * full).epsilon (0.15));
    REQUIRE (at (e2, 1.2) < 0.02 * full);
    const auto e5 = trace (5.0f);       // equal attack and decay of one step each
    REQUIRE (at (e5, 0.5) == Approx (0.5 * full).epsilon (0.15));
    REQUIRE (at (e5, 1.0) > 0.9 * full);
    REQUIRE (at (e5, 1.5) == Approx (0.5 * full).epsilon (0.15));
    REQUIRE (at (e5, 2.2) < 0.02 * full);
    const auto e8 = trace (8.0f);       // smooth attack, abrupt end
    REQUIRE (at (e8, 0.5) == Approx (0.5 * full).epsilon (0.15));
    REQUIRE (at (e8, 0.97) > 0.9 * full);
    REQUIRE (at (e8, 1.1) < 0.02 * full);
    const auto e0 = trace (0.0f);       // choppy: gone after a quarter step
    REQUIRE (at (e0, 0.05) > 0.6 * full);
    REQUIRE (at (e0, 0.35) < 0.02 * full);
}

TEST_CASE ("T-505 FilterBank plays every frozen pattern (external Step clock)", "[filterbank][m3][patterns]") {
    const double fs = 16000.0;
    const long gap = 400;       // samples between step triggers; Rate 32 Hz and Envelope 0: 5-sample attack, 125-sample decay
    for (int p = 0; p < 24; ++p) for (int c = 0; c < 8; ++c) {
        const char* want = kExpectedPatterns[p][c];
        if (p % 12 == 0) continue;      // static patterns are covered by T-501
        const bool mids = p >= 12;
        ts::Rig r (EffectId::FilterBank, fs);
        r.set ("voicing", mids ? 1.0f : 0.0f); r.set ("pattern", (float) (p % 12)); r.set ("rate", 32.0f); r.set ("envelope", 0.0f); r.set ("mix", 1.0f);
        soloFilter (r, c); r.connect ("step_in"); r.fx->reset();
        const int steps = 70;
        std::vector<long> at; for (int s = 0; s < steps; ++s) at.push_back (200 + (long) s * gap);
        const double f = mids ? kMids[c] : (c == 0 ? 60.0 : kBass[c]);
        const auto y = r.run (200 + steps * gap, { { 0, ts::sine (f, fs, 0.2) }, { r.in ("step_in"), ts::pulsesAt (at, 40) } });
        const size_t len = std::strlen (want);
        std::string got, expect;
        for (int s = 0; s < steps; ++s) {
            const double e = ts::rms (y[0], at[(size_t) s] + 10, at[(size_t) s] + 110) + ts::rms (y[1], at[(size_t) s] + 10, at[(size_t) s] + 110);
            got += e > 0.01 ? 'x' : '.'; expect += want[(size_t) s % len];
        }
        INFO ("pattern " << p << " filter " << c + 1);
        REQUIRE (got == expect);
    }
}

TEST_CASE ("T-506 FilterBank Rate, tap tempo (taps are quarter notes, steps are eighths) and Step input", "[filterbank][m3][tap]") {
    // Bass pattern 6 ("Odd even"): filter 5 fires on every second step. Count its bursts.
    auto burstsPerSecond = [] (ts::Rig& r, double seconds) {
        const auto y = r.run (r.samples (seconds), { { 0, ts::sine (kBass[4], r.fs, 0.2) } })[0];
        int n = 0; bool on = false; long first = -1, last = -1;
        for (long w = 0; w + 192 <= (long) y.size(); w += 192) {
            const bool loud = ts::rms (y, w, w + 192) > 0.03;
            if (loud && ! on) { ++n; if (first < 0) first = w; last = w; }
            on = loud;
        }
        return n < 2 ? 0.0 : (double) (n - 1) * r.fs / (double) (last - first);
    };
    auto rig = [] () {
        auto r = std::make_unique<ts::Rig> (EffectId::FilterBank, kFs);
        r->set ("voicing", 0.0f); r->set ("pattern", 5.0f); r->set ("envelope", 0.0f); r->set ("mix", 1.0f); soloFilter (*r, 4);
        return r;
    };
    for (float rate : { 0.5f, 4.0f, 32.0f }) {
        auto r = rig(); r->set ("rate", rate); r->fx->reset();
        INFO ("rate " << rate);
        REQUIRE (burstsPerSecond (*r, rate < 1.0f ? 14.0 : 4.0) == Approx (rate / 2.0).epsilon (0.02));
    }
    auto r = rig(); r->set ("rate", 1.0f); r->connect ("tap_in"); r->fx->reset();
    const long gap = r->samples (0.5);
    r->run (2 * gap + 2000, { { r->in ("tap_in"), ts::pulsesAt ({ 100, 100 + gap, 100 + 2 * gap }, 480) } });
    REQUIRE (burstsPerSecond (*r, 6.0) == Approx (2.0).epsilon (0.02));        // 0.5 s beats -> 4 steps per second -> filter 5 twice a second
    r->set ("rate", 16.0f); r->run (4800);
    REQUIRE (burstsPerSecond (*r, 3.0) == Approx (8.0).epsilon (0.02));        // knob takes over again
}

TEST_CASE ("T-507 FilterBank LFO/Sweep shifts all centre frequencies by up to half an octave", "[filterbank][m3]") {
    REQUIRE (ts::irPeakFreq (filterIr (1.0f, 4, 1.0f), 500.0, 2000.0, kFs) == Approx (1000.0 * std::sqrt (2.0)).epsilon (0.02));
    REQUIRE (ts::irPeakFreq (filterIr (1.0f, 4, 0.0f), 500.0, 2000.0, kFs) == Approx (1000.0 / std::sqrt (2.0)).epsilon (0.02));
    auto wobble = [] (float lfo) {     // spread of the level of a tone at the nominal centre, in dB
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("pattern", 0.0f); r.set ("mix", 1.0f); r.set ("lfo", lfo); r.set ("sweep", 1.0f); soloFilter (r, 4); r.fx->reset();     // LFO at 20 Hz
        const auto y = r.run (48000, { { 0, ts::sine (1000.0, kFs, 0.1) } })[0];
        double lo = 1.0e9, hi = 0.0;
        for (long w = 24000; w + 240 <= 48000; w += 240) { const double v = ts::rms (y, w, w + 240); lo = std::min (lo, v); hi = std::max (hi, v); }
        return ts::dB (hi / lo);
    };
    REQUIRE (wobble (1.0f) > 3.0);
    REQUIRE (wobble (0.0f) < 0.5);
    // with the LFO on, the Sweep control sets its rate from 0.08 Hz to 20 Hz
    ts::Rig r (EffectId::FilterBank, kFs);
    r.set ("pattern", 0.0f); r.set ("mix", 1.0f); r.set ("lfo", 1.0f); r.set ("sweep", 0.5f); soloFilter (r, 4); r.fx->reset();        // 0.08 * sqrt(250) = 1.265 Hz
    const auto y = r.run (r.samples (4.0), { { 0, ts::sine (1000.0, kFs, 0.1) } })[0];
    int peaks = 0; bool above = true;      // the level peaks twice per LFO cycle (the centre passes 1 kHz going up and coming down)
    for (long w = 0; w + 480 <= (long) y.size(); w += 480) {
        const bool a = ts::rms (y, w, w + 480) > 0.06;
        if (a && ! above) ++peaks;
        above = a;
    }
    REQUIRE (peaks >= 9); REQUIRE (peaks <= 11);    // 2 * 1.265 Hz * 4 s = 10.1
}

TEST_CASE ("T-508 FilterBank LFO depth: the centres swing half an octave each way, no further", "[filterbank][m3]") {
    // LFO on at its slowest (0.08 Hz, 12.5 s a cycle): over one cycle the filter at 1 kHz passes fully over tones half an
    // octave away, and only half-way (Q = 5) over a tone three quarters of an octave up.
    auto best = [] (double freq, float lfo) {
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("voicing", 1.0f); r.set ("pattern", 0.0f); r.set ("mix", 1.0f); r.set ("lfo", lfo); r.set ("sweep", lfo > 0.5f ? 0.0f : 0.5f); soloFilter (r, 4); r.fx->reset();
        const long n = r.samples (lfo > 0.5f ? 13.5 : 1.0);
        const auto y = r.run (n, { { 0, ts::sine (freq, kFs, 0.1) } });
        double hi = 0.0;
        for (long w = r.samples (0.5); w + 960 <= n; w += 960) {
            double s = 0.0; for (long i = w; i < w + 960; ++i) { const double v = (double) y[0][(size_t) i] + (double) y[1][(size_t) i]; s += v * v; }
            hi = std::max (hi, std::sqrt (s / 960.0));
        }
        return hi;
    };
    const double full = best (1000.0, 0.0f);
    REQUIRE (full > 0.05);
    REQUIRE (best (1000.0 * std::sqrt (2.0), 1.0f) > 0.9 * full);
    REQUIRE (best (1000.0 / std::sqrt (2.0), 1.0f) > 0.9 * full);
    const double beyond = best (1000.0 * std::pow (2.0, 0.75), 1.0f) / full;
    REQUIRE (beyond > 0.38); REQUIRE (beyond < 0.64);      // 0.50 expected
}

TEST_CASE ("T-509 FilterBank: a new trigger continues the envelope from its present value", "[filterbank][m3]") {
    // Bass pattern 9 ("Polymetre"): filter 1 fires on every step. Envelope 6 rises over two steps, so with a trigger on
    // every step the envelope reaches full level and never falls far; an envelope that restarted from zero at each
    // trigger would never pass half level and would return to zero on every step.
    const long T = 12000, win = 2182;      // window: five cycles of 110 Hz
    auto levels = [&] (float pattern) {
        ts::Rig r (EffectId::FilterBank, kFs);
        r.set ("voicing", 0.0f); r.set ("pattern", pattern); r.set ("rate", 4.0f); r.set ("envelope", 6.0f); r.set ("mix", 1.0f); soloFilter (r, 0); r.fx->reset();
        const auto y = r.run (10 * T, { { 0, ts::sine (kBass[0], kFs, 0.1) } });
        double lo = 1.0e9, hi = 0.0;
        for (long w = 6 * T; w + win <= 10 * T; w += win / 2) {
            double s = 0.0; for (long i = w; i < w + win; ++i) { const double v = (double) y[0][(size_t) i] + (double) y[1][(size_t) i]; s += v * v; }
            lo = std::min (lo, std::sqrt (s / (double) win)); hi = std::max (hi, std::sqrt (s / (double) win));
        }
        return std::pair<double, double> (lo, hi);
    };
    const double full = levels (0.0f).first;        // static pattern: envelope held at 1
    const auto moving = levels (8.0f);
    REQUIRE (full > 0.02);
    REQUIRE (moving.second > 0.85 * full);
    REQUIRE (moving.first > 0.3 * full);            // 0.5 expected
}

// ============================================================ OscBox

TEST_CASE ("T-601 OscBox frequency range 25 Hz-1.6 kHz; Oscillator Out is +/-2.5 V with no input", "[oscbox][m3]") {
    for (auto [hz, seconds] : { std::pair<float, double> { 25.0f, 2.0 }, { 200.0f, 1.0 }, { 1600.0f, 1.0 } }) {
        ts::Rig r (EffectId::OscBox, kFs);
        r.set ("frequency", hz); r.set ("wave", 0.0f); r.fx->reset();
        const long n = r.samples (seconds);
        const auto y = r.run (n);
        const ts::Signal& osc = y[(size_t) r.out ("osc_out")];
        INFO ("frequency " << hz);
        REQUIRE (ts::zcFreq (osc, n / 10, n, kFs) == Approx (hz).epsilon (0.01));
        REQUIRE (ts::peak (osc, n / 10, n) == Approx (0.5).epsilon (0.05));
        REQUIRE (ts::peak (y[0], 0, n) < 1.0e-3);       // the main output is silent without input
    }
}

TEST_CASE ("T-602 OscBox Frequency CV is 1 V per octave and reaches up to 20 kHz", "[oscbox][m3][cv]") {
    auto freq = [] (float cvUnits, float knobHz, double fs) {
        ts::Rig r (EffectId::OscBox, fs);
        r.set ("frequency", knobHz); r.connect ("cv_frequency"); r.fx->reset();
        const long n = r.samples (1.0);
        return ts::zcFreq (r.run (n, { { r.in ("cv_frequency"), ts::constant (cvUnits) } })[(size_t) r.out ("osc_out")], n / 10, n, fs);
    };
    REQUIRE (freq (0.2f, 200.0f, kFs) == Approx (400.0).epsilon (0.005));
    REQUIRE (freq (-0.2f, 200.0f, kFs) == Approx (100.0).epsilon (0.005));
    REQUIRE (freq (0.6f, 200.0f, kFs) == Approx (1600.0).epsilon (0.005));
    REQUIRE (freq (1.0f, 400.0f, 192000.0) == Approx (12800.0).epsilon (0.01));     // beyond the knob range
    REQUIRE (freq (1.0f, 1600.0f, 192000.0) == Approx (20000.0).epsilon (0.01));    // limited to 20 kHz
}

TEST_CASE ("T-603 OscBox Wave morphs triangle, sawtooth, square, narrow pulse", "[oscbox][m3]") {
    auto harmonics = [] (float wave) {
        ts::Rig r (EffectId::OscBox, 96000.0);
        r.set ("frequency", 200.0f); r.set ("wave", wave); r.fx->reset();
        const long n = r.samples (1.0);
        const auto osc = r.run (n)[(size_t) r.out ("osc_out")];
        std::vector<double> h (6, 0.0);
        for (int k = 1; k <= 5; ++k) h[(size_t) k] = ts::toneAmp (osc, n / 2, n, 200.0 * k, 96000.0);
        long pos = 0; for (long i = n / 2; i < n; ++i) pos += osc[(size_t) i] > 0.0f;
        h[0] = (double) pos / (double) (n - n / 2);     // duty cycle
        return h;
    };
    const auto tri = harmonics (0.0f), saw = harmonics (1.0f / 3.0f), sq = harmonics (2.0f / 3.0f), pulse = harmonics (1.0f);
    REQUIRE (ts::dB (tri[3] / tri[1]) == Approx (-19.08).margin (1.5));     // 1/9
    REQUIRE (ts::dB (tri[2] / tri[1]) < -40.0);
    REQUIRE (ts::dB (saw[2] / saw[1]) == Approx (-6.02).margin (1.5));      // 1/2
    REQUIRE (ts::dB (saw[3] / saw[1]) == Approx (-9.54).margin (1.5));      // 1/3
    REQUIRE (ts::dB (sq[3] / sq[1]) == Approx (-9.54).margin (1.5));
    REQUIRE (ts::dB (sq[2] / sq[1]) < -30.0);
    REQUIRE (sq[0] == Approx (0.5).margin (0.02));
    REQUIRE (pulse[0] == Approx (0.1).margin (0.02));                        // 10 % duty at the end of the travel
    REQUIRE (tri[1] == Approx (0.5 * 8.0 / (ts::kPi * ts::kPi)).epsilon (0.05));    // triangle of peak 0.5
}

TEST_CASE ("T-604 OscBox: the oscillator's loudness follows the input envelope; Envelope Out", "[oscbox][m3]") {
    ts::Rig r (EffectId::OscBox, kFs);
    r.set ("frequency", 300.0f); r.set ("wave", 2.0f / 3.0f); r.set ("mix", 1.0f); r.fx->reset();
    const long n = r.samples (1.0);
    const auto y = r.run (2 * n, { { 0, ts::burst (150.0, kFs, 0.5, 0, n) } });
    const ts::Signal& out = y[0]; const ts::Signal& env = y[(size_t) r.out ("env_out")];
    const double e = ts::rms (env, n / 2, n);
    REQUIRE (e == Approx (0.5).epsilon (0.15));                 // envelope of a 0.5 sine reads about 0.5
    REQUIRE (ts::peak (out, n / 2, n) == Approx (e).epsilon (0.2));   // square of peak 1 scaled by the envelope
    REQUIRE (ts::peak (out, 2 * n - 4800, 2 * n) < 1.0e-3);     // input gone: oscillator silent at the main output
    REQUIRE (ts::peak (y[(size_t) r.out ("osc_out")], 2 * n - 4800, 2 * n) > 0.4);   // but always present at Oscillator Out
    REQUIRE (env[(size_t) (0.05 * kFs)] > 0.8 * e);             // attack 5 ms
    REQUIRE (env[(size_t) (n + 0.5 * kFs)] < 0.05 * e);         // release 100 ms
    r.set ("mix", 0.0f); r.run (4800);
    const auto d = r.run (4800, { { 0, ts::sine (150.0, kFs, 0.3) } })[0];
    REQUIRE (ts::rms (d, 0, 4800) == Approx (2.0 * std::tanh (0.15) / std::sqrt (2.0)).epsilon (0.02));   // Mix 0: driven input only
}

TEST_CASE ("T-605 OscBox hard sync locks the oscillator's period to the input", "[oscbox][m3]") {
    const double fs = 44000.0;      // 110 Hz input -> exactly 400 samples per cycle
    auto mismatch = [&] (float sync) {
        ts::Rig r (EffectId::OscBox, fs);
        r.set ("frequency", 517.0f); r.set ("wave", 1.0f / 3.0f); r.set ("sync", sync); r.fx->reset();
        const auto osc = r.run (22000, { { 0, ts::sine (110.0, fs, 0.5) } })[(size_t) r.out ("osc_out")];
        double worst = 0.0; for (long i = 11000; i < 21600; ++i) worst = std::max (worst, (double) std::fabs (osc[(size_t) i] - osc[(size_t) (i + 400)]));
        return worst;
    };
    REQUIRE (mismatch (1.0f) < 2.0e-3);
    REQUIRE (mismatch (0.0f) > 0.1);
}

TEST_CASE ("T-606 OscBox FM Amount and Env Amount move the oscillator frequency", "[oscbox][m3]") {
    auto ratio = [] (float fm) {       // longest / shortest oscillator period while a 20 Hz input modulates it
        ts::Rig r (EffectId::OscBox, kFs);
        r.set ("frequency", 800.0f); r.set ("fm_amount", fm); r.fx->reset();
        const auto osc = r.run (48000, { { 0, ts::sine (20.0, kFs, 0.5) } })[(size_t) r.out ("osc_out")];
        const auto iv = ts::zcIntervals (osc, 24000, 48000);
        double lo = 1.0e9, hi = 0.0; for (double v : iv) { lo = std::min (lo, v); hi = std::max (hi, v); }
        return hi / lo;
    };
    REQUIRE (ratio (0.0f) < 1.01);
    // exponential FM, 2 octaves per core unit at full Amount: +/- 2 * 2*tanh(0.25) = +/-0.98 octave -> ratio 2^1.96 = 3.9
    REQUIRE (ratio (1.0f) == Approx (std::pow (2.0, 4.0 * 2.0 * std::tanh (0.25))).epsilon (0.1));
    REQUIRE (ratio (0.5f) == Approx (std::pow (2.0, 2.0 * 2.0 * std::tanh (0.25))).epsilon (0.1));
    // Env Amount: upward only, Amount octaves at an envelope of 1.0
    auto freq = [] (float amount) {
        ts::Rig r (EffectId::OscBox, kFs);
        r.set ("frequency", 50.0f); r.set ("env_amount", amount); r.fx->reset();
        const long n = 48000;
        const auto y = r.run (n, { { 0, ts::sine (1000.0, kFs, 0.25) } });
        const double env = ts::rms (y[(size_t) r.out ("env_out")], n / 2, n);
        return std::pair<double, double> (ts::zcFreq (y[(size_t) r.out ("osc_out")], n / 2, n, kFs), env);
    };
    const auto f0 = freq (0.0f), f8 = freq (8.0f);
    REQUIRE (f0.first == Approx (50.0).epsilon (0.01));
    REQUIRE (f8.first == Approx (50.0 * std::pow (2.0, 8.0 * f8.second)).epsilon (0.08));
    REQUIRE (f8.first > 150.0);
}

TEST_CASE ("T-607 OscBox: wraps, pulse edges and hard-sync resets are band-limited", "[oscbox][m3]") {
    // 48 kHz; with Sync on, the oscillator is periodic at the 110 Hz input, so its harmonics above 24 kHz fold to
    // frequencies that are not multiples of 110 Hz. Each such alias between 200 Hz and 6 kHz must lie at least 55 dB
    // (40 dB for the narrow pulse) below the strongest of the first 20 harmonics. (Measured in planning: -66 dB or
    // better, -46 dB for the synced narrow pulse; a sync reset quantised to the sample grid gives about -30 dB.)
    const double fs = 48000.0;
    for (double vco : { 517.0, 1333.0 }) for (float wave : { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f }) for (float sync : { 0.0f, 1.0f }) {
        ts::Rig r (EffectId::OscBox, fs);
        r.set ("frequency", (float) vco); r.set ("wave", wave); r.set ("sync", sync); r.fx->reset();
        const long n = 96000;
        const auto osc = r.run (n, { { 0, ts::sine (110.0, fs, 0.5) } })[(size_t) r.out ("osc_out")];
        const double f0 = sync > 0.5f ? 110.0 : vco;
        double ref = 0.0;
        for (int k = 1; k <= 20; ++k) ref = std::max (ref, ts::toneAmp (osc, n / 2, n, k * f0, fs));
        double worst = 0.0;
        for (int k = 1; k * f0 < 3.0 * fs; ++k) {
            const double f = k * f0;
            if (f < fs / 2.0) continue;
            double alias = std::fmod (f, fs); if (alias > fs / 2.0) alias = fs - alias;
            if (alias < 200.0 || alias > 6000.0) continue;
            const double off = std::fmod (alias, f0);
            if (std::min (off, f0 - off) < 8.0) continue;       // lands on a true harmonic: not measurable
            worst = std::max (worst, ts::toneAmp (osc, n / 2, n, alias, fs));
        }
        INFO ("vco " << vco << " wave " << wave << " sync " << sync);
        REQUIRE (ref > 0.1);
        // the 10 % pulse has edges only a few samples apart, where a two-sample correction is less effective
        REQUIRE (ts::dB (worst / ref) < (wave > 0.99f ? -40.0 : -55.0));
    }
}
