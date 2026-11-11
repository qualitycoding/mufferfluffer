// mufferfluffer — framework-free host adaptors shared by the VST3 and the VCV Rack shells (D-013).
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "muff/core.hpp"

#include <memory>

namespace muff {

// Drives one Effect from a block-based stereo or mono host (the VST3 shell).
class BlockRunner {
public:
    explicit BlockRunner (EffectId id);         // throws std::out_of_range for an invalid id
    ~BlockRunner();
    Effect& effect() noexcept;
    void prepare (double sampleRate);           // forwards to Effect::prepare
    // in:  numIn channel pointers, numIn in {0, 1, 2}. 0: silence. 2: the effect's audio input is 0.5 * (L + R).
    // out: numOut channel pointers, numOut in {1, 2}.
    //   numOut == 2: out[0] = effect output 0; out[1] = effect output info().rightOutput, or a copy of out[0] if that is -1.
    //   numOut == 1: out[0] = effect output 0, plus output info().rightOutput when info().monoSumsRight is true.
    // Every other effect input (CV, trigger, auxiliary audio) is fed 0 and left disconnected.
    // in and out may alias (in-place processing). The result does not depend on how a signal is cut into blocks.
    // Other channel counts or numFrames <= 0: does nothing.
    void process (const float* const* in, int numIn, float* const* out, int numOut, int numFrames) noexcept;
private:
    std::unique_ptr<Effect> fx_;
};

// One frame of VCV Rack port voltages for an effect (indices are the effect's input and output indices).
struct RackIO {
    float inVolts[kMaxPorts];
    bool inConnected[kMaxPorts];
    float outVolts[kMaxPorts];
    bool outConnected[kMaxPorts];
};

// Drives one Effect from per-sample Rack voltages.
// Inputs: core value = volts / 5 for every port kind; connection flags are forwarded to Effect::setInputConnected.
// Outputs: volts = core value * 5. When info().monoSumsRight is true and the right output is not connected,
// outVolts[0] also carries the right output (outVolts[rightOutput] is still written).
class RackBridge {
public:
    explicit RackBridge (EffectId id);
    ~RackBridge();
    Effect& effect() noexcept;
    void prepare (double sampleRate);
    void process (RackIO& io) noexcept;
private:
    std::unique_ptr<Effect> fx_;
    bool conn_[kMaxPorts];
};

} // namespace muff
