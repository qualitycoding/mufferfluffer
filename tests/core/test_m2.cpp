// FROZEN - DO NOT MODIFY. Milestone 2 behaviour: Delay (T-401..), Chorus (T-701..).
#include "test_support.hpp"

using namespace muff;
using Catch::Approx;

namespace {
const double kFs = 48000.0;
// time in seconds from a short burst at `at` to the strongest echo at output `o`
double echoTime (ts::Rig& r, int o, double searchSeconds, double burstHz = 1000.0) {
    r.fx->reset(); r.clock = 0;
    const long at = r.samples (0.05), len = r.samples (0.002);
    const auto y = r.run (at + r.samples (searchSeconds), { { 0, ts::burst (burstHz, r.fs, 0.5, at, at + len) } })[(size_t) o];
    return (double) (ts::argPeak (y, at + len + 2, (long) y.size()) - at) / r.fs - 0.001;   // centre of the burst
}
}

// ============================================================ Delay

TEST_CASE ("T-401 Delay time: Short 40-400 ms, Long 80-800 ms, logarithmic knob", "[delay][m2]") {
    struct Case { float range, knob; double seconds; };
    for (const Case& c : { Case { 0, 0.0f, 0.040 }, Case { 0, 0.5f, 0.1265 }, Case { 0, 1.0f, 0.400 }, Case { 1, 0.0f, 0.080 }, Case { 1, 0.5f, 0.2530 }, Case { 1, 1.0f, 0.800 } }) {
        ts::Rig r (EffectId::Delay, kFs);
        INFO ("range " << c.range << " knob " << c.knob);
        r.set ("range", c.range); r.set ("time", c.knob); r.set ("feedback", 0.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f);
        REQUIRE (echoTime (r, r.out ("delay_out"), 1.0, c.range > 0.5f ? 600.0 : 1000.0) == Approx (c.seconds).epsilon (0.03));
    }
}

TEST_CASE ("T-402 Delay Feedback: loop gain is feedback / 8", "[delay][m2]") {
    ts::Rig r (EffectId::Delay, kFs);
    r.set ("time", 0.5f); r.set ("feedback", 4.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f); r.fx->reset();
    const long d = r.samples (0.04 * std::sqrt (10.0)), len = r.samples (0.02);
    const auto y = r.run (6 * d, { { 0, ts::burst (500.0, kFs, 0.05, 0, len) } })[(size_t) r.out ("delay_out")];
    const double e1 = ts::rms (y, d, d + len), e2 = ts::rms (y, 2 * d, 2 * d + len), e3 = ts::rms (y, 3 * d, 3 * d + len);
    REQUIRE (e1 > 0.02);
    REQUIRE (e2 / e1 == Approx (0.5).epsilon (0.1));
    REQUIRE (e3 / e2 == Approx (0.5).epsilon (0.1));
}

TEST_CASE ("T-403 Delay self-oscillates above Feedback 8 and stays bounded", "[delay][m2]") {
    ts::Rig r (EffectId::Delay, kFs);
    r.set ("time", 0.0f); r.set ("feedback", 10.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f); r.fx->reset();
    const long n = r.samples (10.0);
    const auto y = r.run (n, { { 0, ts::burst (500.0, kFs, 0.3, 0, 960) } })[(size_t) r.out ("delay_out")];
    REQUIRE (ts::rms (y, n - 48000, n) > 0.1);
    REQUIRE (ts::peak (y, 0, n) <= 4.0);
    ts::Rig q (EffectId::Delay, kFs);
    q.set ("time", 0.0f); q.set ("feedback", 6.0f); q.set ("lfo_amount", 0.0f); q.set ("mix", 1.0f); q.fx->reset();
    const auto z = q.run (n, { { 0, ts::burst (500.0, kFs, 0.3, 0, 960) } })[(size_t) q.out ("delay_out")];
    REQUIRE (ts::peak (z, n - 48000, n) < 1.0e-3);
}

