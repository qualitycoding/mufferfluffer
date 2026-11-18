// Mufferfluffer VCV Rack plug-in: the plug-in instance and the seven models it registers (D-006).
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include <rack.hpp>

extern rack::plugin::Plugin* pluginInstance;

extern rack::plugin::Model* modelLowpass;
extern rack::plugin::Model* modelRingMod;
extern rack::plugin::Model* modelPhaser;
extern rack::plugin::Model* modelDelay;
extern rack::plugin::Model* modelFilterBank;
extern rack::plugin::Model* modelOscBox;
extern rack::plugin::Model* modelChorus;
