// ORACLE implementation of muff/core.hpp (planning spike, R6).
// Purpose: prove that the frozen tests in tests/core are satisfiable by an implementation written against the
// public header and the decisions D-011..D-037 alone. It is deliberately plain (one file, no optimisation, no
// oversampling). It is NOT the deliverable: the implementer writes core/src under the plan's steps, review and
// mutation gates. SPDX-License-Identifier: AGPL-3.0-only
#include "muff/core.hpp"
#include "muff/blocks.hpp"
#include "muff/adapters.hpp"

#include <algorithm>
#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace muff {
namespace {
#include "generated_tables.inc"

constexpr double kPi = 3.14159265358979323846;
constexpr int kMaxParams = 24;

inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float softclip (float x) { return softClip (x); }
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

struct Smooth {
    float y = 0.0f, a = 1.0f;
    void set (double fs, double ms) { a = (float) (1.0 - std::exp (-1.0 / (0.001 * ms * fs))); }
    float next (float t) { y += a * (t - y); return y; }
};

// Follower of the rectified signal with separate attack and release one-poles, no scaling (D-030, D-035).
struct EnvFollower {
    float y = 0.0f, aAtt = 1.0f, aRel = 1.0f;
    void set (double fs, double attMs, double relMs) {
        aAtt = (float) (1.0 - std::exp (-1.0 / (0.001 * attMs * fs)));
        aRel = (float) (1.0 - std::exp (-1.0 / (0.001 * relMs * fs)));
    }
    float next (float x) {
        const float r = std::fabs (x);
        y += (r > y ? aAtt : aRel) * (r - y);
        return y;
    }
};

struct Lfo {
    double ph = 0.0;
    float held = 0.0f;
    std::uint32_t rng = 0x12345678u;
    void reset() { ph = 0.0; held = 0.0f; rng = 0x12345678u; }
    // wave: 0 sine, 1 triangle, 2 square, 3 sawtooth (rising), 4 ramp (falling), 5 random step
    float next (int wave, double freq, double fs) {
        ph += freq / fs;
        if (ph >= 1.0) {
            ph -= std::floor (ph);
            rng = rng * 1664525u + 1013904223u;
            held = (float) ((rng >> 8) * (1.0 / 8388608.0) - 1.0);
        }
        return lfoShape ((LfoWave) wave, (float) ph, held);
    }
};

// Zavalishin trapezoidal state-variable filter.
struct Svf {
    float s1 = 0.0f, s2 = 0.0f;
    void reset() { s1 = s2 = 0.0f; }
    void tick (float x, float g, float k, float& lp, float& bp) {
        const float hp = (x - (k + g) * s1 - s2) / (1.0f + k * g + g * g);
        const float v1 = g * hp; bp = v1 + s1; s1 = bp + v1;
        const float v2 = g * bp; lp = v2 + s2; s2 = lp + v2;
    }
};
inline float tanG (double fc, double fs) { return (float) std::tan (kPi * std::min (fc, 0.45 * fs) / fs); }

struct Schmitt {
    bool high = false;
    bool rising (float v) {
        if (! high && v >= 0.2f) { high = true; return true; }
        if (high && v < 0.02f) high = false;
        return false;
    }
};

struct Clock { long long n = 0; void reset() { n = 0; } };

// Clocked delay line (D-033): models a bucket-brigade line whose transit time is `d` seconds at the present clock.
// Every stored sample carries the clock phase at which it entered; the output is the signal that entered one
// whole unit of phase ago. A change of d therefore replays what is already in the line faster or slower.
struct ClockedLine {
    std::vector<float> buf;
    std::vector<double> ph;
    long long k = 0, r = 0;     // samples written so far; read index (absolute)
    double phase = 0.0;
    void size (int n) { buf.assign ((size_t) std::max (n, 8), 0.0f); ph.assign (buf.size(), 0.0); clear(); }
    // after clear() the line holds one silent sample just before the start, so the first real sample can be interpolated towards
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); std::fill (ph.begin(), ph.end(), 0.0); ph[0] = -1.0; k = 1; r = 0; phase = 0.0; }
    float read() {
        const long long n = (long long) buf.size();
        const double target = phase - 1.0;
        if (k < 2 || target < ph[(size_t) (std::max (0LL, k - n + 1) % n)]) return 0.0f;
        if (r < k - n + 1) r = k - n + 1;
        while (r + 1 < k - 1 && ph[(size_t) ((r + 1) % n)] <= target) ++r;
        const double p0 = ph[(size_t) (r % n)], p1 = ph[(size_t) ((r + 1) % n)];
        if (target < p0) return 0.0f;
        const double fr = p1 > p0 ? std::min (1.0, (target - p0) / (p1 - p0)) : 0.0;
        return (float) ((1.0 - fr) * buf[(size_t) (r % n)] + fr * buf[(size_t) ((r + 1) % n)]);
    }
    void write (float v, double delaySeconds, double fs) {
        const long long n = (long long) buf.size();
        const double inc = 1.0 / std::max (1.0, delaySeconds * fs);
        if (k == 1) ph[0] = phase - inc;        // the silent sample "before the start" sits one clock step earlier
        buf[(size_t) (k % n)] = v; ph[(size_t) (k % n)] = phase; ++k;
        phase += inc;
        if (phase > 1.0e6) { for (double& q : ph) q -= 1.0e6; phase -= 1.0e6; }
    }
};