TEST_CASE ("T-404 Delay bandwidth: 3.0 kHz in Short, 1.7 kHz in Long, two poles", "[delay][m2]") {
    for (auto [range, fc] : { std::pair<float, double> { 0.0f, 3000.0 }, { 1.0f, 1700.0 } }) {
        auto gain = [&] (double f) {
            ts::Rig r (EffectId::Delay, kFs);
            r.set ("range", range); r.set ("time", 0.0f); r.set ("feedback", 0.0f); r.set ("lfo_amount", 0.0f); r.fx->reset();
            const auto y = r.run (24000, { { 0, ts::sine (f, kFs, 0.05) } })[(size_t) r.out ("delay_out")];
            return ts::dB (ts::toneAmp (y, 12000, 24000, f, kFs) / 0.05);
        };
        INFO ("range " << range);
        REQUIRE (gain (fc / 8.0) == Approx (0.0).margin (0.3));
        REQUIRE (gain (fc) == Approx (-3.0).margin (1.0));
        REQUIRE (gain (2.0 * fc) == Approx (-12.3).margin (2.5));
    }
}

TEST_CASE ("T-405 Delay LFO modulates the delay time; Amount 0 leaves the pitch alone", "[delay][m2]") {
    auto spread = [] (float amount, float wave) {
        ts::Rig r (EffectId::Delay, kFs);
        r.set ("time", 0.5f); r.set ("feedback", 0.0f); r.set ("lfo_rate", 2.0f); r.set ("lfo_amount", amount); r.set ("lfo_wave", wave); r.fx->reset();
        const long n = r.samples (2.0);
        const auto y = r.run (n, { { 0, ts::sine (440.0, kFs, 0.2) } })[(size_t) r.out ("delay_out")];
        const auto iv = ts::zcIntervals (y, r.samples (0.8), n);
        double lo = 1.0e9, hi = 0.0; for (double v : iv) { lo = std::min (lo, v); hi = std::max (hi, v); }
        return hi / lo;
    };
    REQUIRE (spread (0.0f, 0.0f) < 1.005);
    REQUIRE (spread (5.0f, 0.0f) > 1.05);       // sine
    REQUIRE (spread (5.0f, 1.0f) > 1.05);       // triangle
    // the six wave settings give six different results
    std::vector<ts::Signal> outs;
    for (int w = 0; w < 6; ++w) {
        ts::Rig r (EffectId::Delay, kFs);
        r.set ("time", 0.5f); r.set ("feedback", 0.0f); r.set ("lfo_rate", 3.0f); r.set ("lfo_amount", 5.0f); r.set ("lfo_wave", (float) w); r.fx->reset();
        outs.push_back (r.run (r.samples (1.5), { { 0, ts::sine (440.0, kFs, 0.2) } })[(size_t) r.out ("delay_out")]);
    }
    for (int a = 0; a < 6; ++a) for (int b = a + 1; b < 6; ++b) {
        double d = 0.0; for (size_t i = 24000; i < outs[0].size(); ++i) d += std::pow ((double) outs[(size_t) a][i] - outs[(size_t) b][i], 2.0);
        INFO ("waves " << a << " and " << b);
        REQUIRE (std::sqrt (d / (double) (outs[0].size() - 24000)) > 1.0e-3);
    }
}

TEST_CASE ("T-406 Delay tap tempo: three taps set the delay time until the Time knob moves", "[delay][m2][tap]") {
    ts::Rig r (EffectId::Delay, kFs);
    r.set ("time", 0.0f); r.set ("feedback", 0.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f); r.connect ("tap_in"); r.fx->reset();
    const int tapIn = r.in ("tap_in"), o = r.out ("delay_out");
    const long t0 = r.samples (0.1), gap = r.samples (0.3);
    r.run (t0 + 2 * gap + r.samples (0.3), { { tapIn, ts::pulsesAt ({ t0, t0 + gap, t0 + 2 * gap }, 480) } });
    auto measure = [&] () {
        const long at = r.clock + r.samples (0.05), len = r.samples (0.002);
        const long begin = r.clock;
        const auto y = r.run (r.samples (0.7), { { 0, ts::burst (1000.0, kFs, 0.5, at, at + len) } })[(size_t) o];
        return (double) (ts::argPeak (y, at - begin + len + 2, (long) y.size()) - (at - begin)) / kFs - 0.001;
    };
    REQUIRE (measure() == Approx (0.3).epsilon (0.03));
    r.set ("time", 0.5f);                       // moving the knob hands control back
    r.run (r.samples (0.5));
    REQUIRE (measure() == Approx (0.1265).epsilon (0.03));
    // the Tap parameter works like the jack: three rising edges 200 ms apart (after more than 5 s idle: a new run)
    r.run (r.samples (5.5));
    for (int i = 0; i < 3; ++i) { r.set ("tap", 1.0f); r.run (r.samples (0.05)); r.set ("tap", 0.0f); r.run (r.samples (0.15)); }
    r.run (r.samples (0.5));
    REQUIRE (measure() == Approx (0.2).epsilon (0.03));
    // a tapped period outside the range is limited to the range (Short: 40-400 ms)
    r.run (r.samples (5.5));
    const long b = r.clock + 100, g2 = r.samples (0.6);
    r.run (2 * g2 + r.samples (0.6), { { tapIn, ts::pulsesAt ({ b, b + g2, b + 2 * g2 }, 480) } });
    REQUIRE (measure() == Approx (0.4).epsilon (0.03));
}

