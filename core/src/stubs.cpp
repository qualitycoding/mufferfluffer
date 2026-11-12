// STUB implementation of muff/core.hpp and muff/blocks.hpp (plan Phase 2.2). Every entry point reports
// "not implemented": functions that may throw do so; noexcept functions return a value no test accepts.
// Plan step S-010 deletes this file; S-010 to S-017 write the real core. SPDX-License-Identifier: AGPL-3.0-only
#include "muff/blocks.hpp"
#include "muff/core.hpp"

#include <limits>

namespace muff {
namespace { const float kNaN = std::numeric_limits<float>::quiet_NaN(); }

const EffectInfo& effectInfo (EffectId) { throw NotImplemented ("effectInfo"); }
int findParam (const EffectInfo&, const char*) noexcept { return -2; }
int findInput (const EffectInfo&, const char*) noexcept { return -2; }
int findOutput (const EffectInfo&, const char*) noexcept { return -2; }
int findMeter (const EffectInfo&, const char*) noexcept { return -2; }
float toNormalised (const ParamSpec&, float) noexcept { return kNaN; }
float fromNormalised (const ParamSpec&, float) noexcept { return kNaN; }
std::unique_ptr<Effect> createEffect (EffectId) { throw NotImplemented ("createEffect"); }

float softClip (float) noexcept { return kNaN; }
float lfoShape (LfoWave, float, float) noexcept { return kNaN; }
void TapTempo::reset() noexcept {}
bool TapTempo::tap (double) noexcept { return false; }
double TapTempo::periodSeconds() const noexcept { return -1.0; }
void filterBankEnvelopeTimes (float, float& attackSteps, float& decaySteps) noexcept { attackSteps = kNaN; decaySteps = kNaN; }
} // namespace muff