// Flush-to-zero / denormals-are-zero for the duration of one process() call (D-012: no slow-down on decaying signals).
struct NoDenormals {
#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
    unsigned old;
    NoDenormals() : old (_mm_getcsr()) { _mm_setcsr (old | 0x8040u); }
    ~NoDenormals() { _mm_setcsr (old); }
#elif defined(__aarch64__)
    unsigned long long old;
    NoDenormals() { __asm__ volatile ("mrs %0, fpcr" : "=r" (old)); const unsigned long long v = old | (1ull << 24); __asm__ volatile ("msr fpcr, %0" : : "r" (v)); }
    ~NoDenormals() { __asm__ volatile ("msr fpcr, %0" : : "r" (old)); }
#else
    NoDenormals() {}
#endif
};

class Base : public Effect {
public:
    explicit Base (EffectId id) : I (kEffectInfos[(int) id]) {
        for (int p = 0; p < I.numParams; ++p) { target[p] = I.params[p].def; cvFor[p] = -1; }
        for (int i = 0; i < I.numInputs; ++i) {
            conn[i] = (i == 0);
            if (I.inputs[i].kind == PortKind::CvNormalised && I.inputs[i].param >= 0) cvFor[I.inputs[i].param] = i;
        }
    }
    const EffectInfo& info() const noexcept override { return I; }
    void prepare (double sampleRate) override {
        if (! std::isfinite (sampleRate) || sampleRate < kMinSampleRate || sampleRate > kMaxSampleRate)
            throw std::invalid_argument ("sample rate out of range");
        fs = sampleRate;
        for (int p = 0; p < I.numParams; ++p) sm[p].set (fs, 5.0);
        act.set (fs, 5.0); meterDecay = (float) std::exp (-1.0 / (0.1 * fs));
        doPrepare();
        prepared = true;
        reset();
    }
    void reset() noexcept override {
        for (int p = 0; p < I.numParams; ++p) sm[p].y = toNormalised (I.params[p], target[p]);
        act.y = activeParam >= 0 ? target[activeParam] : 1.0f;
        for (float& m : meters) m = 0.0f;
        doReset();
    }
    void setParam (int index, float value) noexcept override {
        if (index < 0 || index >= I.numParams || ! std::isfinite (value)) return;
        const ParamSpec& s = I.params[index];
        value = clampf (value, s.min, s.max);
        if (s.taper == Taper::Stepped) value = std::round (value);
        target[index] = value;
    }
    float getParam (int index) const noexcept override { return (index < 0 || index >= I.numParams) ? 0.0f : target[index]; }
    void setInputConnected (int input, bool c) noexcept override { if (input >= 0 && input < I.numInputs) conn[input] = c; }
    void process (const float* in, float* out) noexcept override {
        if (! prepared) { for (int o = 0; o < I.numOutputs; ++o) out[o] = 0.0f; return; }
        const NoDenormals noDenormals;
        float x[kMaxPorts];
        for (int i = 0; i < I.numInputs; ++i) x[i] = (conn[i] && std::isfinite (in[i])) ? clampf (in[i], -16.0f, 16.0f) : 0.0f;
        tick (x, out);
        bool bad = false;
        for (int o = 0; o < I.numOutputs; ++o) bad = bad || ! std::isfinite (out[o]);
        if (bad) { reset(); for (int o = 0; o < I.numOutputs; ++o) out[o] = 0.0f; return; }
        for (int o = 0; o < I.numOutputs; ++o) out[o] = clampf (out[o], -kOutputLimit, kOutputLimit);
    }
    float getMeter (int index) const noexcept override { return (index < 0 || index >= I.numMeters) ? 0.0f : clampf (meters[index], 0.0f, 1.0f); }
    int latencySamples() const noexcept override { return 0; }

protected:
    virtual void doPrepare() = 0;
    virtual void doReset() noexcept = 0;
    virtual void tick (const float* x, float* out) noexcept = 0;

    int P (const char* id) const { return findParam (I, id); }
    int In (const char* id) const { return findInput (I, id); }
    // normalised position of a continuous parameter including its CV input, smoothed
    float norm (int p, const float* x) {
        float n = toNormalised (I.params[p], target[p]);
        if (cvFor[p] >= 0) n = clampf (n + I.inputs[cvFor[p]].depth * x[cvFor[p]], 0.0f, 1.0f);
        return sm[p].next (n);
    }
    float val (int p, const float* x) { return fromNormalised (I.params[p], norm (p, x)); }
    int step (int p) const { return (int) target[p]; }
    float activeGain() { return act.next (activeParam >= 0 ? target[activeParam] : 1.0f); }
    void levelMeter (float v) { meters[0] = std::max (std::fabs (v), meters[0] * meterDecay); }

    const EffectInfo& I;
    double fs = 0.0;
    bool prepared = false;
    float target[kMaxParams] {};
    int cvFor[kMaxParams] {};
    bool conn[kMaxPorts] {};
    Smooth sm[kMaxParams];
    Smooth act;
    int activeParam = -1;
    float meters[4] {};
    float meterDecay = 0.0f;
};