TEST_CASE ("T-407 Delay bypass: echoes stop, or carry on in Spillover mode", "[delay][m2][bypass]") {
    for (float spill : { 0.0f, 1.0f }) {
        ts::Rig r (EffectId::Delay, kFs);
        r.set ("time", 0.5f); r.set ("feedback", 7.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 0.5f); r.set ("spillover", spill); r.fx->reset();
        r.run (r.samples (0.3), { { 0, ts::burst (500.0, kFs, 0.4, 0, 960) } });
        r.set ("active", 0.0f);
        r.run (r.samples (0.05));               // let the 5 ms switch-over finish
        const long n = r.samples (1.0);
        const auto y = r.run (n);               // no input any more
        INFO ("spillover " << spill);
        if (spill < 0.5f) { REQUIRE (ts::peak (y[0], 0, n) < 1.0e-3); REQUIRE (ts::peak (y[(size_t) r.out ("delay_out")], 0, n) < 1.0e-3); }
        else { REQUIRE (ts::peak (y[0], 0, n) > 0.02); REQUIRE (ts::peak (y[(size_t) r.out ("delay_out")], 0, n) > 0.02); }
        // new input passes dry and is not recorded into the delay line
        const long m = r.samples (2.0);
        r.run (r.samples (8.0));                // let the old echoes die away (loop gain 0.875)
        const auto z = r.run (m, { { 0, ts::burst (500.0, kFs, 0.4, r.clock, r.clock + 960) } });
        REQUIRE (ts::peak (z[0], 0, 960) == Approx (0.4).epsilon (0.02));
        REQUIRE (ts::peak (z[0], 2000, m) < 1.0e-3);
    }
}

TEST_CASE ("T-408 Delay feedback insert, Delay Out and Mix", "[delay][m2]") {
    // Feedback Send carries the delayed signal; a connected, silent Feedback Return breaks the loop.
    ts::Rig r (EffectId::Delay, kFs);
    r.set ("time", 0.0f); r.set ("feedback", 8.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 0.0f); r.set ("output", -6.0f); r.connect ("fb_return"); r.fx->reset();
    const long d = r.samples (0.04), len = 960;
    const auto y = r.run (4 * d, { { 0, ts::burst (500.0, kFs, 0.2, 0, len) } });
    const ts::Signal& send = y[(size_t) r.out ("fb_send")]; const ts::Signal& dly = y[(size_t) r.out ("delay_out")];
    REQUIRE (ts::rms (send, d, d + len) > 0.05);
    REQUIRE (ts::rms (send, 2 * d + 200, 2 * d + len) < 1.0e-4);                 // no second echo
    REQUIRE (ts::rms (dly, d, d + len) / ts::rms (send, d, d + len) == Approx (std::pow (10.0, -6.0 / 20.0)).epsilon (0.01));   // Output scales Delay Out, not the send
    REQUIRE (ts::rms (y[0], d + 200, d + len) < 1.0e-4);                         // Mix 0: no echo at Mix Out, although Delay Out has it
    // an external return signal is what recirculates
    ts::Rig e (EffectId::Delay, kFs);
    e.set ("time", 0.0f); e.set ("feedback", 8.0f); e.set ("lfo_amount", 0.0f); e.connect ("fb_return"); e.fx->reset();
    const auto z = e.run (3 * d, { { e.in ("fb_return"), ts::burst (700.0, kFs, 0.1, 0, len) } })[(size_t) e.out ("delay_out")];
    REQUIRE (ts::rms (z, d, d + len) == Approx (2.0 * std::tanh (0.05) / std::sqrt (2.0)).epsilon (0.08));   // loop gain 1 at Feedback 8
}

