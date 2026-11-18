// mufferfluffer DSP core: "Fluff Osc" (OscBox), an envelope-gated oscillator with FM and hard sync (D-025, D-035).
// SPDX-License-Identifier: AGPL-3.0-only
//
// x = driven input; env = follower of x. The oscillator morphs triangle, sawtooth, square and a narrow pulse and runs
// one sample late, so that each step of its waveform (phase wrap, pulse edge, hard-sync reset) is corrected on both
// sides with a two-sample polynomial band-limited step (D-035).
#include "effect_base.hpp"
#include "factories.hpp"

#include <algorithm>
#include <cmath>

namespace muff::detail {
namespace {

constexpr double kEnvAttackMs = 5.0;            // D-027 item 3
constexpr double kEnvReleaseMs = 100.0;         // D-027 item 3
constexpr double kVoltPerOctScale = 5.0;        // 1 V per octave, 1 core unit = 5 V (D-012)
constexpr double kFmOctavesPerUnit = 2.0;       // exponential FM at full Amount (D-027 item 9)
constexpr double kMinFrequencyHz = 0.01;        // D-025
constexpr double kMaxFrequencyHz = 20000.0;     // D-025
constexpr double kMaxFrequencyFraction = 0.45;  // of the sample rate (D-025)
constexpr float kSyncThreshold = 0.02f;         // D-027 item 9: rise through +0.02 after being below -0.02
constexpr float kEnvOutLimit = 2.0f;            // env_out = env limited to 2.0 (D-025)
constexpr float kOscOutScale = 0.5f;            // osc_out = 0.5 x osc (D-025)
constexpr float kThird = 1.0f / 3.0f;
constexpr float kTwoThirds = 2.0f / 3.0f;
constexpr double kSquareWidth = 0.5;            // pulse width up to Wave 2/3 (D-025)
constexpr double kWidthSpan = 0.4;              // width 0.5 - 0.4 x 3 (w - 2/3): 10 % at the end of the travel (D-025)

// Weights of the three component waveforms and the pulse width for one Wave position (D-025).
struct Shape {
    float triangle;
    float saw;
    float pulse;
    double width;
};

Shape shapeFor (float wave) noexcept {
    if (wave < kThird) {
        const float a = 3.0f * wave;
        return { 1.0f - a, a, 0.0f, kSquareWidth };
    }
    if (wave < kTwoThirds) {
        const float b = 3.0f * (wave - kThird);
        return { 0.0f, 1.0f - b, b, kSquareWidth };
    }
    return { 0.0f, 0.0f, 1.0f, kSquareWidth - kWidthSpan * 3.0 * static_cast<double> (wave - kTwoThirds) };
}

// The naive (not band-limited) waveform at phase p in [0, 1): triangle -1 at 0 and +1 at 0.5; rising sawtooth;
// pulse +1 below the width and -1 above it. Peak 1.
float naiveValue (double p, const Shape& s) noexcept {
    const float phase = static_cast<float> (p);
    const float triangle = 1.0f - 4.0f * std::fabs (phase - 0.5f);
    const float saw = 2.0f * phase - 1.0f;
    const float pulse = p < s.width ? 1.0f : -1.0f;
    return s.triangle * triangle + s.saw * saw + s.pulse * pulse;
}

// Hard-sync trigger of D-025 / D-035: fires when x rises through +0.02 after having been below -0.02, and reports
// how many samples before the present one the crossing happened (linear interpolation, in [0, 1]).
class SyncDetector {
public:
    void reset() noexcept { armed_ = false; previous_ = 0.0f; }
    // Returns the crossing time in samples ago, or a negative value when there was no reset in this sample.
    double next (float x, bool enabled) noexcept;
private:
    bool armed_ = false;
    float previous_ = 0.0f;
};

double SyncDetector::next (float x, bool enabled) noexcept {
    double ago = -1.0;
    if (! enabled) {
        armed_ = false;
    } else if (x < -kSyncThreshold) {
        armed_ = true;
    } else if (armed_ && x > kSyncThreshold) {
        armed_ = false;
        // previous_ <= +0.02 < x while armed, so the denominator is positive
        ago = std::min (1.0, std::max (0.0, static_cast<double> ((x - kSyncThreshold) / (x - previous_))));
    }
    previous_ = x;
    return ago;
}

// Phase-accumulator oscillator with two-sample polynomial band-limited steps, emitted one sample late (D-035).
class BandLimitedOscillator {
public:
    void reset() noexcept { phase_ = 0.0; pending_ = 0.0f; }
    // Advances by dt = f / fs and returns the value of the previous sample. syncAgo >= 0: a hard-sync reset happened
    // that many samples before the present sample.
    float next (double dt, const Shape& shape, double syncAgo) noexcept;
private:
    float advanceFreely (double dt, const Shape& shape) noexcept;
    float resetBySync (double previousPhase, double dt, const Shape& shape, double ago) noexcept;
    float correctPulseEdge (double previousPhase, double dt, const Shape& shape, float cur) noexcept;
    void addStep (float height, double ago, float& cur) noexcept;