// ---------------------------------------------------------------- Lowpass (D-020, D-030)
class Lowpass final : public Base {
public:
    Lowpass() : Base (EffectId::Lowpass) {
        pDrive = P ("drive"); pCut = P ("cutoff"); pRes = P ("resonance"); pAmt = P ("env_amount"); pSpeed = P ("env_speed");
        pPoles = P ("poles"); pMix = P ("mix"); activeParam = P ("active");
    }
    float tailSeconds() const noexcept override { return target[pRes] >= 8.0f ? 1.0e9f : 1.0f; }
private:
    void doPrepare() override { fast.set (fs, 2.0, 50.0); slow.set (fs, 20.0, 300.0); }
    void doReset() noexcept override { s[0] = s[1] = s[2] = s[3] = 0.0f; fast.y = slow.y = 0.0f; rng = 0x9E3779B9u; }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        const float eFast = fast.next (x), eSlow = slow.next (x);
        const float env = step (pSpeed) == 1 ? eFast : eSlow;
        const double fcRaw = (double) val (pCut, in) * std::pow (2.0, (double) (val (pAmt, in) * env));
        const double fc = std::min (std::max (fcRaw, 10.0), 20000.0);
        const float fade = clampf ((float) ((20000.0 - fcRaw) / 4000.0), 0.0f, 1.0f);    // self-oscillation leaves the audible range instead of whistling at the limit
        const float g = tanG (fc, fs), G = g / (1.0f + g), k = 0.5f * val (pRes, in) * fade;
        rng = rng * 1664525u + 1013904223u;
        const float noise = ((float) (rng >> 8) * (1.0f / 8388608.0f) - 1.0f) * 1.0e-5f; // seeds self-oscillation
        const float xin = x + noise;
        const float b = 1.0f - G;
        const float S = b * (G * G * G * s[0] + G * G * s[1] + G * s[2] + s[3]);
        const float G4 = G * G * G * G;
        float u = (xin - k * S) / (1.0f + k * G4);
        const float y4est = G4 * u + S;
        u = xin - k * softclip (y4est);
        float y = u, y2 = 0.0f;
        for (int i = 0; i < 4; ++i) {
            const float v = (y - s[i]) * G;
            y = v + s[i]; s[i] = y + v;
            if (i == 1) y2 = y;
        }
        const float wet = step (pPoles) == 1 ? y : y2;
        const float m = norm (pMix, in), a = activeGain();
        out[0] = a * ((1.0f - m) * x + m * wet) + (1.0f - a) * x;   // Drive stays in the bypass path (D-020)
        out[1] = std::min (env, 2.0f);
        meters[1] = env; meters[2] = a;
    }
    int pDrive, pCut, pRes, pAmt, pSpeed, pPoles, pMix;
    float s[4] {};
    EnvFollower fast, slow;
    std::uint32_t rng = 0x9E3779B9u;
};

// ---------------------------------------------------------------- RingMod (D-021, D-031)
class RingMod final : public Base {
public:
    RingMod() : Base (EffectId::RingMod) {
        pDrive = P ("drive"); pFreq = P ("frequency"); pRange = P ("range"); pRate = P ("lfo_rate"); pAmt = P ("lfo_amount");
        pWave = P ("lfo_wave"); pMix = P ("mix"); activeParam = P ("active"); iCar = In ("carrier_in");
    }
    float tailSeconds() const noexcept override { return 0.1f; }
private:
    void doPrepare() override {}
    void doReset() noexcept override { ph = 0.0; lfo.reset(); }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        const float l = lfo.next (step (pWave) == 0 ? 0 : 2, val (pRate, in), fs);
        const float n = norm (pFreq, in);
        const double base = step (pRange) == 0 ? 0.6 * std::pow (80.0 / 0.6, (double) n) : 30.0 * std::pow (4000.0 / 30.0, (double) n);
        const double f = std::min (base * std::pow (2.0, 0.5 * (double) val (pAmt, in) * l), 0.45 * fs);
        ph += f / fs; if (ph >= 1.0) ph -= std::floor (ph);
        const float osc = (float) std::sin (2.0 * kPi * ph);
        const float car = conn[iCar] ? clampf (in[iCar] * 7.0711f, -2.0f, 2.0f) : osc;   // 0.5 V RMS sine = full modulation
        const float wet = x * car;
        const float m = norm (pMix, in), a = activeGain();
        out[0] = a * ((1.0f - m) * x + m * wet) + (1.0f - a) * x;
        out[1] = 0.25f * osc;   // +/-1.25 V
        out[2] = 0.3f * l;      // +/-1.5 V
        meters[1] = 0.5f + 0.5f * l; meters[2] = a;
    }
    int pDrive, pFreq, pRange, pRate, pAmt, pWave, pMix, iCar;
    double ph = 0.0;
    Lfo lfo;
};