TEST_CASE ("T-409 Delay: shortening the time replays the stored signal faster, for one new delay time", "[delay][m2]") {
    // 400 ms line full of a 500 Hz tone; Time is then set to 200 ms. A clocked line reads its 400 ms of contents in
    // 200 ms: the output is an octave up (1 kHz) for about 200 ms, then returns to 500 Hz.
    ts::Rig r (EffectId::Delay, kFs);
    r.set ("range", 0.0f); r.set ("time", 1.0f); r.set ("feedback", 0.0f); r.set ("lfo_amount", 0.0f); r.fx->reset();
    const int o = r.out ("delay_out");
    const auto src = std::map<int, ts::Source> { { 0, ts::sine (500.0, kFs, 0.2) } };
    r.run (r.samples (1.0), src);
    r.set ("time", (float) (std::log10 (200.0 / 40.0)));        // 40 ms * 10^n = 200 ms
    const auto y = r.run (r.samples (0.8), src)[(size_t) o];
    REQUIRE (ts::zcFreq (y, r.samples (0.04), r.samples (0.17), kFs) == Approx (1000.0).epsilon (0.05));
    REQUIRE (ts::zcFreq (y, r.samples (0.35), r.samples (0.75), kFs) == Approx (500.0).epsilon (0.01));
    // lengthening it replays the contents slower: an octave down for about one new delay time (400 ms)
    r.set ("time", 1.0f);
    const auto z = r.run (r.samples (1.2), src)[(size_t) o];
    REQUIRE (ts::zcFreq (z, r.samples (0.06), r.samples (0.34), kFs) == Approx (250.0).epsilon (0.05));
    REQUIRE (ts::zcFreq (z, r.samples (0.7), r.samples (1.1), kFs) == Approx (500.0).epsilon (0.01));
}

// ============================================================ Chorus

TEST_CASE ("T-410 Delay repeats darken: the loop passes through the wet filter", "[delay][m2]") {
    // Long range filters the wet signal at 1.7 kHz (two poles) and feeds the filtered signal back (D-023),
    // so each repeat loses about 3 dB more at 1.7 kHz than at 300 Hz.
    auto step = [] (double f) {
        ts::Rig r (EffectId::Delay, kFs);
        r.set ("range", 1.0f); r.set ("time", 0.5f); r.set ("feedback", 6.0f); r.set ("lfo_amount", 0.0f); r.set ("mix", 1.0f); r.fx->reset();
        const long d = r.samples (0.08 * std::sqrt (10.0)), len = r.samples (0.04);
        const auto y = r.run (4 * d, { { 0, ts::burst (f, kFs, 0.05, 0, len) } })[(size_t) r.out ("delay_out")];
        const double e1 = ts::rms (y, d, d + len), e2 = ts::rms (y, 2 * d, 2 * d + len);
        REQUIRE (e1 > 0.005);
        return ts::dB (e2 / e1);
    };
    const double low = step (300.0), high = step (1700.0);
    REQUIRE (low == Approx (ts::dB (0.75)).margin (1.0));
    REQUIRE (low - high > 1.8);
    REQUIRE (low - high < 4.5);
}

TEST_CASE ("T-411 Delay LFO depth: Amount 10 swings the delay time one octave each way", "[delay][m2]") {
    // Square LFO at 0.5 Hz: the delay sits at twice and at half the knob's time for a second each.
    // Bursts sent at eight offsets across one LFO cycle: the settled halves give the longest and the shortest arrival.
    const double d0 = 0.04 * std::sqrt (10.0);
    double lo = 1.0e9, hi = 0.0;
    for (int k = 0; k < 8; ++k) {
        ts::Rig r (EffectId::Delay, kFs);
        r.set ("time", 0.5f); r.set ("feedback", 0.0f); r.set ("lfo_rate", 0.5f); r.set ("lfo_amount", 10.0f); r.set ("lfo_wave", 2.0f); r.set ("mix", 1.0f); r.fx->reset();
        const long t0 = r.samples (2.0 + 0.25 * k), len = r.samples (0.005), n = t0 + r.samples (0.6);
        const auto y = r.run (n, { { 0, ts::burst (1000.0, kFs, 0.2, t0, t0 + len) } })[(size_t) r.out ("delay_out")];
        long first = -1;
        for (long i = t0; i < n; ++i) if (std::fabs (y[(size_t) i]) > 0.02f) { first = i; break; }
        REQUIRE (first > 0);
        const double d = (double) (first - t0) / kFs;
        lo = std::min (lo, d); hi = std::max (hi, d);
    }
    REQUIRE (hi == Approx (2.0 * d0).epsilon (0.05));
    REQUIRE (lo == Approx (0.5 * d0).epsilon (0.05));
}

