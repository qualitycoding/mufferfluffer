// mufferfluffer DSP core: the Phaser, "Fluff Phase" (D-018, D-022, D-032).
// SPDX-License-Identifier: AGPL-3.0-only
// Incremental mutation run demonstration (S-MUTATE); reverted by the next commit.
#include "dsp_common.hpp"
#include "effect_base.hpp"
#include "factories.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace muff::detail {
namespace {

constexpr double kPi = 3.14159265358979323846;

// ---- constants of D-022
constexpr double kCentreHz = 1100.0;            // stage corner at Sweep 5
constexpr double kSweepOctaves = 6.0;           // the whole Sweep travel, and the Sweep In scale per core unit (D-027 item 14)
constexpr double kLfoOctaves = 3.0;             // stage-corner swing each way at LFO Amount 1 (C-PEDA-041, T-308)
constexpr double kMinCornerHz = 20.0;           // lower limit of the stage corner
constexpr float kSweepInLimit = 0.5f;           // Sweep In is clamped to +/-0.5 core units (C-PEDA-046)
constexpr double kLfoLoHz = 0.01;               // LFO rate at knob 0, Lo range
constexpr double kLfoHiHz = 1.0;                // LFO rate at knob 0, Hi range
constexpr double kLfoSpan = 250.0;              // ratio of the LFO rate at knob 1 to knob 0
constexpr float kFeedbackPerStep = 0.09f;       // feedback gain per Resonance unit (0.9 at the end of the travel)
constexpr float kOutputHalf = 0.5f;             // Out and Aux are half the sum or difference
constexpr float kLfoOutScale = 0.56f;           // LFO Out amplitude in core units: 2.8 V
constexpr int kStagesPerChain = 6;
constexpr int kTwelveStageStep = 1;             // step index of the "12-stage" setting

// ---- constants of the tail bound (S-013)
constexpr float kTailBase = 0.1f;               // seconds
constexpr float kTailNegligibleGain = 0.001f;   // feedback gain at or below which the loop adds no tail
constexpr double kTailDecadesToNeper = 6.908;   // ln(1000): feedback must fall by 60 dB
constexpr double kMaxStages = 2.0 * kStagesPerChain;

// Anything smaller than this in an all-pass state is flushed: a decay by 0.9997 per sample can stall in the denormal range.
constexpr float kFlushThreshold = 1.0e-20f;

// First-order all-pass of D-032: y = a x + x1 - a y1.
class AllPassStage {
public:
    void reset() noexcept { x1_ = 0.0f; y1_ = 0.0f; }
    float tick (float x, float a) noexcept {
        const float y = a * x + x1_ - a * y1_;
        x1_ = flushed (x);
        y1_ = flushed (y);
        return y;
    }
private:
    static float flushed (float v) noexcept { return std::fabs (v) < kFlushThreshold ? 0.0f : v; }
    float x1_ = 0.0f;
    float y1_ = 0.0f;
};

// Six all-pass stages sharing one coefficient.
class AllPassChain {
public:
    void reset() noexcept { for (AllPassStage& stage : stages_) stage.reset(); }
    float tick (float x, float a) noexcept {
        for (AllPassStage& stage : stages_) x = stage.tick (x, a);
        return x;
    }
private:
    std::array<AllPassStage, kStagesPerChain> stages_;
};

// Coefficient a = (t - 1) / (t + 1), t = tan (pi fc / fs), for a stage corner `octaves` above or below the centre.
float stageCoefficient (double octaves, double sampleRate) noexcept {
    const double corner = std::max (kMinCornerHz, kCentreHz * std::exp2 (octaves));
    const double t = static_cast<double> (prewarp (corner, sampleRate));
    return static_cast<float> ((t - 1.0) / (t + 1.0));
}

class Phaser final : public EffectBase {
public:
    Phaser();
    float tailSeconds() const noexcept override;

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    float lfoValue() noexcept;
    bool twelveStages() const noexcept;
    float chainOutput (float u, float lfo, bool sweepInPatched, float sweepIn) noexcept;

