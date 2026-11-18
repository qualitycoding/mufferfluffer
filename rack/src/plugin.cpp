// Mufferfluffer VCV Rack plug-in: entry point called by Rack when it loads the library.
// SPDX-License-Identifier: AGPL-3.0-only
#include "plugin.hpp"

rack::plugin::Plugin* pluginInstance = nullptr;

void init (rack::plugin::Plugin* p) {
    pluginInstance = p;
    // The order follows muff::EffectId and the module list of plugin.json.
    p->addModel (modelLowpass);
    p->addModel (modelRingMod);
    p->addModel (modelPhaser);
    p->addModel (modelDelay);
    p->addModel (modelFilterBank);
    p->addModel (modelOscBox);
    p->addModel (modelChorus);
}