TEST_CASE ("T-701 Chorus delay ranges: Flange 0.6-10 ms, Chorus 5-50 ms", "[chorus][m2]") {
    const double fs = 192000.0;
    struct Case { float range, knob; double seconds; };
    for (const Case& c : { Case { 0, 0.0f, 0.0006 }, Case { 0, 1.0f, 0.010 }, Case { 0, 0.5f, std::sqrt (0.0006 * 0.010) }, Case { 1, 0.0f, 0.005 }, Case { 1, 1.0f, 0.050 }, Case { 1, 0.5f, std::sqrt (0.005 * 0.050) } }) {
        ts::Rig r (EffectId::Chorus, fs);
        INFO ("range " << c.range << " knob " << c.knob);
        r.set ("range", c.range); r.set ("time", c.knob); r.set ("feedback", 0.0f); r.set ("lfo_amount", 0.0f); r.fx->reset();
        const auto y = r.run (r.samples (0.08), { { 0, ts::impulseAt (100, 0.2f) } })[(size_t) r.out ("fb_send")];
        const double t = (double) (ts::argPeak (y, 101, (long) y.size()) - 100) / fs;
        REQUIRE (t == Approx (c.seconds).epsilon (0.05));
    }
}

TEST_CASE ("T-702 Chorus stereo: the right output carries the delayed signal inverted", "[chorus][m2]") {
    ts::Rig r (EffectId::Chorus, kFs);
    r.set ("mix", 0.5f); r.set ("feedback", 0.3f); r.fx->reset();
    const long n = 24000;
    const auto y = r.run (n, { { 0, ts::sine (440.0, kFs, 0.2) } });
    const ts::Signal& L = y[(size_t) r.out ("out_l")]; const ts::Signal& R = y[(size_t) r.out ("out_r")]; const ts::Signal& wet = y[(size_t) r.out ("fb_send")];
    double sumErr = 0.0, difErr = 0.0;
    for (long i = 4800; i < n; ++i) {
        const float dry = 2.0f * std::tanh (0.5f * (float) (0.2 * std::sin (2.0 * ts::kPi * 440.0 * i / kFs)));
        sumErr = std::max (sumErr, (double) std::fabs (L[(size_t) i] + R[(size_t) i] - dry));           // wet cancels in the sum
        difErr = std::max (difErr, (double) std::fabs (L[(size_t) i] - R[(size_t) i] - wet[(size_t) i]));
    }
    REQUIRE (sumErr < 1.0e-4);
    REQUIRE (difErr < 1.0e-4);
    REQUIRE (ts::rms (wet, 4800, n) > 0.05);
    r.set ("mix", 0.0f); r.run (4800);
    const auto dry = r.run (4800, { { 0, ts::sine (440.0, kFs, 0.2) } });
    double lr = 0.0; for (size_t i = 0; i < dry[0].size(); ++i) lr = std::max (lr, (double) std::fabs (dry[0][i] - dry[1][i]));
    REQUIRE (lr < 1.0e-5);                      // Mix 0: both outputs dry
}

TEST_CASE ("T-703 Chorus feedback is bipolar: all harmonics of 1/delay, or odd harmonics an octave lower", "[chorus][m2]") {
    auto gains = [] (float fb) {
        ts::Rig r (EffectId::Chorus, kFs);
        r.set ("range", 1.0f); r.set ("time", 0.0f); r.set ("feedback", fb); r.set ("lfo_amount", 0.0f); r.fx->reset();     // 5 ms -> 200 Hz
        const auto h = r.run (r.samples (1.0), { { 0, ts::impulseAt (0, 0.01f) } })[(size_t) r.out ("fb_send")];
        return std::pair<double, double> (ts::dB (ts::irGain (h, 100.0, kFs, 0.01)), ts::dB (ts::irGain (h, 200.0, kFs, 0.01)));
    };
    const auto pos = gains (0.5f), neg = gains (-0.5f), none = gains (0.0f);
    REQUIRE (pos.second - pos.first > 6.0);      // loop gain 0.625: +8.5 dB against -4.2 dB
    REQUIRE (neg.first - neg.second > 6.0);
    REQUIRE (std::fabs (none.first - none.second) < 0.5);
    REQUIRE (pos.second == Approx (ts::dB (1.0 / (1.0 - 0.625))).margin (1.5));
}

