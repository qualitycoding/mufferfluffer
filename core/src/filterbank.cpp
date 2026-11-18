// mufferfluffer DSP core: FilterBank, "Fluff Bank" (D-024, filters D-034, tap tempo D-014, bypass D-018).
// SPDX-License-Identifier: AGPL-3.0-only
//
// Eight state-variable filters in parallel on the driven input, each scaled by its slider and by an envelope that a
// step sequencer fires from the pattern table; odd filters feed the left output, even filters the right one.
#include "effect_base.hpp"
#include "factories.hpp"
#include "tables.hpp"

#include "muff/blocks.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace muff::detail {
namespace {

constexpr int kBands = 8;
constexpr int kPatternsPerBank = 12;                // D-024: 12 patterns per voicing, Bass bank first
constexpr int kMidsVoicing = 1;                     // step of the `voicing` parameter
constexpr int kLowpassBand = 0;                     // D-024: filter 1 of the Bass voicing is a low-pass
constexpr double kMidsCentresHz[kBands] = { 200, 300, 450, 675, 1000, 1500, 2200, 3400 };    // D-024
constexpr double kBassCentresHz[kBands] = { 110, 160, 240, 350, 525, 775, 1200, 1800 };      // D-024
constexpr float kBandPassK = 0.2f;                  // D-034: k = 1 / Q with Q = 5
constexpr float kButterworthK = 1.41421356f;        // D-034: k = sqrt(2) for the two-pole low-pass
constexpr double kSweepCentreOffset = 0.5;          // D-024: LFO off, shift = sweep - 0.5 octaves
constexpr double kLfoDepthOctaves = 0.5;            // D-024: LFO on, shift = 0.5 sin(2 pi phase)
constexpr double kLfoMinHz = 0.08;                  // D-024: LFO rate 0.08 x 250^sweep Hz
constexpr double kLfoRateSpan = 250.0;
constexpr double kStepsPerTap = 2.0;                // D-014: taps are quarter notes, steps are eighths
constexpr double kTappedMinRate = 0.05;             // D-014: tapped step rate limits, Hz
constexpr double kTappedMaxRate = 200.0;
constexpr float kTapReleaseTravel = 0.01f;          // D-014: knob travel that takes control back from a tapped tempo
constexpr double kRateMeterOnFraction = 0.5;        // meter `rate` is lit for the first half of each step
constexpr const char* kLevelIds[kBands] = { "level1", "level2", "level3", "level4", "level5", "level6", "level7", "level8" };

// Envelope of one channel (D-024): a trigger starts a linear rise from the present value to 1, after which it falls
// linearly to 0; the slopes are given per sample.
class StepEnvelope {
public:
    void reset() noexcept { value_ = 0.0f; rising_ = false; }
    void trigger() noexcept { rising_ = true; }
    // Static pattern: the envelope is held at 1 (and falls from there if a sequenced pattern follows).
    float hold() noexcept;
    float next (float riseStep, float fallStep) noexcept;
private:
    float value_ = 0.0f;
    bool rising_ = false;
};

float StepEnvelope::hold() noexcept {
    value_ = 1.0f;
    rising_ = false;
    return value_;
}

float StepEnvelope::next (float riseStep, float fallStep) noexcept {
    if (rising_) {
        value_ += riseStep;
        if (value_ >= 1.0f) {
            value_ = 1.0f;
            rising_ = false;
        }
    } else {
        value_ = std::max (0.0f, value_ - fallStep);
    }
    return value_;
}

// Step clock and step counter. The internal clock steps on the first sample after reset() and then every 1 / rate
// seconds; with the Step input patched, each trigger is one step instead. tick() returns bit c set when channel c
// of `pattern` triggers on this sample.
class Sequencer {
public:
    void reset() noexcept { phase_ = 1.0; nextStep_ = 0; }
    unsigned tick (int pattern, double increment, bool external, bool trigger) noexcept;
    bool inFirstHalfOfStep() const noexcept { return phase_ < kRateMeterOnFraction; }
private:
    bool clockStep (double increment, bool external, bool trigger) noexcept;
    double phase_ = 1.0;            // fraction of the present step that has elapsed
    std::uint64_t nextStep_ = 0;
};

bool Sequencer::clockStep (double increment, bool external, bool trigger) noexcept {
    phase_ += increment;
    if (external) {
        if (trigger) phase_ = 0.0;
        phase_ = std::min (phase_, 1.0);
        return trigger;
    }
    if (phase_ < 1.0) return false;
    phase_ -= std::floor (phase_);
    return true;
}

unsigned Sequencer::tick (int pattern, double increment, bool external, bool trigger) noexcept {
    if (! clockStep (increment, external, trigger)) return 0u;
    const std::uint64_t s = nextStep_++;
    unsigned fired = 0u;
    for (int c = 0; c < kBands; ++c) {
        const int length = patternLength (pattern, c);
        if (length > 0 && ((patternMask (pattern, c) >> (s % static_cast<std::uint64_t> (length))) & 1ULL) != 0ULL)
            fired |= 1u << c;
    }
    return fired;
}

// Tap tempo of D-014: a tap that makes a period available takes control of the step rate until the Rate knob
// (CV included) moves by more than 0.01 of its travel from where it was at that tap.
class TappedRate {
public:
    void reset() noexcept;
    void update (bool tapped, double nowSeconds, float knob) noexcept;
    bool engaged() const noexcept { return engaged_; }
    double stepRate() const noexcept;
private:
    TapTempo tempo_;
    float knobAtTap_ = 0.0f;
    bool engaged_ = false;
};

void TappedRate::reset() noexcept {
    tempo_.reset();
    knobAtTap_ = 0.0f;
    engaged_ = false;
}

void TappedRate::update (bool tapped, double nowSeconds, float knob) noexcept {
    if (tapped && tempo_.tap (nowSeconds)) {
        engaged_ = true;
        knobAtTap_ = knob;
    }
    if (engaged_ && std::fabs (knob - knobAtTap_) > kTapReleaseTravel) engaged_ = false;
}

double TappedRate::stepRate() const noexcept {
    const double period = tempo_.periodSeconds();
    const double rate = period > 0.0 ? kStepsPerTap / period : kTappedMaxRate;
    return std::min (std::max (rate, kTappedMinRate), kTappedMaxRate);
}

class FilterBank final : public EffectBase {
public:
    FilterBank();
    // The sequencer and envelopes only scale the bands; the slowest band (Q 5 at 110 Hz shifted down half an octave)
    // rings 60 dB down in under 0.2 s (S-016).
    float tailSeconds() const noexcept override { return 0.5f; }

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    double stepRate (const float* in) noexcept;
    float frequencyMultiplier() noexcept;
    void updateCoefficients (bool mids, float multiplier) noexcept;
    float band (int c, bool mids, float x) noexcept;

