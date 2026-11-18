// mufferfluffer DSP core: "Fluff Lowpass", an envelope-controlled ladder low-pass filter (D-020, D-030, D-037, D-018).
// SPDX-License-Identifier: AGPL-3.0-only
//
// Signal: in -> Drive (base class, D-037) -> x. An envelope follower on x moves the cutoff by up to Env Amount octaves;
// a zero-delay-feedback ladder of four trapezoidal one-poles with one soft limiter in its feedback path filters x plus
// a tiny noise (D-030); Mix crossfades from x to stage 4 (4-pole) or stage 2 (2-pole); Active crossfades the whole
// effect to the driven dry signal (D-018).
#include "effect_base.hpp"
#include "factories.hpp"

#include "muff/blocks.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>

namespace muff::detail {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kFastAttackMs = 2.0;          // D-027 item 3: envelope speed Fast
constexpr double kFastReleaseMs = 50.0;
constexpr double kSmoothAttackMs = 20.0;       // D-027 item 3: envelope speed Smooth
constexpr double kSmoothReleaseMs = 300.0;
constexpr int kSpeedFast = 1;                  // step of env_speed: 0 Smooth, 1 Fast
constexpr int kPolesFour = 1;                  // step of poles: 0 2-pole, 1 4-pole
constexpr float kMinCutoffHz = 10.0f;          // D-020: the filter runs at fc = clamp (fr, 10 Hz, min (20 kHz, 0.45 fs))
constexpr float kMaxCutoffHz = 20000.0f;       // D-020 (prewarp() adds the 0.45 fs limit)
constexpr float kFadeWidthHz = 4000.0f;        // D-020: resonance fades out between 16 and 20 kHz
constexpr float kLoopGainPerResonance = 0.5f;  // D-020: k = resonance / 2; k = 4 (resonance 8) self-oscillates
constexpr float kEnvOutLimit = 2.0f;           // D-020: env_out = env limited to 2.0
constexpr float kNoiseAmplitude = 1.0e-5f;     // D-030: uniform noise that starts self-oscillation without input
constexpr std::uint32_t kNoiseSeed = 0x9E3779B9u;
constexpr float kNoiseScale = 1.0f / 8388608.0f;     // 2^-23: maps 24 random bits to [0, 2)
constexpr float kSelfOscillationResonance = 8.0f;    // C-PEDA-002: self-oscillation from 8 upward
constexpr double kTailBaseSeconds = 0.1;             // S-011 tail formula
constexpr double kSixtyDbNepers = 6.908;             // ln (1000): 60 dB of decay
constexpr double kTailLowestCutoffHz = 10.0;         // the lowest cutoff CV or the envelope can reach
constexpr double kMaxFiniteTailSeconds = 60.0;       // core.hpp: a longer bound is reported as endless
constexpr float kEndlessTail = 1.0e9f;

// Uniform noise in [-amplitude, amplitude) from a linear congruential generator, restarted by reset() (T-008).
class NoiseSource {
public:
    void reset() noexcept { state_ = kNoiseSeed; }
    float next (float amplitude) noexcept;
private:
    std::uint32_t state_ = kNoiseSeed;
};

float NoiseSource::next (float amplitude) noexcept {
    state_ = state_ * 1664525u + 1013904223u;
    return (static_cast<float> (state_ >> 8) * kNoiseScale - 1.0f) * amplitude;
}

struct LadderOutputs { float stage2; float stage4; };

// Zero-delay-feedback ladder of four trapezoidal one-poles with the soft limiter in the feedback path (D-030).
class Ladder {
public:
    void reset() noexcept { std::fill (std::begin (s_), std::end (s_), 0.0f); }
    // G = g / (1 + g) with g = tan (pi fc / fs); k = loop gain.
    LadderOutputs tick (float x, float G, float k) noexcept;
private:
    float s_[4] {};
};

LadderOutputs Ladder::tick (float x, float G, float k) noexcept {
    const float G2 = G * G;
    const float G4 = G2 * G2;
    const float S = (1.0f - G) * (G2 * G * s_[0] + G2 * s_[1] + G * s_[2] + s_[3]);
    const float linear = (x - k * S) / (1.0f + k * G4);        // the linear zero-delay solution
    float y = x - k * softClip (G4 * linear + S);              // one refinement through the limiter
    LadderOutputs out { 0.0f, 0.0f };
    for (int i = 0; i < 4; ++i) {
        const float v = (y - s_[i]) * G;
        y = v + s_[i];
        s_[i] = y + v;
        if (i == 1) out.stage2 = y;
    }
    out.stage4 = y;
    return out;
}

class Lowpass final : public EffectBase {
public:
    Lowpass();
    float tailSeconds() const noexcept override;

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    float followEnvelope (float x) noexcept;
    void updateCutoff (float cutoffHz) noexcept;

