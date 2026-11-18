// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelDelay = rack::createModel<MuffModule<muff::EffectId::Delay>, MuffWidget<muff::EffectId::Delay>>("Delay");
