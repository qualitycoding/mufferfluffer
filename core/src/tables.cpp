// mufferfluffer DSP core: effect description tables, lookups and the parameter normalisation maps (D-011).
// SPDX-License-Identifier: AGPL-3.0-only
#include "tables.hpp"

#include "muff/core.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace muff {
namespace {
#include "generated_tables.inc"

constexpr int kNumPatterns = 24;
constexpr int kNumPatternChannels = 8;

template <typename Spec>
int findById (const Spec* specs, int count, const char* id) noexcept {
    if (id == nullptr) return -1;
    for (int i = 0; i < count; ++i)
        if (std::strcmp (specs[i].id, id) == 0) return i;
    return -1;
}

float clampTo (float value, float lo, float hi) noexcept { return std::min (std::max (value, lo), hi); }
} // namespace

const EffectInfo& effectInfo (EffectId id) {
    const int index = static_cast<int> (id);
    if (index < 0 || index >= kNumEffects) throw std::out_of_range ("effectInfo: invalid effect id");
    return kEffectInfos[index];
}

int findParam (const EffectInfo& info, const char* id) noexcept { return findById (info.params, info.numParams, id); }
int findInput (const EffectInfo& info, const char* id) noexcept { return findById (info.inputs, info.numInputs, id); }
int findOutput (const EffectInfo& info, const char* id) noexcept { return findById (info.outputs, info.numOutputs, id); }
int findMeter (const EffectInfo& info, const char* id) noexcept { return findById (info.meters, info.numMeters, id); }

float toNormalised (const ParamSpec& spec, float value) noexcept {
    if (! std::isfinite (value)) value = spec.def;
    if (spec.taper == Taper::Stepped) {
        if (spec.numSteps < 2) return 0.0f;
        const float step = std::round (clampTo (value, 0.0f, static_cast<float> (spec.numSteps - 1)));
        return step / static_cast<float> (spec.numSteps - 1);
    }
    const double v = clampTo (value, spec.min, spec.max);
    double n;
    if (spec.taper == Taper::Log) n = std::log (v / spec.min) / std::log (static_cast<double> (spec.max) / spec.min);
    else n = (v - spec.min) / (static_cast<double> (spec.max) - spec.min);
    return clampTo (static_cast<float> (n), 0.0f, 1.0f);
}

float fromNormalised (const ParamSpec& spec, float normalised) noexcept {
    if (! std::isfinite (normalised)) return spec.def;
    const double n = clampTo (normalised, 0.0f, 1.0f);
    if (spec.taper == Taper::Stepped) {
        if (spec.numSteps < 2) return 0.0f;
        return std::round (static_cast<float> (n * (spec.numSteps - 1)));
    }
    double v;
    if (spec.taper == Taper::Log) v = spec.min * std::pow (static_cast<double> (spec.max) / spec.min, n);
    else v = spec.min + n * (static_cast<double> (spec.max) - spec.min);
    return clampTo (static_cast<float> (v), spec.min, spec.max);
}

namespace detail {

int patternLength (int pattern, int channel) noexcept {
    if (pattern < 0 || pattern >= kNumPatterns || channel < 0 || channel >= kNumPatternChannels) return 0;
    return kPatterns[pattern][channel].length;
}

unsigned long long patternMask (int pattern, int channel) noexcept {
    if (pattern < 0 || pattern >= kNumPatterns || channel < 0 || channel >= kNumPatternChannels) return 0ULL;
    return kPatterns[pattern][channel].mask;
}

} // namespace detail
} // namespace muff