// ---------------------------------------------------------------- Phaser (D-022, D-032)
class Phaser final : public Base {
public:
    Phaser() : Base (EffectId::Phaser) {
        pDrive = P ("drive"); pOut = P ("output"); pSweep = P ("sweep"); pRes = P ("resonance"); pRate = P ("lfo_rate");
        pRange = P ("lfo_range"); pAmt = P ("lfo_amount"); pStages = P ("stages"); activeParam = P ("active"); iSweep = In ("sweep_in");
    }
    float tailSeconds() const noexcept override { return 0.5f; }
private:
    void doPrepare() override {}
    void doReset() noexcept override { for (int i = 0; i < 12; ++i) { x1[i] = 0.0f; y1[i] = 0.0f; } fbState = 0.0f; lfo.reset(); }
    float coeff (double octaves) const {
        const double fc = std::min (std::max (1100.0 * std::pow (2.0, octaves), 20.0), 0.45 * fs);
        const double t = std::tan (kPi * fc / fs);
        return (float) ((t - 1.0) / (t + 1.0));
    }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        const float rn = norm (pRate, in);
        const double rate = (step (pRange) == 0 ? 0.01 : 1.0) * std::pow (250.0, (double) rn);
        const float l = lfo.next (1, rate, fs);
        const double centre = 6.0 * ((double) val (pSweep, in) / 10.0 - 0.5);
        const float amt = norm (pAmt, in);
        const bool twelve = step (pStages) == 1;
        const float a1 = coeff (centre + 3.0 * amt * l);
        const float a2 = (twelve && conn[iSweep]) ? coeff (centre + 6.0 * clampf (in[iSweep], -0.5f, 0.5f)) : a1;
        const float fb = 0.09f * val (pRes, in);
        const float u = x + fb * std::tanh (fbState);
        float y = u;
        const int n = twelve ? 12 : 6;
        for (int i = 0; i < n; ++i) {
            const float a = i < 6 ? a1 : a2;
            const float o = a * y + x1[i] - a * y1[i];
            x1[i] = y; y1[i] = o; y = o;
        }
        fbState = y;
        const float lvl = dbToGain (val (pOut, in)), ag = activeGain();
        out[0] = ag * lvl * 0.5f * (u + y) + (1.0f - ag) * in[0];
        out[1] = ag * lvl * 0.5f * (u - y) + (1.0f - ag) * in[0];
        out[2] = 0.56f * l;     // +/-2.8 V
        meters[1] = 0.5f + 0.5f * l; meters[2] = ag;
    }
    int pDrive, pOut, pSweep, pRes, pRate, pRange, pAmt, pStages, iSweep;
    float x1[12] {}, y1[12] {};
    float fbState = 0.0f;
    Lfo lfo;
};

// ---------------------------------------------------------------- Delay (D-023, D-033)
class Delay final : public Base {
public:
    Delay() : Base (EffectId::Delay) {
        pDrive = P ("drive"); pOut = P ("output"); pTime = P ("time"); pRange = P ("range"); pFb = P ("feedback"); pRate = P ("lfo_rate");
        pAmt = P ("lfo_amount"); pWave = P ("lfo_wave"); pMix = P ("mix"); pSpill = P ("spillover"); pTap = P ("tap");
        activeParam = P ("active"); iRet = In ("fb_return"); iTap = In ("tap_in");
    }
    float tailSeconds() const noexcept override {
        const float g = target[pFb] / 8.0f;
        if (g >= 1.0f) return 1.0e9f;
        const float d = 0.8f;   // longest delay of either range
        const float t = g <= 0.001f ? d + 0.1f : d * (1.0f + 6.908f / -std::log (g)) + 0.1f;
        return t > 60.0f ? 1.0e9f : t;
    }
private:
    void doPrepare() override { line.size ((int) (0.85 * fs) + 8); }
    void doReset() noexcept override { line.clear(); lp.reset(); lfo.reset(); tap.reset(); clock.reset(); trig = Schmitt(); lastTapParam = target[pTap]; tapped = false; }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        clock.n++;
        const bool edge = trig.rising (in[iTap]) || (target[pTap] > 0.5f && lastTapParam <= 0.5f);
        lastTapParam = target[pTap];
        const float tn = norm (pTime, in);
        if (edge && tap.tap ((double) clock.n / fs)) { tapped = true; tapNorm = tn; }
        if (tapped && std::fabs (tn - tapNorm) > 0.01f) tapped = false;
        const bool lng = step (pRange) == 1;
        const double lo = lng ? 0.08 : 0.04, hi = lng ? 0.8 : 0.4;
        double d = tapped ? tap.periodSeconds() : lo * std::pow (10.0, (double) tn);
        const float l = lfo.next (step (pWave), val (pRate, in), fs);
        d *= std::pow (2.0, (double) (l * val (pAmt, in) / 10.0f));
        d = std::min (std::max (d, lo), hi);
        float lpOut, bp;
        lp.tick (line.read(), tanG (lng ? 1700.0 : 3000.0, fs), 1.41421356f, lpOut, bp);
        const float wet = lpOut;
        const float a = activeGain();
        const float fbSig = conn[iRet] ? in[iRet] : wet;
        line.write (a * x + (val (pFb, in) / 8.0f) * softclip (fbSig), d, fs);
        const float m = norm (pMix, in), lvl = dbToGain (val (pOut, in));
        const float spill = step (pSpill) == 1 ? 1.0f : a;      // wet audible while bypassed only in spillover mode
        out[0] = a * lvl * ((1.0f - m) * x + m * wet) + (1.0f - a) * (in[0] + spill * lvl * m * wet);
        out[1] = (a + (1.0f - a) * spill) * lvl * wet;
        out[2] = wet;
        meters[1] = lfo.ph < 0.5 ? 1.0f : 0.0f; meters[2] = a;
    }
    int pDrive, pOut, pTime, pRange, pFb, pRate, pAmt, pWave, pMix, pSpill, pTap, iRet, iTap;
    ClockedLine line; Svf lp; Lfo lfo; TapTempo tap; Clock clock; Schmitt trig;
    float lastTapParam = 0.0f, tapNorm = 0.0f;
    bool tapped = false;
};

