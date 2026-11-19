// mufferfluffer DSP core: Chorus, "Fluff Swarm" (D-026): a clocked delay line (D-033) whose clock follows the Time
// knob and an LFO (D-015, tap tempo D-014), a clock-tracking wet low-pass (D-036), a bipolar feedback loop with an
// external insert, and the stereo outputs dry + wet / dry - wet; bypass per D-018.
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

constexpr double kFlangeMinSeconds = 0.0006;    // D-026 (C-PEDC-024): Flange range 0.6-10 ms
constexpr double kFlangeMaxSeconds = 0.010;
constexpr double kChorusMinSeconds = 0.005;     // D-026 (C-PEDC-024): Chorus range 5-50 ms
constexpr double kChorusMaxSeconds = 0.050;
constexpr double kLfoSpanPerAmount = 0.5;       // D-026: d = lo (hi/lo)^(time + 0.5 amount lfo)
constexpr double kWetMaxCutoffHz = 16000.0;     // D-036: wet bandwidth min(16 kHz, 200 / d)
constexpr double kWetCutoffTimesDelay = 200.0;
constexpr float kButterworthDamping = 1.41421356f;  // D-034: k = sqrt(2) for the two-pole Butterworth low-pass
constexpr float kFeedbackScale = 1.25f;         // D-026 (C-PEDC-025): loop gain 1 at |feedback| = 0.8
constexpr double kTappedMinRateHz = 0.01;       // D-014: tapped LFO rate = 1 / period, limited to 0.01-100 Hz
constexpr double kTappedMaxRateHz = 100.0;
constexpr float kTapReleaseTravel = 0.01f;      // D-014: moving the knob by more than this hands control back
constexpr double kLineSeconds = 0.06;           // D-033: the Chorus's line holds 0.06 fs + 8 samples
constexpr int kLineExtraSamples = 8;
constexpr double kRateMeterHalfCycle = 0.5;     // D-026: meter `rate` is lit while the LFO phase is below 0.5
// tailSeconds() (plan step S-015): the longest delay, the decay to -60 dB (ln 1000 = 6.908) and a margin.
constexpr float kTailLongestDelay = 0.05f;
constexpr float kTailSixtyDbNepers = 9.0f;       // ln (1000 x 2 x 3.98): -60 dB re 1.0 from a first echo of 2 at Output +12 dB (S-REVIEW B1-01)
constexpr float kTailMargin = 0.1f;
constexpr float kTailNegligibleGain = 0.001f;
constexpr float kTailMaxSeconds = 60.0f;
constexpr float kTailForever = 1.0e9f;

class FluffSwarm final : public EffectBase {
public:
    FluffSwarm();
    float tailSeconds() const noexcept override;

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    // True on a rising edge of the tap jack or a 0-to-1 change of the Tap parameter (D-014).
    bool tapped (const float* in) noexcept;
    // The LFO rate of this sample in Hz: the LFO Rate knob, or 1 / the tapped period while the tap holds control.
    double lfoRateHz (bool tap) noexcept;
    // The delay time of this sample in seconds (D-026); advances the LFO and drives the `rate` meter.
    double delaySeconds (bool tap) noexcept;
    // The wet filter's warping term for the cutoff min(16 kHz, 200 / d) (D-036); recomputed only when d changes.
    float wetFilterG (double delay) noexcept;

    int outputParam_, timeParam_, rangeParam_, feedbackParam_, rateParam_, amountParam_, waveParam_, mixParam_, tapParam_;
    int returnInput_, tapInput_;
    int leftOut_, rightOut_, sendOut_;
    int rateMeter_;

    ClockedLine line_;
    StateVariableFilter wetFilter_;
    Lfo lfo_;
    TapTempo tapTempo_;
    TriggerDetector tapTrigger_;
    std::int64_t clock_ = 0;        // samples since reset(): the time base of the tap tempo
    int lastTapStep_ = 0;
    bool tapControl_ = false;       // the tapped period, not the knob, sets the LFO rate
    float tapKnob_ = 0.0f;          // LFO Rate knob position (CV included) at the tap that made the period available
    double filterDelay_ = -1.0;     // the delay for which filterG_ was computed; negative: none yet
    float filterG_ = 0.0f;
};

FluffSwarm::FluffSwarm()
    : EffectBase (EffectId::Chorus),
      outputParam_ (paramIndex ("output")), timeParam_ (paramIndex ("time")), rangeParam_ (paramIndex ("range")),
      feedbackParam_ (paramIndex ("feedback")), rateParam_ (paramIndex ("lfo_rate")), amountParam_ (paramIndex ("lfo_amount")),
      waveParam_ (paramIndex ("lfo_wave")), mixParam_ (paramIndex ("mix")), tapParam_ (paramIndex ("tap")),
      returnInput_ (inputIndex ("fb_return")), tapInput_ (inputIndex ("tap_in")),
      leftOut_ (outputIndex ("out_l")), rightOut_ (outputIndex ("out_r")), sendOut_ (outputIndex ("fb_send")),
      rateMeter_ (findMeter (info(), "rate")) {}

