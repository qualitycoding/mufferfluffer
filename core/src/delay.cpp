// mufferfluffer DSP core: "Fluff Delay" (D-023), a clocked delay line (D-033) with a fixed two-pole wet filter,
// LFO-modulated clock (D-015), tap tempo (D-014) and the Spillover bypass of D-018.
// SPDX-License-Identifier: AGPL-3.0-only
#include "clocked_line.hpp"
#include "dsp_common.hpp"
#include "effect_base.hpp"
#include "factories.hpp"

#include "muff/blocks.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace muff::detail {
namespace {

constexpr double kShortMinSeconds = 0.040;      // D-023 (C-PEDB-006): Short range 40-400 ms
constexpr double kLongMinSeconds = 0.080;       // D-023 (C-PEDB-006): Long range 80-800 ms
constexpr double kRangeRatio = 10.0;            // D-023: d = lo x 10^n, so each range spans one decade
constexpr double kShortCutoffHz = 3000.0;       // D-023 (C-PEDB-022): wet filter, Short range
constexpr double kLongCutoffHz = 1700.0;        // D-023 (C-PEDB-022): wet filter, Long range
constexpr float kButterworthDamping = 1.41421356f;  // D-034: k = sqrt(2) for the two-pole Butterworth low-pass
constexpr float kUnityFeedback = 8.0f;          // D-023: loop gain = feedback / 8, unity at Feedback 8 (C-PEDB-011)
constexpr float kLfoOctavesPerAmount = 0.1f;    // D-023: the LFO multiplies d by 2^(lfo x amount / 10)
constexpr double kLineSeconds = 0.85;           // D-033: the Delay's line holds 0.85 fs + 8 samples
constexpr int kLineExtraSamples = 8;
constexpr float kTapReleaseTravel = 0.01f;      // D-014: moving the knob by more than this hands control back
constexpr double kRateMeterHalfCycle = 0.5;     // meter `rate` is lit for the first half of each LFO cycle
// tailSeconds() (plan step S-014): the longest delay, the decay to -60 dB (ln 1000 = 6.908) and a margin.
constexpr float kTailLongestDelay = 0.8f;
constexpr float kTailSixtyDbNepers = 9.0f;       // ln (1000 x 2 x 3.98): -60 dB re 1.0 from a first echo of 2 at Output +12 dB (S-REVIEW B1-01)
constexpr float kTailMargin = 0.1f;
constexpr float kTailNegligibleGain = 0.001f;
constexpr float kTailMaxSeconds = 60.0f;
constexpr float kTailForever = 1.0e9f;

class FluffDelay final : public EffectBase {
public:
    FluffDelay();
    float tailSeconds() const noexcept override;

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    // True on a rising edge of the tap jack or a 0-to-1 change of the Tap parameter (D-014).
    bool tapped (const float* in) noexcept;
    // Registers a tap and decides whether the tapped period or the Time knob sets the delay (D-014).
    void updateTapControl (bool tap, float knob) noexcept;
    // The delay time of this sample in seconds: knob or tapped period, times the LFO factor, limited to the range.
    double delaySeconds (bool longRange, bool tap) noexcept;
    // 2^(lfo x amount / 10) for this sample; also drives the `rate` meter.
    double lfoFactor() noexcept;

    int outputParam_, timeParam_, rangeParam_, feedbackParam_, rateParam_, amountParam_, waveParam_, mixParam_, spillParam_, tapParam_;
    int returnInput_, tapInput_;
    int mixOut_, delayOut_, sendOut_;
    int rateMeter_;