// ---------------------------------------------------------------- FilterBank (D-024, D-034)
class FilterBank final : public Base {
public:
    FilterBank() : Base (EffectId::FilterBank) {
        pDrive = P ("drive"); pOut = P ("output"); pLevel = P ("level1"); pVoice = P ("voicing"); pPattern = P ("pattern"); pEnv = P ("envelope");
        pRate = P ("rate"); pLfo = P ("lfo"); pSweep = P ("sweep"); pMix = P ("mix"); pTap = P ("tap"); activeParam = P ("active");
        iTap = In ("tap_in"); iStep = In ("step_in");
    }
    float tailSeconds() const noexcept override { return 4.5f; }
private:
    void doPrepare() override {}
    void doReset() noexcept override {
        for (int c = 0; c < 8; ++c) { f[c].reset(); env[c] = 0.0f; stage[c] = 0; }
        stepPhase = 1.0; stepIndex = -1; lfoPh = 0.0; tap.reset(); clock.reset(); trigTap = Schmitt(); trigStep = Schmitt();
        lastTapParam = target[pTap]; tapped = false;
    }
    void tick (const float* in, float* out) noexcept override {
        static const double kMids[8] = { 200, 300, 450, 675, 1000, 1500, 2200, 3400 };
        static const double kBass[8] = { 110, 160, 240, 350, 525, 775, 1200, 1800 };
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        clock.n++;
        const bool edge = trigTap.rising (in[iTap]) || (target[pTap] > 0.5f && lastTapParam <= 0.5f);
        lastTapParam = target[pTap];
        const float rn = norm (pRate, in);
        if (edge && tap.tap ((double) clock.n / fs)) { tapped = true; tapNorm = rn; }
        if (tapped && std::fabs (rn - tapNorm) > 0.01f) tapped = false;
        const double rate = tapped ? std::min (std::max (2.0 / tap.periodSeconds(), 0.05), 200.0) : (double) fromNormalised (I.params[pRate], rn);
        const bool mids = step (pVoice) == 1;
        const PatternChannel* pat = kPatterns[(mids ? 12 : 0) + step (pPattern)];
        // sequencer clock
        bool advance = false;
        if (conn[iStep]) advance = trigStep.rising (in[iStep]);
        else { stepPhase += rate / fs; if (stepPhase >= 1.0) { stepPhase -= 1.0; advance = true; } }
        if (advance) {
            ++stepIndex;
            for (int c = 0; c < 8; ++c)
                if (pat[c].length > 0 && ((pat[c].mask >> (stepIndex % pat[c].length)) & 1ULL)) stage[c] = 1;
        }
        float att, dec; filterBankEnvelopeTimes (val (pEnv, in), att, dec);
        const float period = (float) (fs / rate);
        const float up = 1.0f / std::max (1.0f, att * period), down = 1.0f / std::max (1.0f, dec * period);
        // frequency shift
        const float sw = norm (pSweep, in);
        double shift;
        if (step (pLfo) == 1) { lfoPh += 0.08 * std::pow (250.0, (double) sw) / fs; if (lfoPh >= 1.0) lfoPh -= 1.0; shift = 0.5 * std::sin (2.0 * kPi * lfoPh); }
        else shift = (double) sw - 0.5;
        const double mult = std::pow (2.0, shift);
        float sumL = 0.0f, sumR = 0.0f;
        for (int c = 0; c < 8; ++c) {
            if (pat[c].length == 0) { env[c] = 1.0f; stage[c] = 0; }
            else if (stage[c] == 1) { env[c] += up; if (env[c] >= 1.0f) { env[c] = 1.0f; stage[c] = 2; } }
            else if (stage[c] == 2) { env[c] -= down; if (env[c] <= 0.0f) { env[c] = 0.0f; stage[c] = 0; } }
            else if (env[c] > 0.0f) { env[c] = std::max (0.0f, env[c] - down); }
            const float g = tanG ((mids ? kMids[c] : kBass[c]) * mult, fs);
            float lpv, bpv, y;
            if (! mids && c == 0) { f[c].tick (x, g, 1.41421356f, lpv, bpv); y = lpv; }
            else { f[c].tick (x, g, 0.2f, lpv, bpv); y = 0.2f * bpv; }       // Q = 5, unity gain at the centre
            y *= target[pLevel + c] * env[c];
            if (c % 2 == 0) sumL += y; else sumR += y;
        }
        const float m = norm (pMix, in), lvl = dbToGain (val (pOut, in)), a = activeGain();
        out[0] = a * lvl * ((1.0f - m) * x + m * sumL) + (1.0f - a) * in[0];
        out[1] = a * lvl * ((1.0f - m) * x + m * sumR);          // bypass: nothing at the right output
        meters[1] = stepPhase < 0.5 ? 1.0f : 0.0f; meters[2] = a;
    }
    int pDrive, pOut, pLevel, pVoice, pPattern, pEnv, pRate, pLfo, pSweep, pMix, pTap, iTap, iStep;
    Svf f[8];
    float env[8] {};
    int stage[8] {};
    double stepPhase = 1.0, lfoPh = 0.0;
    long long stepIndex = -1;
    TapTempo tap; Clock clock; Schmitt trigTap, trigStep;
    float lastTapParam = 0.0f, tapNorm = 0.0f;
    bool tapped = false;
};

