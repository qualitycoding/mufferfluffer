// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

rack::plugin::Model* modelFilterBank = rack::createModel<MuffModule<muff::EffectId::FilterBank>, MuffWidget<muff::EffectId::FilterBank>>("FilterBank");
