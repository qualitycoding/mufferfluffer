// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelOscBox = rack::createModel<MuffModule<muff::EffectId::OscBox>, MuffWidget<muff::EffectId::OscBox>>("OscBox");
