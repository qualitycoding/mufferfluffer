// mufferfluffer DSP core: the private base class of the seven effects (see effect_base.hpp).
// SPDX-License-Identifier: AGPL-3.0-only
#include "effect_base.hpp"

#include "muff/blocks.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace muff::detail {
namespace {

constexpr double kSmoothingMs = 5.0;              // D-017
constexpr double kMeterDecaySeconds = 0.1;        // meter `level`: 100 ms decay
constexpr float kMeterMin = 0.0f;
constexpr float kMeterMax = 1.0f;
constexpr float kDecibelsToNaturalLog = 0.11512925464970229f;    // ln(10) / 20

float clamp (float v, float lo, float hi) noexcept { return std::min (std::max (v, lo), hi); }

// Flush-to-zero and denormals-are-zero for the duration of one process() call, restoring the caller's mode (D-012).
class ScopedNoDenormals {
public:
#if defined(__SSE__) || defined(_M_X64) || defined(__x86_64__)
    ScopedNoDenormals() noexcept : saved_ (_mm_getcsr()) { _mm_setcsr (saved_ | 0x8040u); }
    ~ScopedNoDenormals() { _mm_setcsr (saved_); }
private:
    unsigned int saved_;
#elif defined(__aarch64__)
    ScopedNoDenormals() noexcept {
        __asm__ volatile ("mrs %0, fpcr" : "=r" (saved_));
        const unsigned long long flushed = saved_ | (1ULL << 24);
        __asm__ volatile ("msr fpcr, %0" : : "r" (flushed));
    }
    ~ScopedNoDenormals() { __asm__ volatile ("msr fpcr, %0" : : "r" (saved_)); }
private:
    unsigned long long saved_ = 0;
#else
    ScopedNoDenormals() noexcept {}
#endif
    ScopedNoDenormals (const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator= (const ScopedNoDenormals&) = delete;
};

} // namespace

EffectBase::EffectBase (EffectId id) : info_ (effectInfo (id)) {
    for (int p = 0; p < info_.numParams; ++p) {
        target_[p] = info_.params[p].def;
        targetNorm_[p] = toNormalised (info_.params[p], info_.params[p].def);
        cvInput_[p] = -1;
    }
    for (int i = 0; i < info_.numInputs; ++i) {
        connected_[i] = (i == 0);
        const PortSpec& port = info_.inputs[i];
        if (port.kind == PortKind::CvNormalised && port.param >= 0) {
            cvInput_[port.param] = i;
            cvDepth_[port.param] = port.depth;
        }
    }
    activeParam_ = findParam (info_, "active");
    driveParam_ = findParam (info_, "drive");
    levelMeter_ = findMeter (info_, "level");
    activeMeter_ = findMeter (info_, "active");
}

void EffectBase::prepare (double sampleRate) {
    if (! std::isfinite (sampleRate) || sampleRate < kMinSampleRate || sampleRate > kMaxSampleRate)
        throw std::invalid_argument ("sample rate out of range");
    sampleRate_ = sampleRate;
    for (int p = 0; p < info_.numParams; ++p) smoothers_[p].configure (sampleRate_, kSmoothingMs);
    activeSmoother_.configure (sampleRate_, kSmoothingMs);
    meterDecay_ = static_cast<float> (std::exp (-1.0 / (kMeterDecaySeconds * sampleRate_)));
    onPrepare();
    prepared_ = true;
    reset();
}

void EffectBase::reset() noexcept {
    for (int p = 0; p < info_.numParams; ++p) {
        smoothers_[p].snap (targetNorm_[p]);        // no CV at reset (D-017)
        position_[p] = targetNorm_[p];
    }
    activeGain_ = activeParam_ >= 0 ? target_[activeParam_] : 1.0f;
    activeSmoother_.snap (activeGain_);
    driveGain_ = driveParam_ >= 0 ? dbToGain (target_[driveParam_]) : 1.0f;
    for (float& meter : meters_) meter = 0.0f;
    if (activeMeter_ >= 0) meters_[activeMeter_] = activeGain_;
    onReset();
}

void EffectBase::setParam (int index, float value) noexcept {
    if (index < 0 || index >= info_.numParams || ! std::isfinite (value)) return;
    const ParamSpec& spec = info_.params[index];
    value = clamp (value, spec.min, spec.max);
    if (spec.taper == Taper::Stepped) value = std::round (value);
    target_[index] = value;
    targetNorm_[index] = toNormalised (spec, value);
}

float EffectBase::getParam (int index) const noexcept {
    return (index < 0 || index >= info_.numParams) ? 0.0f : target_[index];
}

void EffectBase::setInputConnected (int input, bool connected) noexcept {
    if (input >= 0 && input < info_.numInputs) connected_[input] = connected;
}

float EffectBase::getMeter (int index) const noexcept {
    return (index < 0 || index >= info_.numMeters) ? 0.0f : clamp (meters_[index], kMeterMin, kMeterMax);
}

int EffectBase::paramIndex (const char* id) const noexcept { return findParam (info_, id); }
int EffectBase::inputIndex (const char* id) const noexcept { return findInput (info_, id); }
int EffectBase::outputIndex (const char* id) const noexcept { return findOutput (info_, id); }

bool EffectBase::connected (int input) const noexcept {
    return input >= 0 && input < info_.numInputs && connected_[input];
}

float EffectBase::value (int param) const noexcept { return fromNormalised (info_.params[param], position_[param]); }

float EffectBase::drive (float x) const noexcept { return softClip (driveGain_ * x); }

void EffectBase::trackLevel (float driven) noexcept {
    if (levelMeter_ >= 0) meters_[levelMeter_] = std::max (std::fabs (driven), meters_[levelMeter_] * meterDecay_);
}

void EffectBase::setMeter (int index, float value) noexcept {
    if (index >= 0 && index < info_.numMeters) meters_[index] = value;
}

void EffectBase::advanceControls (const float* in) noexcept {
    for (int p = 0; p < info_.numParams; ++p) {
        if (info_.params[p].taper == Taper::Stepped) continue;
        float n = targetNorm_[p];
        if (cvInput_[p] >= 0) n = clamp (n + cvDepth_[p] * in[cvInput_[p]], 0.0f, 1.0f);
        position_[p] = smoothers_[p].next (n);
    }
    activeGain_ = activeSmoother_.next (activeParam_ >= 0 ? target_[activeParam_] : 1.0f);
    if (activeMeter_ >= 0) meters_[activeMeter_] = activeGain_;
    if (driveParam_ >= 0) driveGain_ = std::exp (value (driveParam_) * kDecibelsToNaturalLog);
}

void EffectBase::process (const float* in, float* out) noexcept {
    if (! prepared_) {
        for (int o = 0; o < info_.numOutputs; ++o) out[o] = 0.0f;
        return;
    }
    const ScopedNoDenormals noDenormals;
    float inputs[kMaxPorts];
    for (int i = 0; i < info_.numInputs; ++i)
        inputs[i] = (connected_[i] && std::isfinite (in[i])) ? clamp (in[i], -kInputLimit, kInputLimit) : 0.0f;
    advanceControls (inputs);
    tick (inputs, out);
    bool finite = true;
    for (int o = 0; o < info_.numOutputs; ++o) finite = finite && std::isfinite (out[o]);
    if (! finite) {         // a state went non-finite: start again from silence (T-007)
        reset();
        for (int o = 0; o < info_.numOutputs; ++o) out[o] = 0.0f;
        return;
    }
    for (int o = 0; o < info_.numOutputs; ++o) out[o] = clamp (out[o], -kOutputLimit, kOutputLimit);
}

} // namespace muff::detail
