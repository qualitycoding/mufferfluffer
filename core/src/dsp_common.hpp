// mufferfluffer DSP core: small stateful building blocks shared by several effects (private header).
// Declarations and trivial accessors only: every member function with logic is defined in dsp_common.cpp, so that
// mutation testing (whose scope is the .cpp files) covers it.
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "muff/blocks.hpp"

#include <cstdint>

namespace muff::detail {

// 10^(db / 20).
float dbToGain (float db) noexcept;

// tan(pi * fc / fs) with fc limited to 0.45 fs: the frequency-warping term `g` of the trapezoidal filters (D-030, D-032, D-034).
float prewarp (double cutoffHz, double sampleRate) noexcept;

// One-pole smoother: y += (1 - exp(-1 / (tau fs))) * (target - y) (D-017).
class Smoother {
public:
    void configure (double sampleRate, double timeConstantMs) noexcept;
    void snap (float value) noexcept { y_ = value; }
    float next (float target) noexcept;
private:
    float coefficient_ = 1.0f;
    float y_ = 0.0f;
};

// Follower of the rectified signal with separate attack and release one-poles, no scaling (D-030, D-035).
class EnvelopeFollower {
public:
    void configure (double sampleRate, double attackMs, double releaseMs) noexcept;
    void reset() noexcept { y_ = 0.0f; }
    float next (float x) noexcept;
private:
    float attack_ = 1.0f;
    float release_ = 1.0f;
    float y_ = 0.0f;
};

// Low-frequency oscillator of D-015: a phase accumulator that starts at phase 0 after reset(). tick() returns the
// shape at the present phase and then advances the phase by hz / fs. For LfoWave::RandomStep a new uniform value in
// [-1, 1) is drawn from a generator seeded identically at every reset() each time the phase wraps; before the first
// wrap the held value is 0.
class Lfo {
public:
    void prepare (double sampleRate) noexcept { sampleRate_ = sampleRate; }
    void reset() noexcept;
    float tick (LfoWave wave, double hz) noexcept;
    double phase() const noexcept { return phase_; }
private:
    double sampleRate_ = 48000.0;
    double phase_ = 0.0;
    float held_ = 0.0f;
    std::uint32_t random_ = 0;
};

// Schmitt trigger of D-012: fires on a rising edge through 0.2 CV units, re-arms below 0.02.
class TriggerDetector {
public:
    void reset() noexcept { high_ = false; }
    bool rising (float value) noexcept;
private:
    bool high_ = false;
};

struct SvfOutputs { float low; float band; float high; };

// Zavalishin trapezoidal state-variable filter (D-034). g = prewarp(fc, fs); k = 1 / Q (sqrt(2) for a two-pole
// Butterworth low-pass). The unity-gain band-pass of D-034 is k * band.
class StateVariableFilter {
public:
    void reset() noexcept { s1_ = 0.0f; s2_ = 0.0f; }
    SvfOutputs tick (float x, float g, float k) noexcept;
private:
    float s1_ = 0.0f;
    float s2_ = 0.0f;
};

} // namespace muff::detail