    int outputParam_, rateParam_, envelopeParam_, voicingParam_, patternParam_, lfoParam_, sweepParam_, mixParam_, tapParam_;
    int levelParam_[kBands];
    int tapInput_, stepInput_;
    int leftOutput_, rightOutput_;
    int rateMeter_;

    StateVariableFilter filters_[kBands];
    StepEnvelope envelopes_[kBands];
    float g_[kBands] {};
    float cachedMultiplier_ = -1.0f;    // coefficients in g_ belong to this multiplier and voicing
    bool cachedMids_ = false;
    Sequencer sequencer_;
    TappedRate tappedRate_;
    TriggerDetector tapTrigger_;
    TriggerDetector stepTrigger_;
    Lfo lfo_;
    int lastTapStep_ = 0;
    std::uint64_t sampleCount_ = 0;     // clock of the tap tempo
};

FilterBank::FilterBank()
    : EffectBase (EffectId::FilterBank),
      outputParam_ (paramIndex ("output")), rateParam_ (paramIndex ("rate")), envelopeParam_ (paramIndex ("envelope")),
      voicingParam_ (paramIndex ("voicing")), patternParam_ (paramIndex ("pattern")), lfoParam_ (paramIndex ("lfo")),
      sweepParam_ (paramIndex ("sweep")), mixParam_ (paramIndex ("mix")), tapParam_ (paramIndex ("tap")),
      tapInput_ (inputIndex ("tap_in")), stepInput_ (inputIndex ("step_in")),
      leftOutput_ (outputIndex ("out_l")), rightOutput_ (outputIndex ("out_r")),
      rateMeter_ (findMeter (info(), "rate")) {
    for (int c = 0; c < kBands; ++c) levelParam_[c] = paramIndex (kLevelIds[c]);
}

void FilterBank::onPrepare() { lfo_.prepare (sampleRate()); }

void FilterBank::onReset() noexcept {
    for (int c = 0; c < kBands; ++c) {
        filters_[c].reset();
        envelopes_[c].reset();
    }
    cachedMultiplier_ = -1.0f;
    sequencer_.reset();
    tappedRate_.reset();
    tapTrigger_.reset();
    stepTrigger_.reset();
    lfo_.reset();
    lastTapStep_ = step (tapParam_);
    sampleCount_ = 0;
}

// Steps per second: the Rate knob, or the tapped rate while it has control (D-014).
double FilterBank::stepRate (const float* in) noexcept {
    const bool tapEdge = tapTrigger_.rising (in[tapInput_]);
    const int tapStep = step (tapParam_);
    const bool tapPressed = tapStep == 1 && lastTapStep_ == 0;
    lastTapStep_ = tapStep;
    tappedRate_.update (tapEdge || tapPressed, static_cast<double> (sampleCount_) / sampleRate(), norm (rateParam_));
    return tappedRate_.engaged() ? tappedRate_.stepRate() : static_cast<double> (value (rateParam_));
}

// 2^shift for every centre frequency (D-024).
float FilterBank::frequencyMultiplier() noexcept {
    const double sweep = static_cast<double> (norm (sweepParam_));
    if (step (lfoParam_) == 0) return static_cast<float> (std::exp2 (sweep - kSweepCentreOffset));
    const double lfoHz = kLfoMinHz * std::pow (kLfoRateSpan, sweep);
    return static_cast<float> (std::exp2 (kLfoDepthOctaves * static_cast<double> (lfo_.tick (LfoWave::Sine, lfoHz))));
}

void FilterBank::updateCoefficients (bool mids, float multiplier) noexcept {
    if (multiplier == cachedMultiplier_ && mids == cachedMids_) return;
    const double* centres = mids ? kMidsCentresHz : kBassCentresHz;
    for (int c = 0; c < kBands; ++c) g_[c] = prewarp (centres[c] * static_cast<double> (multiplier), sampleRate());
    cachedMultiplier_ = multiplier;
    cachedMids_ = mids;
}

float FilterBank::band (int c, bool mids, float x) noexcept {
    if (! mids && c == kLowpassBand) return filters_[c].tick (x, g_[c], kButterworthK).low;
    return kBandPassK * filters_[c].tick (x, g_[c], kBandPassK).band;    // unity gain at the centre (D-034)
}

void FilterBank::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);

    const double rate = stepRate (in);
    ++sampleCount_;
    const bool mids = step (voicingParam_) == kMidsVoicing;
    const int pattern = (mids ? kPatternsPerBank : 0) + step (patternParam_);
    const unsigned fired = sequencer_.tick (pattern, rate / sampleRate(), connected (stepInput_), stepTrigger_.rising (in[stepInput_]));

    float attackSteps = 0.0f, decaySteps = 0.0f;
    filterBankEnvelopeTimes (value (envelopeParam_), attackSteps, decaySteps);
    const float samplesPerStep = static_cast<float> (sampleRate() / rate);
    const float riseStep = 1.0f / std::max (1.0f, attackSteps * samplesPerStep);     // each phase lasts at least one sample
    const float fallStep = 1.0f / std::max (1.0f, decaySteps * samplesPerStep);

    updateCoefficients (mids, frequencyMultiplier());
    float sums[2] = { 0.0f, 0.0f };     // left: filters 1, 3, 5, 7; right: filters 2, 4, 6, 8
    for (int c = 0; c < kBands; ++c) {
        if ((fired >> c) & 1u) envelopes_[c].trigger();
        const float envelope = patternLength (pattern, c) == 0 ? envelopes_[c].hold() : envelopes_[c].next (riseStep, fallStep);
        sums[c % 2] += band (c, mids, x) * value (levelParam_[c]) * envelope;
    }

    const float mix = value (mixParam_);
    const float gain = dbToGain (value (outputParam_));
    const float active = activeGain();
    out[leftOutput_] = active * gain * ((1.0f - mix) * x + mix * sums[0]) + (1.0f - active) * in[0];   // bypass: Left = input
    out[rightOutput_] = active * gain * ((1.0f - mix) * x + mix * sums[1]);                          // bypass: Right silent (D-018)
    setMeter (rateMeter_, sequencer_.inFirstHalfOfStep() ? 1.0f : 0.0f);
}

} // namespace

std::unique_ptr<Effect> makeFilterBank() { return std::make_unique<FilterBank>(); }

} // namespace muff::detail