// ---------------------------------------------------------------- OscBox (D-025, D-035)
class OscBox final : public Base {
public:
    OscBox() : Base (EffectId::OscBox) {
        pDrive = P ("drive"); pOut = P ("output"); pFreq = P ("frequency"); pWave = P ("wave"); pSync = P ("sync"); pFm = P ("fm_amount");
        pEnv = P ("env_amount"); pMix = P ("mix"); activeParam = P ("active"); iFreq = In ("cv_frequency");
    }
    float tailSeconds() const noexcept override { return 1.0f; }
private:
    void doPrepare() override { env.set (fs, 5.0, 100.0); }
    void doReset() noexcept override { ph = 0.0; pending = 0.0f; pendingNaive = 0.0f; xPrev = 0.0f; env.y = 0.0f; armed = false; }
    // naive (not band-limited) value of the morphed waveform at phase p, and the weights of its sawtooth and pulse parts
    static float naive (double p, float w, float& ws, float& wp, double& width) {
        const float tri = 1.0f - 4.0f * std::fabs ((float) p - 0.5f), saw = (float) (2.0 * p - 1.0);
        width = w <= 2.0f / 3.0f ? 0.5 : 0.5 - 0.4 * (double) ((w - 2.0f / 3.0f) * 3.0f);
        const float pulse = p < width ? 1.0f : -1.0f;
        if (w < 1.0f / 3.0f) { ws = w * 3.0f; wp = 0.0f; return (1.0f - ws) * tri + ws * saw; }
        if (w < 2.0f / 3.0f) { wp = (w - 1.0f / 3.0f) * 3.0f; ws = 1.0f - wp; return ws * saw + wp * pulse; }
        ws = 0.0f; wp = 1.0f; return pulse;
    }
    // a step of height h that happened t samples (0 <= t < 1) before the present sample: correct the previous and the present sample
    void step2 (float h, double t, float& cur) {
        const float a = (float) t, b = 1.0f - a;
        pending += 0.5f * h * a * a;        // one sample earlier: distance (1 - t) before the step
        cur -= 0.5f * h * b * b;
    }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        const float e = env.next (x);
        const double oct = 5.0 * (double) in[iFreq] + (double) (val (pEnv, in) * std::min (e, 1.0f)) + 2.0 * (double) (norm (pFm, in) * x);
        const double f = std::min (std::max ((double) val (pFreq, in) * std::pow (2.0, oct), 0.01), std::min (20000.0, 0.45 * fs));
        const double dt = f / fs;
        const float w = norm (pWave, in);
        float ws, wp; double width;
        const double before = ph;
        ph += dt;
        bool wrapped = false; double tWrap = 0.0;
        if (ph >= 1.0) { ph -= 1.0; wrapped = true; tWrap = ph / dt; }
        float cur = naive (ph, w, ws, wp, width);
        if (wrapped) step2 (-2.0f * ws + 2.0f * wp, tWrap, cur);                                    // sawtooth falls, pulse rises
        const bool crossed = wrapped ? (ph >= width) : (before < width && ph >= width);
        if (crossed && wp > 0.0f) step2 (-2.0f * wp, (ph - width) / dt, cur);                        // pulse falls
        if (step (pSync) == 1) {
            if (x < -0.02f) armed = true;
            else if (armed && x > 0.02f) {
                // Hard sync. The input crossed +0.02 between the previous sample and this one; t samples ago by linear
                // interpolation. The phase restarts from 0 at that instant, so it is t * dt now.
                armed = false;
                const double t = (x - xPrev) > 1.0e-12f ? std::min (1.0, std::max (0.0, (double) ((x - 0.02f) / (x - xPrev)))) : 0.0;
                double atCross = ph - t * dt; if (atCross < 0.0) atCross += 1.0;
                const float raw = naive (atCross, w, ws, wp, width);            // value the waveform had reached when it was reset
                const float fresh = naive (0.0, w, ws, wp, width);
                ph = t * dt;
                // steps already applied for this sample (wrap, pulse edge) are discarded together with the old phase
                pending = pendingNaive; cur = naive (ph, w, ws, wp, width);
                step2 (fresh - raw, t, cur);
            }
        }
        xPrev = x;
        pendingNaive = naive (ph, w, ws, wp, width);
        const float osc = pending;          // the oscillator runs one sample late so that steps can be corrected on both sides
        pending = cur;
        const float m = norm (pMix, in), lvl = dbToGain (val (pOut, in)), a = activeGain();
        out[0] = a * lvl * ((1.0f - m) * x + m * osc * std::min (e, 1.0f)) + (1.0f - a) * in[0];
        out[1] = std::min (e, 2.0f);
        out[2] = 0.5f * osc;    // +/-2.5 V, present whatever the input does
        meters[1] = e; meters[2] = a;
    }
    int pDrive, pOut, pFreq, pWave, pSync, pFm, pEnv, pMix, iFreq;
    double ph = 0.0;
    float pending = 0.0f, pendingNaive = 0.0f, xPrev = 0.0f;
    EnvFollower env;
    bool armed = false;
};

