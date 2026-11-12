// STUB implementation of muff/adapters.hpp (plan Phase 2.2). Plan step S-018 replaces this file.
// SPDX-License-Identifier: AGPL-3.0-only
#include "muff/adapters.hpp"

namespace muff {
BlockRunner::BlockRunner (EffectId) { throw NotImplemented ("BlockRunner"); }
BlockRunner::~BlockRunner() = default;
Effect& BlockRunner::effect() noexcept { return *fx_; }
void BlockRunner::prepare (double) { throw NotImplemented ("BlockRunner::prepare"); }
void BlockRunner::process (const float* const*, int, float* const*, int, int) noexcept {}
RackBridge::RackBridge (EffectId) { throw NotImplemented ("RackBridge"); }
RackBridge::~RackBridge() = default;
Effect& RackBridge::effect() noexcept { return *fx_; }
void RackBridge::prepare (double) { throw NotImplemented ("RackBridge::prepare"); }
void RackBridge::process (RackIO&) noexcept {}
} // namespace muff