    int sweepParam_, resonanceParam_, rateParam_, rangeParam_, amountParam_, stagesParam_, outputParam_;
    int sweepInInput_;
    int outMain_, outAux_, outLfo_;
    int lfoMeter_;
    Lfo lfo_;
    AllPassChain lower_, upper_;
    float feedback_ = 0.0f;     // previous sample's chain output (w of D-022)
};

Phaser::Phaser()
    : EffectBase (EffectId::Phaser),
      sweepParam_ (paramIndex ("sweep")),
      resonanceParam_ (paramIndex ("resonance")),
      rateParam_ (paramIndex ("lfo_rate")),
      rangeParam_ (paramIndex ("lfo_range")),
      amountParam_ (paramIndex ("lfo_amount")),
      stagesParam_ (paramIndex ("stages")),
      outputParam_ (paramIndex ("output")),
      sweepInInput_ (inputIndex ("sweep_in")),
      outMain_ (outputIndex ("out")),
      outAux_ (outputIndex ("aux_out")),
      outLfo_ (outputIndex ("lfo_out")),
      lfoMeter_ (findMeter (info(), "lfo")) {}

void Phaser::onPrepare() { lfo_.prepare (sampleRate()); }

void Phaser::onReset() noexcept {
    lfo_.reset();
    lower_.reset();
    upper_.reset();
    feedback_ = 0.0f;
}

bool Phaser::twelveStages() const noexcept { return step (stagesParam_) == kTwelveStageStep; }

// Triangle LFO at 0.01 x 250^n Hz (Lo) or 1 x 250^n Hz (Hi), n the smoothed knob position (C-PEDA-039).
float Phaser::lfoValue() noexcept {
    const double base = step (rangeParam_) == 0 ? kLfoLoHz : kLfoHiHz;
    const double hz = base * std::exp (static_cast<double> (norm (rateParam_)) * std::log (kLfoSpan));
    return lfo_.tick (LfoWave::Triangle, hz);
}

// The 6 or 12 all-pass stages. The second chain always runs behind the first, so that switching between 6 and 12
// stages finds its states warm; it follows the Sweep In law only in 12-stage mode with the jack patched (D-022).
float Phaser::chainOutput (float u, float lfo, bool sweepInPatched, float sweepIn) noexcept {
    const double sweepOctaves = kSweepOctaves * (static_cast<double> (norm (sweepParam_)) - 0.5);
    const double lfoOctaves = kLfoOctaves * static_cast<double> (norm (amountParam_)) * static_cast<double> (lfo);
    const float lowerCoefficient = stageCoefficient (sweepOctaves + lfoOctaves, sampleRate());
    float upperCoefficient = lowerCoefficient;
    if (sweepInPatched) {
        const double offset = static_cast<double> (std::min (std::max (sweepIn, -kSweepInLimit), kSweepInLimit));
        upperCoefficient = stageCoefficient (sweepOctaves + kSweepOctaves * offset, sampleRate());
    }
    const float six = lower_.tick (u, lowerCoefficient);
    const float twelve = upper_.tick (six, upperCoefficient);
    return twelveStages() ? twelve : six;
}

void Phaser::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);
    const float lfo = lfoValue();
    const float u = x + kFeedbackPerStep * value (resonanceParam_) * std::tanh (feedback_);
    const bool sweepInPatched = twelveStages() && connected (sweepInInput_);
    const float y = chainOutput (u, lfo, sweepInPatched, sweepInPatched ? in[sweepInInput_] : 0.0f);
    feedback_ = y;

    const float gain = dbToGain (value (outputParam_)) * kOutputHalf;
    const float wet = activeGain();
    out[outMain_] = wet * gain * (u + y) + (1.0f - wet) * in[0];
    out[outAux_] = wet * gain * (u - y) + (1.0f - wet) * in[0];      // D-018: bypass sends the input to every audio output
    out[outLfo_] = kLfoOutScale * lfo;
    setMeter (lfoMeter_, 0.5f * (1.0f + lfo));
}

// Upper bound on the time to fall 60 dB (S-013): the chain's low-frequency group delay D, once for the signal going
// round and once more per trip of the feedback loop, which loses a factor g = 0.09 x Resonance on each.
float Phaser::tailSeconds() const noexcept {
    const double g = static_cast<double> (kFeedbackPerStep) * static_cast<double> (getParam (resonanceParam_));
    const double lowestCorner = std::max (kMinCornerHz, kCentreHz * std::exp2 (kSweepOctaves * (static_cast<double> (getParam (sweepParam_)) / 10.0 - 0.5)
                                                                              - kLfoOctaves * static_cast<double> (getParam (amountParam_))));
    const double stages = static_cast<double> (getParam (stagesParam_) >= static_cast<float> (kTwelveStageStep) ? kMaxStages : kStagesPerChain);
    const double delay = stages / (kPi * lowestCorner);
    // Without feedback the signal only goes round the chain once; the bound still includes that group delay (S-REVIEW B1-02).
    if (g <= static_cast<double> (kTailNegligibleGain)) return static_cast<float> (static_cast<double> (kTailBase) + 2.0 * delay);
    return static_cast<float> (static_cast<double> (kTailBase) + 2.0 * delay * (1.0 + kTailDecadesToNeper / -std::log (g)));
}

} // namespace

std::unique_ptr<muff::Effect> makePhaser() { return std::make_unique<Phaser>(); }

} // namespace muff::detail
