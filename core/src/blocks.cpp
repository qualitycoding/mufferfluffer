// mufferfluffer DSP core: the small pure building blocks of blocks.hpp (D-014, D-015, D-037).
// SPDX-License-Identifier: AGPL-3.0-only
#include "muff/blocks.hpp"

#include <algorithm>
#include <cmath>

namespace muff {
namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr double kTapRunTimeoutSeconds = 5.0;     // D-014: a longer gap starts a new run
constexpr int kTapsForPeriod = 3;                 // the third tap of a run makes a period available

// Frozen table of D-024: setting, attack and decay in sequencer steps.
constexpr int kEnvelopePoints = 6;
constexpr float kEnvelopeSetting[kEnvelopePoints] = { 0.0f, 2.0f, 5.0f, 6.0f, 8.0f, 10.0f };
constexpr float kEnvelopeAttack[kEnvelopePoints] = { 0.01f, 0.01f, 1.0f, 2.0f, 1.0f, 2.0f };
constexpr float kEnvelopeDecay[kEnvelopePoints] = { 0.25f, 1.0f, 1.0f, 2.0f, 0.02f, 0.02f };
constexpr float kEnvelopeDefaultSetting = 2.0f;

} // namespace

float softClip (float x) noexcept { return 2.0f * std::tanh (0.5f * x); }

float lfoShape (LfoWave wave, float phase, float held) noexcept {
    if (! std::isfinite (phase)) return 0.0f;
    float p = phase - std::floor (phase);
    if (p >= 1.0f) p = 0.0f;        // phase - floor(phase) can round up to 1 for a tiny negative phase
    switch (wave) {
        case LfoWave::Sine: return static_cast<float> (std::sin (kTwoPi * static_cast<double> (p)));
        case LfoWave::Triangle: return 1.0f - 4.0f * std::fabs (p - 0.5f);
        case LfoWave::Square: return p < 0.5f ? 1.0f : -1.0f;
        case LfoWave::Sawtooth: return 2.0f * p - 1.0f;
        case LfoWave::Ramp: return 1.0f - 2.0f * p;
        case LfoWave::RandomStep: return held;
    }
    return 0.0f;
}

void TapTempo::reset() noexcept {
    last_ = -1.0;
    sum_ = 0.0;
    period_ = 0.0;
    count_ = 0;
}

bool TapTempo::tap (double timeSeconds) noexcept {
    if (count_ == 0 || timeSeconds - last_ > kTapRunTimeoutSeconds) {   // a new run; the old period stays available
        last_ = timeSeconds;
        sum_ = 0.0;
        count_ = 1;
        return false;
    }
    sum_ += timeSeconds - last_;
    last_ = timeSeconds;
    ++count_;
    if (count_ < kTapsForPeriod) return false;
    const double period = sum_ / static_cast<double> (count_ - 1);
    const bool changed = period != period_;
    period_ = period;
    return changed;
}

double TapTempo::periodSeconds() const noexcept { return period_; }

void filterBankEnvelopeTimes (float envelope, float& attackSteps, float& decaySteps) noexcept {
    const float setting = std::isfinite (envelope) ? std::min (std::max (envelope, kEnvelopeSetting[0]), kEnvelopeSetting[kEnvelopePoints - 1])
                                                   : kEnvelopeDefaultSetting;
    int segment = 0;
    while (segment < kEnvelopePoints - 2 && setting > kEnvelopeSetting[segment + 1]) ++segment;
    const float t = (setting - kEnvelopeSetting[segment]) / (kEnvelopeSetting[segment + 1] - kEnvelopeSetting[segment]);
    attackSteps = kEnvelopeAttack[segment] + t * (kEnvelopeAttack[segment + 1] - kEnvelopeAttack[segment]);
    decaySteps = kEnvelopeDecay[segment] + t * (kEnvelopeDecay[segment + 1] - kEnvelopeDecay[segment]);
}

} // namespace muff
