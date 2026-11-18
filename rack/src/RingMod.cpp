// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelRingMod = rack::createModel<MuffModule<muff::EffectId::RingMod>, MuffWidget<muff::EffectId::RingMod>>("RingMod");