float FluffSwarm::tailSeconds() const noexcept {
    const float g = kFeedbackScale * std::fabs (getParam (feedbackParam_));
    if (g >= 1.0f) return kTailForever;
    if (g <= kTailNegligibleGain) return kTailLongestDelay + kTailMargin;
    const float t = kTailLongestDelay * (1.0f + kTailSixtyDbNepers / -std::log (g)) + kTailMargin;
    return t > kTailMaxSeconds ? kTailForever : t;
}

void FluffSwarm::onPrepare() {
    lfo_.prepare (sampleRate());
    line_.size (static_cast<int> (kLineSeconds * sampleRate()) + kLineExtraSamples);
}

void FluffSwarm::onReset() noexcept {
    line_.clear();
    wetFilter_.reset();
    lfo_.reset();
    tapTempo_.reset();
    tapTrigger_.reset();
    clock_ = 0;
    lastTapStep_ = step (tapParam_);
    tapControl_ = false;
    tapKnob_ = 0.0f;
    filterDelay_ = -1.0;
}

bool FluffSwarm::tapped (const float* in) noexcept {
    const bool jack = tapTrigger_.rising (in[tapInput_]);
    const int tapStep = step (tapParam_);
    const bool button = tapStep == 1 && lastTapStep_ == 0;
    lastTapStep_ = tapStep;
    return jack || button;
}

double FluffSwarm::lfoRateHz (bool tap) noexcept {
    const float knob = norm (rateParam_);
    if (tap && tapTempo_.tap (static_cast<double> (clock_) / sampleRate())) {
        tapControl_ = true;
        tapKnob_ = knob;
    }
    if (tapControl_ && std::fabs (knob - tapKnob_) > kTapReleaseTravel) tapControl_ = false;
    if (!tapControl_) return static_cast<double> (value (rateParam_));
    // A period is always available here: tapControl_ is set only by a tap that made one available.
    return std::min (std::max (1.0 / tapTempo_.periodSeconds(), kTappedMinRateHz), kTappedMaxRateHz);
}

double FluffSwarm::delaySeconds (bool tap) noexcept {
    const float lfo = lfo_.tick (static_cast<LfoWave> (step (waveParam_)), lfoRateHz (tap));
    setMeter (rateMeter_, lfo_.phase() < kRateMeterHalfCycle ? 1.0f : 0.0f);
    const bool chorusRange = step (rangeParam_) == 1;
    const double lo = chorusRange ? kChorusMinSeconds : kFlangeMinSeconds;
    const double hi = chorusRange ? kChorusMaxSeconds : kFlangeMaxSeconds;
    const double exponent = static_cast<double> (norm (timeParam_)) + kLfoSpanPerAmount * static_cast<double> (value (amountParam_) * lfo);
    return std::min (std::max (lo * std::pow (hi / lo, exponent), lo), hi);
}

float FluffSwarm::wetFilterG (double delay) noexcept {
    if (delay != filterDelay_) {
        filterDelay_ = delay;
        filterG_ = prewarp (std::min (kWetMaxCutoffHz, kWetCutoffTimesDelay / delay), sampleRate());
    }
    return filterG_;
}

void FluffSwarm::tick (const float* in, float* out) noexcept {
    const float x = drive (in[0]);
    trackLevel (x);
    const double d = delaySeconds (tapped (in));
    ++clock_;

    const float wet = wetFilter_.tick (line_.read(), wetFilterG (d), kButterworthDamping).low;
    const float returned = connected (returnInput_) ? in[returnInput_] : wet;
    line_.write (x + kFeedbackScale * value (feedbackParam_) * softClip (returned), d * sampleRate());

    // D-018: bypassed, every audio output is the input at unity gain; the line keeps running unheard.
    const float a = activeGain();
    const float level = dbToGain (value (outputParam_));
    const float mix = norm (mixParam_);
    const float dry = (1.0f - mix) * x;
    const float bypassed = (1.0f - a) * in[0];
    out[leftOut_] = a * level * (dry + mix * wet) + bypassed;
    out[rightOut_] = a * level * (dry - mix * wet) + bypassed;      // C-PEDC-035: the wet part inverted
    out[sendOut_] = a * wet + bypassed;
}

} // namespace

std::unique_ptr<Effect> makeChorus() { return std::make_unique<FluffSwarm>(); }

} // namespace muff::detail