    int cutoff_, resonance_, envAmount_, envSpeed_, poles_, mix_;
    int envOut_, envMeter_;
    EnvelopeFollower fast_, smooth_;
    Ladder ladder_;
    NoiseSource noise_;
    float cutoffHz_ = 0.0f;    // the cutoff G_ was computed for (control-rate hoisting of tan)
    float G_ = 0.0f;
};

Lowpass::Lowpass()
    : EffectBase (EffectId::Lowpass),
      cutoff_ (paramIndex ("cutoff")), resonance_ (paramIndex ("resonance")), envAmount_ (paramIndex ("env_amount")),
      envSpeed_ (paramIndex ("env_speed")), poles_ (paramIndex ("poles")), mix_ (paramIndex ("mix")),
      envOut_ (outputIndex ("env_out")), envMeter_ (findMeter (info(), "env")) {}

float Lowpass::tailSeconds() const noexcept {
    const double resonance = getParam (resonance_);
    if (resonance >= kSelfOscillationResonance) return kEndlessTail;
    const double slowestPole = 2.0 * kPi * kTailLowestCutoffHz * (1.0 - std::pow (resonance / kSelfOscillationResonance, 0.25));
    const double tail = kTailBaseSeconds + kSixtyDbNepers / slowestPole;
    return tail <= kMaxFiniteTailSeconds ? static_cast<float> (tail) : kEndlessTail;
}

void Lowpass::onPrepare() {
    fast_.configure (sampleRate(), kFastAttackMs, kFastReleaseMs);
    smooth_.configure (sampleRate(), kSmoothAttackMs, kSmoothReleaseMs);
}

void Lowpass::onReset() noexcept {
    fast_.reset();
    smooth_.reset();
    ladder_.reset();
    noise_.reset();
    cutoffHz_ = 0.0f;
    G_ = 0.0f;
}

// Both followers run so that switching Env Speed continues from a settled envelope.
float Lowpass::followEnvelope (float x) noexcept {
    const float fast = fast_.next (x);
    const float smooth = smooth_.next (x);
    return step (envSpeed_) == kSpeedFast ? fast : smooth;
}

void Lowpass::updateCutoff (float cutoffHz) noexcept {
    if (cutoffHz == cutoffHz_) return;
    cutoffHz_ = cutoffHz;
    const float g = prewarp (cutoffHz, sampleRate());
    G_ = g / (1.0f + g);
}

void Lowpass::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);
    const float env = followEnvelope (x);
    const float rawCutoff = value (cutoff_) * std::exp2 (value (envAmount_) * env);
    updateCutoff (std::min (std::max (rawCutoff, kMinCutoffHz), kMaxCutoffHz));
    const float fade = std::min (std::max ((kMaxCutoffHz - rawCutoff) / kFadeWidthHz, 0.0f), 1.0f);
    const float k = kLoopGainPerResonance * value (resonance_) * fade;
    const LadderOutputs stages = ladder_.tick (x + noise_.next (kNoiseAmplitude), G_, k);
    const float wet = step (poles_) == kPolesFour ? stages.stage4 : stages.stage2;
    const float effect = activeGain() * value (mix_);          // D-018: bypass leaves the driven dry signal
    out[0] = x + effect * (wet - x);
    out[envOut_] = std::min (env, kEnvOutLimit);
    setMeter (envMeter_, env);
}

} // namespace

std::unique_ptr<Effect> makeLowpass() { return std::make_unique<Lowpass>(); }

} // namespace muff::detail