    double phase_ = 0.0;
    float pending_ = 0.0f;      // the previous sample's value, still open to correction
};

float BandLimitedOscillator::next (double dt, const Shape& shape, double syncAgo) noexcept {
    const float cur = syncAgo >= 0.0 ? resetBySync (phase_, dt, shape, syncAgo) : advanceFreely (dt, shape);
    const float emitted = pending_;
    pending_ = cur;
    return emitted;
}

// Phase wrap (sawtooth falls by 2, pulse rises by 2) and pulse falling edge.
float BandLimitedOscillator::advanceFreely (double dt, const Shape& shape) noexcept {
    double edgeSearchStart = phase_;
    phase_ += dt;
    const bool wrapped = phase_ >= 1.0;
    if (wrapped) {
        phase_ -= 1.0;
        edgeSearchStart = 0.0;      // dt < 0.45 and width <= 0.5: an edge can only follow the wrap
    }
    float cur = naiveValue (phase_, shape);
    if (wrapped) addStep (2.0f * (shape.pulse - shape.saw), phase_ / dt, cur);
    return correctPulseEdge (edgeSearchStart, dt, shape, cur);
}

// The phase restarts from 0 `ago` samples before the present sample. Corrections of this sample for steps before
// the reset are never applied (D-035: they are discarded with the old phase); a pulse edge after the reset is.
float BandLimitedOscillator::resetBySync (double previousPhase, double dt, const Shape& shape, double ago) noexcept {
    double atReset = previousPhase + (1.0 - ago) * dt;
    if (atReset >= 1.0) atReset -= 1.0;
    phase_ = ago * dt;
    float cur = naiveValue (phase_, shape);
    addStep (naiveValue (0.0, shape) - naiveValue (atReset, shape), ago, cur);
    return correctPulseEdge (0.0, dt, shape, cur);
}

float BandLimitedOscillator::correctPulseEdge (double previousPhase, double dt, const Shape& shape, float cur) noexcept {
    if (shape.pulse > 0.0f && previousPhase < shape.width && phase_ >= shape.width)
        addStep (-2.0f * shape.pulse, (phase_ - shape.width) / dt, cur);
    return cur;
}

// A step of the given height `ago` samples (0 <= ago < 1) before the present sample (C-DSP-053).
void BandLimitedOscillator::addStep (float height, double ago, float& cur) noexcept {
    const float t = static_cast<float> (ago);
    const float u = 1.0f - t;
    pending_ += 0.5f * height * t * t;
    cur -= 0.5f * height * u * u;
}

class OscBox final : public EffectBase {
public:
    OscBox();
    float tailSeconds() const noexcept override { return 1.0f; }    // S-017: the level follows the 100 ms release

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;
    double oscillatorIncrement (float x, float env, float voltPerOct) const noexcept;

    int output_, frequency_, wave_, sync_, fmAmount_, envAmount_, mix_;
    int cvFrequency_;
    int envOut_, oscOut_;
    int envMeter_;
    double maxFrequency_ = kMaxFrequencyHz;
    EnvelopeFollower follower_;
    SyncDetector syncDetector_;
    BandLimitedOscillator oscillator_;
};

OscBox::OscBox()
    : EffectBase (EffectId::OscBox),
      output_ (paramIndex ("output")), frequency_ (paramIndex ("frequency")), wave_ (paramIndex ("wave")),
      sync_ (paramIndex ("sync")), fmAmount_ (paramIndex ("fm_amount")), envAmount_ (paramIndex ("env_amount")),
      mix_ (paramIndex ("mix")), cvFrequency_ (inputIndex ("cv_frequency")), envOut_ (outputIndex ("env_out")),
      oscOut_ (outputIndex ("osc_out")), envMeter_ (findMeter (info(), "env")) {}

void OscBox::onPrepare() {
    follower_.configure (sampleRate(), kEnvAttackMs, kEnvReleaseMs);
    maxFrequency_ = std::min (kMaxFrequencyHz, kMaxFrequencyFraction * sampleRate());
}

void OscBox::onReset() noexcept {
    follower_.reset();
    syncDetector_.reset();
    oscillator_.reset();
}

// dt = f / fs with f = frequency x 2^(5 cv + env_amount x min(env, 1) + 2 x fm_amount x x), limited (D-025).
double OscBox::oscillatorIncrement (float x, float env, float voltPerOct) const noexcept {
    const double octaves = kVoltPerOctScale * static_cast<double> (voltPerOct)
                         + static_cast<double> (value (envAmount_) * std::min (env, 1.0f))
                         + kFmOctavesPerUnit * static_cast<double> (norm (fmAmount_) * x);
    const double f = static_cast<double> (value (frequency_)) * std::exp2 (octaves);
    return std::min (std::max (f, kMinFrequencyHz), maxFrequency_) / sampleRate();
}

void OscBox::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);
    const float env = follower_.next (x);
    const double syncAgo = syncDetector_.next (x, step (sync_) == 1);
    const float osc = oscillator_.next (oscillatorIncrement (x, env, in[cvFrequency_]), shapeFor (norm (wave_)), syncAgo);

    const float mix = norm (mix_);
    const float wet = dbToGain (value (output_)) * ((1.0f - mix) * x + mix * osc * std::min (env, 1.0f));
    const float active = activeGain();
    out[0] = active * wet + (1.0f - active) * in[0];        // bypass: the input at unity gain (D-018)
    out[envOut_] = std::min (env, kEnvOutLimit);
    out[oscOut_] = kOscOutScale * osc;                      // always running (D-018, D-025)
    setMeter (envMeter_, env);
}

} // namespace

std::unique_ptr<Effect> makeOscBox() { return std::make_unique<OscBox>(); }

} // namespace muff::detail