// ---------------------------------------------------------------- Chorus (D-026, D-036)
class Chorus final : public Base {
public:
    Chorus() : Base (EffectId::Chorus) {
        pDrive = P ("drive"); pOut = P ("output"); pTime = P ("time"); pRange = P ("range"); pFb = P ("feedback"); pRate = P ("lfo_rate");
        pAmt = P ("lfo_amount"); pWave = P ("lfo_wave"); pMix = P ("mix"); pTap = P ("tap"); activeParam = P ("active");
        iRet = In ("fb_return"); iTap = In ("tap_in");
    }
    float tailSeconds() const noexcept override {
        const float g = 1.25f * std::fabs (target[pFb]);
        if (g >= 1.0f) return 1.0e9f;
        const float t = g <= 0.001f ? 0.15f : 0.05f * (1.0f + 6.908f / -std::log (g)) + 0.1f;
        return t > 60.0f ? 1.0e9f : t;
    }
private:
    void doPrepare() override { line.size ((int) (0.06 * fs) + 8); }
    void doReset() noexcept override { line.clear(); lp.reset(); lfo.reset(); tap.reset(); clock.reset(); trig = Schmitt(); lastTapParam = target[pTap]; tapped = false; }
    void tick (const float* in, float* out) noexcept override {
        const float x = softclip (dbToGain (val (pDrive, in)) * in[0]);
        levelMeter (x);
        clock.n++;
        const bool edge = trig.rising (in[iTap]) || (target[pTap] > 0.5f && lastTapParam <= 0.5f);
        lastTapParam = target[pTap];
        const float rn = norm (pRate, in);
        if (edge && tap.tap ((double) clock.n / fs)) { tapped = true; tapNorm = rn; }
        if (tapped && std::fabs (rn - tapNorm) > 0.01f) tapped = false;
        const double rate = tapped ? std::min (std::max (1.0 / tap.periodSeconds(), 0.01), 100.0) : (double) fromNormalised (I.params[pRate], rn);
        const bool chorus = step (pRange) == 1;
        const double lo = chorus ? 0.005 : 0.0006, hi = chorus ? 0.05 : 0.01;
        const float l = lfo.next (step (pWave), rate, fs);
        double d = lo * std::pow (hi / lo, (double) norm (pTime, in));
        d *= std::pow (hi / lo, 0.5 * (double) (norm (pAmt, in) * l));
        d = std::min (std::max (d, lo), hi);
        float lpOut, bp;
        lp.tick (line.read(), tanG (std::min (16000.0, 200.0 / d), fs), 1.41421356f, lpOut, bp);
        const float wet = lpOut;
        const float fbSig = conn[iRet] ? in[iRet] : wet;
        line.write (x + 1.25f * val (pFb, in) * softclip (fbSig), d, fs);
        const float m = norm (pMix, in), lvl = dbToGain (val (pOut, in)), a = activeGain();
        out[0] = a * lvl * ((1.0f - m) * x + m * wet) + (1.0f - a) * in[0];
        out[1] = a * lvl * ((1.0f - m) * x - m * wet) + (1.0f - a) * in[0];
        out[2] = wet;
        meters[1] = lfo.ph < 0.5 ? 1.0f : 0.0f; meters[2] = a;
    }
    int pDrive, pOut, pTime, pRange, pFb, pRate, pAmt, pWave, pMix, pTap, iRet, iTap;
    ClockedLine line; Svf lp; Lfo lfo; TapTempo tap; Clock clock; Schmitt trig;
    float lastTapParam = 0.0f, tapNorm = 0.0f;
    bool tapped = false;
};

int findIn (const char* id, int n, const char* (*get) (const void*, int), const void* ctx) {
    for (int i = 0; i < n; ++i) if (std::strcmp (get (ctx, i), id) == 0) return i;
    return -1;
}
} // namespace

