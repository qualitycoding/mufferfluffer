// mufferfluffer DSP core: RingMod, "Fluff Ring" (D-021, D-031; bypass D-018, LFO D-015).
// SPDX-License-Identifier: AGPL-3.0-only
#include "effect_base.hpp"
#include "factories.hpp"

#include <algorithm>
#include <cmath>

namespace muff::detail {
namespace {

constexpr double kTwoPi = 6.28318530717958647692;

// Carrier ranges (D-021): base = low * (high / low)^knob.
constexpr double kLoRangeLowHz = 0.6;
constexpr double kLoRangeHighHz = 80.0;
constexpr double kHiRangeLowHz = 30.0;
constexpr double kHiRangeHighHz = 4000.0;
constexpr int kRangeHi = 1;
constexpr double kMaxCarrierFraction = 0.45;            // of the sample rate (D-021)
constexpr double kOctavesPerSweepUnit = 0.5;            // lfo_amount is peak to peak, so half of it each way (D-021)

// External carrier (D-021): 0.5 V RMS (0.1 core units) gives a unit-amplitude carrier, limited to +/-2.
constexpr float kExternalCarrierGain = 7.0711f;
constexpr float kExternalCarrierLimit = 2.0f;

// Monitoring outputs (D-021).
constexpr float kCarrierOutGain = 0.25f;
constexpr float kLfoOutGain = 0.3f;

constexpr int kLfoWaveSquare = 1;                        // step index of the `lfo_wave` parameter

double carrierBaseHz (int range, float knob) noexcept {
    const double low = range == kRangeHi ? kHiRangeLowHz : kLoRangeLowHz;
    const double high = range == kRangeHi ? kHiRangeHighHz : kLoRangeHighHz;
    return low * std::pow (high / low, static_cast<double> (knob));
}

LfoWave lfoWaveFor (int step) noexcept { return step == kLfoWaveSquare ? LfoWave::Square : LfoWave::Sine; }

float externalCarrier (float carrierIn) noexcept {
    return std::min (std::max (kExternalCarrierGain * carrierIn, -kExternalCarrierLimit), kExternalCarrierLimit);
}

class RingMod final : public EffectBase {
public:
    RingMod();
    float tailSeconds() const noexcept override;

private:
    void onPrepare() override;
    void onReset() noexcept override;
    void tick (const float* in, float* out) noexcept override;

    float advanceOscillator (float lfo) noexcept;

    int frequencyParam_ = -1;
    int rangeParam_ = -1;
    int lfoRateParam_ = -1;
    int lfoAmountParam_ = -1;
    int lfoWaveParam_ = -1;
    int mixParam_ = -1;
    int carrierIn_ = -1;
    int carrierOut_ = -1;
    int lfoOut_ = -1;
    int lfoMeter_ = -1;
    Lfo lfo_;
    double phase_ = 0.0;                    // carrier phase in cycles, [0, 1)
};

RingMod::RingMod() : EffectBase (EffectId::RingMod) {
    frequencyParam_ = paramIndex ("frequency");
    rangeParam_ = paramIndex ("range");
    lfoRateParam_ = paramIndex ("lfo_rate");
    lfoAmountParam_ = paramIndex ("lfo_amount");
    lfoWaveParam_ = paramIndex ("lfo_wave");
    mixParam_ = paramIndex ("mix");
    carrierIn_ = inputIndex ("carrier_in");
    carrierOut_ = outputIndex ("carrier_out");
    lfoOut_ = outputIndex ("lfo_out");
    lfoMeter_ = findMeter (info(), "lfo");
}

// Output stays within 0.1 s of the input: the ideal multiplier has no memory (D-031); the 5 ms smoothing is the only lag.
float RingMod::tailSeconds() const noexcept { return 0.1f; }

void RingMod::onPrepare() { lfo_.prepare (sampleRate()); }

void RingMod::onReset() noexcept {
    lfo_.reset();
    phase_ = 0.0;
}

// Advances the carrier by one sample, its frequency swept by the LFO value (D-021), and returns the oscillator output.
float RingMod::advanceOscillator (float lfo) noexcept {
    const double sweep = std::exp2 (kOctavesPerSweepUnit * static_cast<double> (value (lfoAmountParam_)) * static_cast<double> (lfo));
    const double hz = std::min (carrierBaseHz (step (rangeParam_), norm (frequencyParam_)) * sweep, kMaxCarrierFraction * sampleRate());
    const float carrier = static_cast<float> (std::sin (kTwoPi * phase_));
    phase_ += hz / sampleRate();
    phase_ -= std::floor (phase_);
    return carrier;
}

void RingMod::tick (const float* in, float* out) noexcept {
    const float driven = drive (in[0]);
    trackLevel (driven);

    const float lfo = lfo_.tick (lfoWaveFor (step (lfoWaveParam_)), static_cast<double> (value (lfoRateParam_)));
    const float oscillator = advanceOscillator (lfo);
    const float carrier = connected (carrierIn_) ? externalCarrier (in[carrierIn_]) : oscillator;

    const float mix = norm (mixParam_);
    const float wet = driven * carrier;
    const float processed = (1.0f - mix) * driven + mix * wet;
    const float on = activeGain();

    out[0] = on * processed + (1.0f - on) * driven;
    out[carrierOut_] = kCarrierOutGain * oscillator;
    out[lfoOut_] = kLfoOutGain * lfo;
    setMeter (lfoMeter_, 0.5f * (lfo + 1.0f));
}

} // namespace

std::unique_ptr<muff::Effect> makeRingMod() { return std::make_unique<RingMod>(); }

} // namespace muff::detail
