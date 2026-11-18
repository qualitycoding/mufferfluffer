// Mufferfluffer VCV Rack plug-in: one module and one panel widget per effect, as thin shells over muff::RackBridge
// (D-006, D-013). The work is done by the three functions declared here, defined in module_shell.cpp.
// SPDX-License-Identifier: AGPL-3.0-only
#pragma once

#include "plugin.hpp"

#include "muff/adapters.hpp"
#include "muff/core.hpp"

namespace muffrack {

// Declares the effect's parameters, ports, lights and bypass route on a freshly constructed module.
void configureModule (rack::engine::Module& module, muff::EffectId id);

// One sample frame: copies the parameters into the effect, runs the bridge on the port voltages, then writes the
// outputs and the meter lights.
void processFrame (rack::engine::Module& module, muff::RackBridge& bridge, muff::RackIO& io);

// Gives a module widget its panel and places the controls, ports and lights of the generated layout.
// `module` is null when the widget is drawn in the module browser.
void populateWidget (rack::app::ModuleWidget& widget, rack::engine::Module* module, muff::EffectId id);

} // namespace muffrack

template <muff::EffectId ID>
struct MuffModule : rack::engine::Module {
    MuffModule() { muffrack::configureModule (*this, ID); }

    void process (const ProcessArgs&) override { muffrack::processFrame (*this, bridge_, io_); }

    // Rack also sends this event when the module is added to the engine, so the effect is prepared before it runs.
    void onSampleRateChange (const SampleRateChangeEvent& e) override { bridge_.prepare (e.sampleRate); }

private:
    muff::RackBridge bridge_ { ID };
    muff::RackIO io_ {};
};

template <muff::EffectId ID>
struct MuffWidget : rack::app::ModuleWidget {
    explicit MuffWidget (MuffModule<ID>* module) { muffrack::populateWidget (*this, module, ID); }
};
