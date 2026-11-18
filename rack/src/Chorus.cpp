// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelChorus = rack::createModel<MuffModule<muff::EffectId::Chorus>, MuffWidget<muff::EffectId::Chorus>>("Chorus");
