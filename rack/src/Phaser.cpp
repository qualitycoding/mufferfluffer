// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelPhaser = rack::createModel<MuffModule<muff::EffectId::Phaser>, MuffWidget<muff::EffectId::Phaser>>("Phaser");
