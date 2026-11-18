// mufferfluffer DSP core: the seven effect factories (private header). Each is defined by the .cpp of its effect.
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "muff/core.hpp"

#include <memory>

namespace muff::detail {

std::unique_ptr<muff::Effect> makeLowpass();
std::unique_ptr<muff::Effect> makeRingMod();
std::unique_ptr<muff::Effect> makePhaser();
std::unique_ptr<muff::Effect> makeDelay();
std::unique_ptr<muff::Effect> makeFilterBank();
std::unique_ptr<muff::Effect> makeOscBox();
std::unique_ptr<muff::Effect> makeChorus();

} // namespace muff::detail