const EffectInfo& effectInfo (EffectId id) {
    const int i = (int) id;
    if (i < 0 || i >= kNumEffects) throw std::out_of_range ("invalid EffectId");
    return kEffectInfos[i];
}
int findParam (const EffectInfo& info, const char* id) noexcept {
    return id == nullptr ? -1 : findIn (id, info.numParams, [] (const void* c, int i) { return static_cast<const EffectInfo*> (c)->params[i].id; }, &info);
}
int findInput (const EffectInfo& info, const char* id) noexcept {
    return id == nullptr ? -1 : findIn (id, info.numInputs, [] (const void* c, int i) { return static_cast<const EffectInfo*> (c)->inputs[i].id; }, &info);
}
int findOutput (const EffectInfo& info, const char* id) noexcept {
    return id == nullptr ? -1 : findIn (id, info.numOutputs, [] (const void* c, int i) { return static_cast<const EffectInfo*> (c)->outputs[i].id; }, &info);
}
int findMeter (const EffectInfo& info, const char* id) noexcept {
    return id == nullptr ? -1 : findIn (id, info.numMeters, [] (const void* c, int i) { return static_cast<const EffectInfo*> (c)->meters[i].id; }, &info);
}
float toNormalised (const ParamSpec& s, float v) noexcept {
    if (! std::isfinite (v)) v = s.def;
    v = clampf (v, s.min, s.max);
    if (s.taper == Taper::Log) return std::log (v / s.min) / std::log (s.max / s.min);
    if (s.taper == Taper::Stepped) return s.numSteps > 1 ? std::round (v) / (float) (s.numSteps - 1) : 0.0f;
    return (v - s.min) / (s.max - s.min);
}
float fromNormalised (const ParamSpec& s, float n) noexcept {
    if (! std::isfinite (n)) return s.def;
    n = clampf (n, 0.0f, 1.0f);
    if (s.taper == Taper::Log) return clampf (s.min * std::pow (s.max / s.min, n), s.min, s.max);
    if (s.taper == Taper::Stepped) return std::round (n * (float) (s.numSteps - 1));
    return s.min + n * (s.max - s.min);
}
float softClip (float x) noexcept { return 2.0f * std::tanh (0.5f * x); }
float lfoShape (LfoWave wave, float phase, float held) noexcept {
    if (! std::isfinite (phase)) return 0.0f;
    float p = phase - std::floor (phase);
    switch (wave) {
        case LfoWave::Sine: return std::sin (6.2831853f * p);
        case LfoWave::Triangle: return 1.0f - 4.0f * std::fabs (p - 0.5f);
        case LfoWave::Square: return p < 0.5f ? 1.0f : -1.0f;
        case LfoWave::Sawtooth: return 2.0f * p - 1.0f;
        case LfoWave::Ramp: return 1.0f - 2.0f * p;
        case LfoWave::RandomStep: return held;
    }
    return 0.0f;
}
void TapTempo::reset() noexcept { last_ = -1.0; sum_ = 0.0; period_ = 0.0; count_ = 0; }
bool TapTempo::tap (double t) noexcept {
    if (last_ >= 0.0 && t - last_ > 5.0) { count_ = 0; sum_ = 0.0; }
    if (count_ > 0) sum_ += t - last_;
    ++count_; last_ = t;
    if (count_ >= 3) { period_ = sum_ / (double) (count_ - 1); return true; }
    return false;
}
double TapTempo::periodSeconds() const noexcept { return period_; }
void filterBankEnvelopeTimes (float e, float& att, float& dec) noexcept {
    static const float E[6] = { 0, 2, 5, 6, 8, 10 }, A[6] = { 0.01f, 0.01f, 1.0f, 2.0f, 1.0f, 2.0f }, D[6] = { 0.25f, 1.0f, 1.0f, 2.0f, 0.02f, 0.02f };
    if (! std::isfinite (e)) e = 2.0f;
    e = clampf (e, 0.0f, 10.0f);
    int i = 0; while (i < 4 && e > E[i + 1]) ++i;
    const float t = (e - E[i]) / (E[i + 1] - E[i]);
    att = A[i] + t * (A[i + 1] - A[i]); dec = D[i] + t * (D[i + 1] - D[i]);
}

BlockRunner::BlockRunner (EffectId id) : fx_ (createEffect (id)) {}
BlockRunner::~BlockRunner() = default;
Effect& BlockRunner::effect() noexcept { return *fx_; }
void BlockRunner::prepare (double fs) { fx_->prepare (fs); }
void BlockRunner::process (const float* const* in, int numIn, float* const* out, int numOut, int n) noexcept {
    if (numIn < 0 || numIn > 2 || numOut < 1 || numOut > 2 || n <= 0) return;
    const EffectInfo& I = fx_->info();
    float x[kMaxPorts] {}, y[kMaxPorts] {};
    for (int i = 0; i < n; ++i) {
        x[0] = numIn == 0 ? 0.0f : (numIn == 1 ? in[0][i] : 0.5f * (in[0][i] + in[1][i]));
        fx_->process (x, y);
        const float r = I.rightOutput >= 0 ? y[I.rightOutput] : y[0];
        if (numOut == 2) { out[0][i] = y[0]; out[1][i] = r; }
        else out[0][i] = (I.monoSumsRight && I.rightOutput >= 0) ? y[0] + r : y[0];
    }
}
RackBridge::RackBridge (EffectId id) : fx_ (createEffect (id)) { for (int i = 0; i < kMaxPorts; ++i) conn_[i] = (i == 0); }
RackBridge::~RackBridge() = default;
Effect& RackBridge::effect() noexcept { return *fx_; }
void RackBridge::prepare (double fs) { fx_->prepare (fs); }
void RackBridge::process (RackIO& io) noexcept {
    const EffectInfo& I = fx_->info();
    float x[kMaxPorts] {}, y[kMaxPorts] {};
    for (int i = 0; i < I.numInputs; ++i) {
        if (io.inConnected[i] != conn_[i]) { conn_[i] = io.inConnected[i]; fx_->setInputConnected (i, conn_[i]); }
        x[i] = io.inVolts[i] * 0.2f;
    }
    fx_->process (x, y);
    for (int o = 0; o < I.numOutputs; ++o) io.outVolts[o] = clampf (5.0f * y[o], -12.0f, 12.0f);
    if (I.monoSumsRight && I.rightOutput >= 0 && ! io.outConnected[I.rightOutput]) io.outVolts[0] = clampf (io.outVolts[0] + io.outVolts[I.rightOutput], -12.0f, 12.0f);
}

std::unique_ptr<Effect> createEffect (EffectId id) {
    switch (id) {
        case EffectId::Lowpass: return std::make_unique<Lowpass>();
        case EffectId::RingMod: return std::make_unique<RingMod>();
        case EffectId::Phaser: return std::make_unique<Phaser>();
        case EffectId::Delay: return std::make_unique<Delay>();
        case EffectId::FilterBank: return std::make_unique<FilterBank>();
        case EffectId::OscBox: return std::make_unique<OscBox>();
        case EffectId::Chorus: return std::make_unique<Chorus>();
    }
    throw std::out_of_range ("invalid EffectId");
}
} // namespace muff