TEST_CASE ("T-704 Chorus self-oscillates from |Feedback| 0.8 and decays below it", "[chorus][m2]") {
    for (float fb : { 1.0f, -1.0f }) {
        ts::Rig r (EffectId::Chorus, kFs);
        r.set ("range", 1.0f); r.set ("time", 0.5f); r.set ("feedback", fb); r.set ("lfo_amount", 0.0f); r.fx->reset();
        const long n = r.samples (6.0);
        const auto y = r.run (n, { { 0, ts::burst (300.0, kFs, 0.3, 0, 960) } })[(size_t) r.out ("fb_send")];
        REQUIRE (ts::rms (y, n - 24000, n) > 0.05);
        REQUIRE (ts::peak (y, 0, n) <= 4.0);
    }
    ts::Rig q (EffectId::Chorus, kFs);
    q.set ("range", 1.0f); q.set ("time", 0.5f); q.set ("feedback", 0.6f); q.set ("lfo_amount", 0.0f); q.fx->reset();
    const float tail = q.fx->tailSeconds();
    REQUIRE (tail < 10.0f);
    const long n = 960 + q.samples (tail) + 24000;
    const auto z = q.run (n, { { 0, ts::burst (300.0, kFs, 0.3, 0, 960) } })[(size_t) q.out ("fb_send")];
    REQUIRE (ts::peak (z, 960 + q.samples (tail), n) < 1.0e-3);
}

TEST_CASE ("T-705 Chorus LFO sweeps the delay inside the range; six wave settings differ", "[chorus][m2]") {
    // Amount 1 from the centre of the range sweeps the whole range and never leaves it: Chorus 5-50 ms.
    ts::Rig r (EffectId::Chorus, kFs);
    r.set ("range", 1.0f); r.set ("time", 0.5f); r.set ("feedback", 0.0f); r.set ("lfo_rate", 1.0f); r.set ("lfo_amount", 1.0f); r.set ("lfo_wave", 2.0f); r.fx->reset();   // square, 1 Hz
    // square LFO: the delay sits at 50 ms for half a second, then at 5 ms
    auto delayAt = [&] (double atSeconds) {
        r.fx->reset(); r.clock = 0;
        const long at = r.samples (atSeconds);
        const auto y = r.run (at + r.samples (0.08), { { 0, ts::impulseAt (at, 0.2f) } })[(size_t) r.out ("fb_send")];
        return (double) (ts::argPeak (y, at + 1, (long) y.size()) - at) / kFs;
    };
    REQUIRE (delayAt (0.25) == Approx (0.050).epsilon (0.05));
    REQUIRE (delayAt (0.75) == Approx (0.005).epsilon (0.05));
    std::vector<ts::Signal> outs;
    for (int w = 0; w < 6; ++w) {
        ts::Rig q (EffectId::Chorus, kFs);
        q.set ("lfo_rate", 3.0f); q.set ("lfo_amount", 0.5f); q.set ("lfo_wave", (float) w); q.fx->reset();
        outs.push_back (q.run (q.samples (1.5), { { 0, ts::sine (440.0, kFs, 0.2) } })[(size_t) q.out ("fb_send")]);
    }
    for (int a = 0; a < 6; ++a) for (int b = a + 1; b < 6; ++b) {
        double d = 0.0; for (size_t i = 24000; i < outs[0].size(); ++i) d += std::pow ((double) outs[(size_t) a][i] - outs[(size_t) b][i], 2.0);
        INFO ("waves " << a << " and " << b);
        REQUIRE (std::sqrt (d / (double) (outs[0].size() - 24000)) > 1.0e-3);
    }
}

