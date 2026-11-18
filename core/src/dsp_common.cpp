// mufferfluffer DSP core: shared stateful building blocks (see dsp_common.hpp).
// SPDX-License-Identifier: AGPL-3.0-only
#include "dsp_common.hpp"

#include <algorithm>
#include <cmath>

namespace muff::detail {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMaxCutoffFraction = 0.45;         // of the sample rate
constexpr float kTriggerThreshold = 0.2f;           // D-012: 1 V
constexpr float kTriggerRearm = 0.02f;              // D-012: 0.1 V
constexpr std::uint32_t kLfoSeed = 0x12345678u;
constexpr double kLfoRandomScale = 1.0 / 8388608.0; // 2^-23: maps 24 random bits to [0, 2)

// One-pole coefficient for a time constant in milliseconds.
float onePoleCoefficient (double sampleRate, double timeConstantMs) noexcept {
    return static_cast<float> (1.0 - std::exp (-1.0 / (0.001 * timeConstantMs * sampleRate)));
}

} // namespace

float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }

float prewarp (double cutoffHz, double sampleRate) noexcept {
    return static_cast<float> (std::tan (kPi * std::min (cutoffHz, kMaxCutoffFraction * sampleRate) / sampleRate));
}

void Smoother::configure (double sampleRate, double timeConstantMs) noexcept {
    coefficient_ = onePoleCoefficient (sampleRate, timeConstantMs);
}

float Smoother::next (float target) noexcept {
    y_ += coefficient_ * (target - y_);
    return y_;
}

void EnvelopeFollower::configure (double sampleRate, double attackMs, double releaseMs) noexcept {
    attack_ = onePoleCoefficient (sampleRate, attackMs);
    release_ = onePoleCoefficient (sampleRate, releaseMs);
}

float EnvelopeFollower::next (float x) noexcept {
    const float rectified = std::fabs (x);
    y_ += (rectified > y_ ? attack_ : release_) * (rectified - y_);
    return y_;
}

void Lfo::reset() noexcept {
    phase_ = 0.0;
    held_ = 0.0f;
    random_ = kLfoSeed;
}

float Lfo::tick (LfoWave wave, double hz) noexcept {
    const float value = lfoShape (wave, static_cast<float> (phase_), held_);
    if (std::isfinite (hz) && hz > 0.0) phase_ += hz / sampleRate_;
    if (phase_ >= 1.0) {
        phase_ -= std::floor (phase_);
        random_ = random_ * 1664525u + 1013904223u;
        held_ = static_cast<float> ((random_ >> 8) * kLfoRandomScale - 1.0);
    }
    return value;
}

bool TriggerDetector::rising (float value) noexcept {
    if (! high_ && value >= kTriggerThreshold) {
        high_ = true;
        return true;
    }
    if (high_ && value < kTriggerRearm) high_ = false;
    return false;
}

SvfOutputs StateVariableFilter::tick (float x, float g, float k) noexcept {
    const float high = (x - (k + g) * s1_ - s2_) / (1.0f + k * g + g * g);
    const float v1 = g * high;
    const float band = v1 + s1_;
    s1_ = band + v1;
    const float v2 = g * band;
    const float low = v2 + s2_;
    s2_ = low + v2;
    return { low, band, high };
}

} // namespace muff::detail
