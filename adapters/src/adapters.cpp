// mufferfluffer: host adaptors shared by the VST3 and the VCV Rack shells (D-013). Framework-free: no JUCE, no Rack.
// SPDX-License-Identifier: AGPL-3.0-only
#include "muff/adapters.hpp"

#include <algorithm>
#include <cstring>

namespace muff {
namespace {

constexpr float kVoltsPerCoreUnit = 5.0f;     // D-012: core unit 1.0 = 5 V on every port
constexpr float kRackVoltLimit = 12.0f;       // D-012: outputs are limited to +/-12 V
constexpr int kMaxHostInputChannels = 2;
constexpr int kMaxHostOutputChannels = 2;

float limitVolts (float volts) noexcept { return std::min (std::max (volts, -kRackVoltLimit), kRackVoltLimit); }

} // namespace

BlockRunner::BlockRunner (EffectId id) : fx_ (createEffect (id)) {}

BlockRunner::~BlockRunner() = default;

Effect& BlockRunner::effect() noexcept { return *fx_; }

void BlockRunner::prepare (double sampleRate) { fx_->prepare (sampleRate); }

void BlockRunner::process (const float* const* in, int numIn, float* const* out, int numOut, int numFrames) noexcept {
    if (numFrames <= 0 || numIn < 0 || numIn > kMaxHostInputChannels || numOut < 1 || numOut > kMaxHostOutputChannels) return;
    if (out == nullptr || (numIn > 0 && in == nullptr)) return;

    const EffectInfo& info = fx_->info();
    const int right = info.rightOutput;
    const bool sumRight = info.monoSumsRight && right >= 0;
    float inputs[kMaxPorts] {};     // inputs other than 0 stay 0 and disconnected
    float outputs[kMaxPorts];
    // One frame at a time, reading every input channel of the frame before writing any output channel of it, so that
    // input and output buffers may be the same memory.
    for (int i = 0; i < numFrames; ++i) {
        if (numIn == 1) inputs[0] = in[0][i];
        else if (numIn == 2) inputs[0] = 0.5f * (in[0][i] + in[1][i]);
        else inputs[0] = 0.0f;
        fx_->process (inputs, outputs);
        if (numOut == 2) {
            out[0][i] = outputs[0];
            out[1][i] = right >= 0 ? outputs[right] : outputs[0];
        } else {
            out[0][i] = sumRight ? outputs[0] + outputs[right] : outputs[0];
        }
    }
}

RackBridge::RackBridge (EffectId id) : fx_ (createEffect (id)) {
    std::memset (conn_, 0, sizeof (conn_));
    conn_[0] = true;                // a new effect has only its audio input connected
}

RackBridge::~RackBridge() = default;

Effect& RackBridge::effect() noexcept { return *fx_; }

void RackBridge::prepare (double sampleRate) { fx_->prepare (sampleRate); }

void RackBridge::process (RackIO& io) noexcept {
    const EffectInfo& info = fx_->info();
    float inputs[kMaxPorts];
    float outputs[kMaxPorts];
    for (int i = 0; i < info.numInputs; ++i) {
        if (io.inConnected[i] != conn_[i]) {
            conn_[i] = io.inConnected[i];
            fx_->setInputConnected (i, conn_[i]);
        }
        inputs[i] = io.inVolts[i] / kVoltsPerCoreUnit;
    }
    fx_->process (inputs, outputs);
    for (int o = 0; o < info.numOutputs; ++o) io.outVolts[o] = limitVolts (outputs[o] * kVoltsPerCoreUnit);
    const int right = info.rightOutput;
    if (info.monoSumsRight && right >= 0 && ! io.outConnected[right]) io.outVolts[0] = limitVolts (io.outVolts[0] + io.outVolts[right]);
}

} // namespace muff