TEST_CASE ("T-706 Chorus tap tempo sets one LFO cycle per tapped beat; the Rate meter blinks at the LFO rate", "[chorus][m2][tap]") {
    ts::Rig r (EffectId::Chorus, kFs);
    r.set ("lfo_rate", 5.0f); r.connect ("tap_in"); r.fx->reset();
    const int rate = r.meter ("rate");
    auto blinkHz = [&] (double seconds) {
        int edges = 0; long first = -1, last = -1; float prev = r.fx->getMeter (rate);
        float x[kMaxPorts] {}, y[kMaxPorts];
        for (long i = 0; i < r.samples (seconds); ++i) {
            r.fx->process (x, y); ++r.clock;
            const float m = r.fx->getMeter (rate);
            if (m > 0.5f && prev <= 0.5f) { if (first < 0) first = i; last = i; ++edges; }
            prev = m;
        }
        return edges < 2 ? 0.0 : (double) (edges - 1) * kFs / (double) (last - first);
    };
    REQUIRE (blinkHz (2.0) == Approx (5.0).epsilon (0.02));
    const long t0 = r.clock + 100, gap = r.samples (0.5);
    r.run (2 * gap + 1000, { { r.in ("tap_in"), ts::pulsesAt ({ t0, t0 + gap, t0 + 2 * gap }, 480) } });
    REQUIRE (blinkHz (4.0) == Approx (2.0).epsilon (0.02));
    r.set ("lfo_rate", 8.0f);                   // moving the knob hands control back
    r.run (4800);
    REQUIRE (blinkHz (2.0) == Approx (8.0).epsilon (0.02));
}

TEST_CASE ("T-707 Chorus feedback insert and wet bandwidth", "[chorus][m2]") {
    ts::Rig r (EffectId::Chorus, kFs);
    r.set ("range", 1.0f); r.set ("time", 0.0f); r.set ("feedback", 1.0f); r.set ("lfo_amount", 0.0f); r.connect ("fb_return"); r.fx->reset();
    const auto y = r.run (r.samples (0.1), { { 0, ts::impulseAt (0, 0.2f) } })[(size_t) r.out ("fb_send")];
    REQUIRE (ts::peak (y, 0, r.samples (0.008)) > 0.02);                    // first pass at 5 ms
    REQUIRE (ts::peak (y, r.samples (0.008), (long) y.size()) < 1.0e-4);    // silent return: nothing recirculates
    // wet bandwidth follows the delay time: min(16 kHz, 200 / delay). 50 ms -> 4 kHz, two poles
    auto gain = [] (double f) {
        ts::Rig q (EffectId::Chorus, 96000.0);
        q.set ("range", 1.0f); q.set ("time", 1.0f); q.set ("feedback", 0.0f); q.set ("lfo_amount", 0.0f); q.fx->reset();
        const auto z = q.run (48000, { { 0, ts::sine (f, 96000.0, 0.05) } })[(size_t) q.out ("fb_send")];
        return ts::dB (ts::toneAmp (z, 24000, 48000, f, 96000.0) / 0.05);
    };
    REQUIRE (gain (500.0) == Approx (0.0).margin (0.3));
    REQUIRE (gain (4000.0) == Approx (-3.0).margin (1.0));
    REQUIRE (gain (8000.0) == Approx (-12.3).margin (2.5));
}

TEST_CASE ("T-708 Chorus repeats darken: the loop passes through the wet filter", "[chorus][m2]") {
    // Chorus range at the longest time: 50 ms, wet filter at 4 kHz (D-026, D-036); feedback 0.6 gives loop gain 0.75
    auto step = [] (double f) {
        ts::Rig r (EffectId::Chorus, 96000.0);
        r.set ("range", 1.0f); r.set ("time", 1.0f); r.set ("feedback", 0.6f); r.set ("lfo_amount", 0.0f); r.fx->reset();
        const long d = r.samples (0.05), len = r.samples (0.03);
        const auto y = r.run (4 * d, { { 0, ts::burst (f, 96000.0, 0.05, 0, len) } })[(size_t) r.out ("fb_send")];
        const double e1 = ts::rms (y, d, d + len), e2 = ts::rms (y, 2 * d, 2 * d + len);
        REQUIRE (e1 > 0.005);
        return ts::dB (e2 / e1);
    };
    const double low = step (500.0), high = step (4000.0);
    REQUIRE (low == Approx (ts::dB (0.75)).margin (1.0));
    REQUIRE (low - high > 1.8);
    REQUIRE (low - high < 4.5);
}
