// mufferfluffer DSP core: the private base class of the seven effects (D-011, D-012, D-015, D-017, D-018).
// SPDX-License-Identifier: AGPL-3.0-only
//
// It owns everything the frozen Effect interface asks of every effect: parameter storage with clamping and
// rounding, connection flags, input sanitising, the 5 ms smoothing of knob positions (control voltages included),
// the Active cross-fade, the level meter, flush-to-zero for the duration of process(), output limiting and the
// reset after a non-finite result. An effect derives from it and supplies onPrepare(), onReset(), tick() and
// tailSeconds(). Declarations and trivial accessors only: member functions with logic are in effect_base.cpp.
#pragma once

#include "dsp_common.hpp"

#include "muff/core.hpp"

namespace muff::detail {

inline constexpr int kMaxParams = 24;         // upper bound on the parameters of any effect (the largest has 19)
inline constexpr int kMaxMeters = 4;          // upper bound on the meters of any effect (all have 3)
inline constexpr float kInputLimit = 16.0f;   // finite inputs are limited to +/-16 before use (D-012)

class EffectBase : public Effect {
public:
    const EffectInfo& info() const noexcept final { return info_; }
    void prepare (double sampleRate) final;
    void reset() noexcept final;
    void setParam (int index, float value) noexcept final;
    float getParam (int index) const noexcept final;
    void setInputConnected (int input, bool connected) noexcept final;
    void process (const float* in, float* out) noexcept final;
    float getMeter (int index) const noexcept final;
    int latencySamples() const noexcept final { return 0; }     // no effect delays its dry path

protected:
    explicit EffectBase (EffectId id);

    // Called by prepare() once sampleRate() is known and before reset(): size buffers and configure filters.
    virtual void onPrepare() = 0;
    // Called by reset() (and after a non-finite result): clear every audio state of the effect.
    virtual void onReset() noexcept = 0;
    // One sample frame. `in` holds info().numInputs sanitised values (a disconnected input reads 0, a non-finite one
    // reads 0, finite ones are limited to +/-16); `out` receives info().numOutputs values. The base class has already
    // advanced every smoother, so norm(), value() and activeGain() describe this sample. The base class limits the
    // outputs to +/-kOutputLimit afterwards; an effect must keep its own state finite.
    virtual void tick (const float* in, float* out) noexcept = 0;

    double sampleRate() const noexcept { return sampleRate_; }
    int paramIndex (const char* id) const noexcept;     // -1 when absent
    int inputIndex (const char* id) const noexcept;
    int outputIndex (const char* id) const noexcept;
    bool connected (int input) const noexcept;          // whether something is patched into the input

    // Smoothed knob position in [0, 1] of a continuous parameter, with its normalised CV input added (and clamped).
    float norm (int param) const noexcept { return position_[param]; }
    // The same position mapped to the parameter's unit.
    float value (int param) const noexcept;
    // Current step of a stepped parameter; steps switch at once (D-017).
    int step (int param) const noexcept { return static_cast<int> (target_[param]); }
    // Cross-fade position of the Active parameter: 1 = on, 0 = bypassed, moving over 5 ms (D-017).
    float activeGain() const noexcept { return activeGain_; }
    // softClip (drive gain x) for the Drive parameter of this sample (D-037); the identity soft limiter at 0 dB.
    float drive (float x) const noexcept;
    // Level meter: peak of the driven signal with a 100 ms decay.
    void trackLevel (float driven) noexcept;
    void setMeter (int index, float value) noexcept;

private:
    void advanceControls (const float* in) noexcept;

    const EffectInfo& info_;
    double sampleRate_ = 0.0;
    bool prepared_ = false;
    int activeParam_ = -1;
    int driveParam_ = -1;
    int levelMeter_ = -1;
    int activeMeter_ = -1;
    float meterDecay_ = 0.0f;
    float driveGain_ = 1.0f;
    float activeGain_ = 1.0f;
    Smoother activeSmoother_;
    float target_[kMaxParams] {};          // last accepted value of each parameter, in its unit
    float targetNorm_[kMaxParams] {};      // the same as a knob position
    int cvInput_[kMaxParams] {};           // input index of the normalised CV input of a parameter, or -1
    float cvDepth_[kMaxParams] {};
    float position_[kMaxParams] {};        // smoothed knob position, CV included
    Smoother smoothers_[kMaxParams];
    bool connected_[kMaxPorts] {};
    float meters_[kMaxMeters] {};
};

} // namespace muff::detail
