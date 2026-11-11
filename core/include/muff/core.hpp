// mufferfluffer DSP core — public interface (frozen by plan decision D-011; see plan/DECISIONS.md).
// SPDX-License-Identifier: AGPL-3.0-only
//
// Conventions (D-012):
//  * Audio samples are floats in "core units": 1.0 is nominal full scale. The Rack adaptor maps 5 V to 1.0.
//  * Control-voltage (CV) samples are floats in "CV units": 1.0 corresponds to +5 V.
//  * Every effect has one mono audio input at input index 0. Further inputs are CV, trigger or auxiliary audio inputs.
//  * An Effect is used from one thread at a time. No method allocates, locks or blocks except prepare().
#pragma once

#include <memory>
#include <stdexcept>

namespace muff {

inline constexpr int kApiVersion = 1;

enum class EffectId : int { Lowpass = 0, RingMod = 1, Phaser = 2, Delay = 3, FilterBank = 4, OscBox = 5, Chorus = 6 };
inline constexpr int kNumEffects = 7;
inline constexpr int kMaxPorts = 16;       // upper bound on inputs and on outputs of any effect
inline constexpr double kMinSampleRate = 8000.0;
inline constexpr double kMaxSampleRate = 768000.0;
inline constexpr float kOutputLimit = 4.0f; // |output sample| never exceeds this (SC-7)

enum class Taper : int { Linear = 0, Log = 1, Stepped = 2 };

// One user-facing control. Values are in the stated unit, between min and max inclusive.
// For Taper::Stepped the value is an integer step index in [0, numSteps-1] and stepNames has numSteps entries.
struct ParamSpec {
    const char* id;
    const char* name;
    const char* unit;
    float min;
    float max;
    float def;
    Taper taper;
    int numSteps;
    const char* const* stepNames;
};

enum class PortKind : int {
    Audio = 0,          // audio-rate signal in core units
    CvNormalised = 1,   // input: adds depth * cv to the target parameter's normalised position, result clamped to [0,1];
                        // output: a control signal in CV units
    CvVoltPerOct = 2,   // input: multiplies the target frequency by 2^(5 * cv) (1 V per octave), not limited to the knob range
    Trigger = 3         // input: a rising edge through 0.2 CV units (1 V) is one trigger; re-arms below 0.02 (0.1 V)
};

struct PortSpec {
    const char* id;
    const char* name;
    PortKind kind;
    int param;      // index of the parameter a CV or trigger input acts on, or -1
    float depth;    // CvNormalised inputs: fraction of the knob range swept by 1.0 CV unit; otherwise 0
};

// A display value for an indicator lamp (level, rate, bypass state), each in [0,1].
struct MeterSpec {
    const char* id;
    const char* name;
};

struct EffectInfo {
    EffectId id;
    const char* slug;   // lower-case identifier, [a-z]+
    const char* name;   // display name; never contains a third-party mark (D-004)
    int numParams;
    const ParamSpec* params;
    int numInputs;
    const PortSpec* inputs;
    int numOutputs;
    const PortSpec* outputs;
    int numMeters;
    const MeterSpec* meters;
    int rightOutput;    // index of the output that feeds the right channel of a stereo host, or -1 (right = copy of output 0)
    bool monoSumsRight; // true: a mono host (or an unpatched right output) receives output 0 + output rightOutput
};

// Thrown by every not-yet-implemented entry point of the stub library.
struct NotImplemented : std::logic_error {
    explicit NotImplemented (const char* what) : std::logic_error (what) {}
};

// Static description of an effect. Throws std::out_of_range if id is not a valid EffectId.
const EffectInfo& effectInfo (EffectId id);

// Index of the parameter / input / output / meter with the given id, or -1.
int findParam (const EffectInfo& info, const char* id) noexcept;
int findInput (const EffectInfo& info, const char* id) noexcept;
int findOutput (const EffectInfo& info, const char* id) noexcept;
int findMeter (const EffectInfo& info, const char* id) noexcept;

// Map between a parameter value and its normalised knob position in [0,1].
// Linear: n = (v-min)/(max-min). Log: n = ln(v/min)/ln(max/min). Stepped: n = step/(numSteps-1), value rounded to the nearest step.
// Inputs outside the range are clamped; a non-finite input maps to the default.
float toNormalised (const ParamSpec& spec, float value) noexcept;
float fromNormalised (const ParamSpec& spec, float normalised) noexcept;

class Effect {
public:
    virtual ~Effect() = default;

    virtual const EffectInfo& info() const noexcept = 0;

    // Allocates and sizes all state for the given sample rate, then calls reset(). May be called repeatedly.
    // Throws std::invalid_argument unless kMinSampleRate <= sampleRate <= kMaxSampleRate and sampleRate is finite.
    virtual void prepare (double sampleRate) = 0;

    // Clears audio state (filters, delay lines, envelopes, oscillator phases, tap-tempo history). Keeps parameter values.
    virtual void reset() noexcept = 0;

    // Sets a parameter to a value in its unit. Out-of-range values are clamped, stepped values are rounded,
    // non-finite values and invalid indices are ignored. Continuous parameters are smoothed inside the effect.
    virtual void setParam (int index, float value) noexcept = 0;
    // Returns the last accepted (clamped, rounded) target value; the default before any call; 0 for an invalid index.
    virtual float getParam (int index) const noexcept = 0;

    // Tells the effect whether something is patched into an input. All inputs start disconnected except input 0.
    // A disconnected input reads as 0 whatever is passed to process(). Invalid indices are ignored.
    virtual void setInputConnected (int input, bool connected) noexcept = 0;

    // Processes one sample frame. `in` holds info().numInputs values, `out` receives info().numOutputs values.
    // Non-finite input values are treated as 0. Every output is finite and within +/- kOutputLimit.
    // Must be preceded by prepare(); before prepare() it writes zeros.
    virtual void process (const float* in, float* out) noexcept = 0;

    // Current indicator value in [0,1]; 0 for an invalid index.
    virtual float getMeter (int index) const noexcept = 0;

    // Delay in samples between input and the dry-aligned output, constant between prepare() calls.
    virtual int latencySamples() const noexcept = 0;

    // Upper bound, in seconds, on how long the output stays above -60 dB (re 1.0) after the input stops, for the
    // current parameter values and no CV; returns 1.0e9f when the effect can sound for ever (self-oscillation).
    virtual float tailSeconds() const noexcept = 0;
};

// Creates an effect with every parameter at its default. Throws std::out_of_range for an invalid id.
std::unique_ptr<Effect> createEffect (EffectId id);

} // namespace muff