    ClockedLine line_;
    StateVariableFilter wetFilter_;
    Lfo lfo_;
    TapTempo tapTempo_;
    TriggerDetector tapTrigger_;
    float gShort_ = 0.0f;           // prewarped wet-filter coefficients, fixed per sample rate
    float gLong_ = 0.0f;
    std::int64_t clock_ = 0;        // samples since reset(): the time base of the tap tempo
    int lastTapStep_ = 0;
    bool tapControl_ = false;       // the tapped period, not the knob, sets the delay
    float tapKnob_ = 0.0f;          // Time knob position (CV included) at the tap that made the period available
};

FluffDelay::FluffDelay()
    : EffectBase (EffectId::Delay),
      outputParam_ (paramIndex ("output")), timeParam_ (paramIndex ("time")), rangeParam_ (paramIndex ("range")),
      feedbackParam_ (paramIndex ("feedback")), rateParam_ (paramIndex ("lfo_rate")), amountParam_ (paramIndex ("lfo_amount")),
      waveParam_ (paramIndex ("lfo_wave")), mixParam_ (paramIndex ("mix")), spillParam_ (paramIndex ("spillover")),
      tapParam_ (paramIndex ("tap")), returnInput_ (inputIndex ("fb_return")), tapInput_ (inputIndex ("tap_in")),
      mixOut_ (outputIndex ("out")), delayOut_ (outputIndex ("delay_out")), sendOut_ (outputIndex ("fb_send")),
      rateMeter_ (findMeter (info(), "rate")) {}

float FluffDelay::tailSeconds() const noexcept {
    const float g = getParam (feedbackParam_) / kUnityFeedback;
    if (g >= 1.0f) return kTailForever;
    if (g <= kTailNegligibleGain) return kTailLongestDelay + kTailMargin;
    const float t = kTailLongestDelay * (1.0f + kTailSixtyDbNepers / -std::log (g)) + kTailMargin;
    return t > kTailMaxSeconds ? kTailForever : t;
}

void FluffDelay::onPrepare() {
    lfo_.prepare (sampleRate());
    line_.size (static_cast<int> (kLineSeconds * sampleRate()) + kLineExtraSamples);
    gShort_ = prewarp (kShortCutoffHz, sampleRate());
    gLong_ = prewarp (kLongCutoffHz, sampleRate());
}

void FluffDelay::onReset() noexcept {
    line_.clear();
    wetFilter_.reset();
    lfo_.reset();
    tapTempo_.reset();
    tapTrigger_.reset();
    clock_ = 0;
    lastTapStep_ = step (tapParam_);
    tapControl_ = false;
    tapKnob_ = 0.0f;
}

bool FluffDelay::tapped (const float* in) noexcept {
    const bool jack = tapTrigger_.rising (in[tapInput_]);
    const int tapStep = step (tapParam_);
    const bool button = tapStep == 1 && lastTapStep_ == 0;
    lastTapStep_ = tapStep;
    return jack || button;
}

void FluffDelay::updateTapControl (bool tap, float knob) noexcept {
    if (tap && tapTempo_.tap (static_cast<double> (clock_) / sampleRate())) {
        tapControl_ = true;
        tapKnob_ = knob;
    }
    if (tapControl_ && std::fabs (knob - tapKnob_) > kTapReleaseTravel) tapControl_ = false;
}

double FluffDelay::lfoFactor() noexcept {
    const float lfo = lfo_.tick (static_cast<LfoWave> (step (waveParam_)), static_cast<double> (value (rateParam_)));
    setMeter (rateMeter_, lfo_.phase() < kRateMeterHalfCycle ? 1.0f : 0.0f);
    return std::exp2 (static_cast<double> (lfo * value (amountParam_) * kLfoOctavesPerAmount));
}

double FluffDelay::delaySeconds (bool longRange, bool tap) noexcept {
    const double lo = longRange ? kLongMinSeconds : kShortMinSeconds;
    const double hi = lo * kRangeRatio;
    const float knob = norm (timeParam_);
    updateTapControl (tap, knob);
    const double base = tapControl_ ? tapTempo_.periodSeconds() : lo * std::pow (kRangeRatio, static_cast<double> (knob));
    return std::min (std::max (base * lfoFactor(), lo), hi);
}

void FluffDelay::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);
    const bool longRange = step (rangeParam_) == 1;
    const double d = delaySeconds (longRange, tapped (in));
    ++clock_;

    const float wet = wetFilter_.tick (line_.read(), longRange ? gLong_ : gShort_, kButterworthDamping).low;
    const float a = activeGain();
    const float returned = connected (returnInput_) ? in[returnInput_] : wet;
    line_.write (a * x + value (feedbackParam_) / kUnityFeedback * softClip (returned), d * sampleRate());

    // D-018: bypassed, Mix Out is the input; the wet part fades out with Active unless Spillover keeps it sounding.
    const float wetGain = step (spillParam_) == 1 ? 1.0f : a;
    const float level = dbToGain (value (outputParam_));
    const float mix = norm (mixParam_);
    out[mixOut_] = level * (a * (1.0f - mix) * x + wetGain * mix * wet) + (1.0f - a) * in[0];
    out[delayOut_] = level * wetGain * wet;
    out[sendOut_] = wet;
}

} // namespace

std::unique_ptr<Effect> makeDelay() { return std::make_unique<FluffDelay>(); }

} // namespace muff::detail
