// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelLowpass = rack::createModel<MuffModule<muff::EffectId::Lowpass>, MuffWidget<muff::EffectId::Lowpass>>("Lowpass");
