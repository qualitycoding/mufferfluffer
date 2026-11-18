// Mufferfluffer VCV Rack plug-in: configuration, per-sample processing and panel population shared by the seven
// modules (D-006). Everything is read from muff::effectInfo() and the generated panel layout.
// SPDX-License-Identifier: AGPL-3.0-only
#include "module_shell.hpp"

#include "generated_layout.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace muffrack {
namespace {

using muff::ParamSpec;
using muff::Taper;

// Panel artwork in the order of muff::EffectId; the file names are the module slugs of plugin.json.
constexpr const char* kPanelFiles[muff::kNumEffects] = {
    "res/Lowpass.svg", "res/RingMod.svg", "res/Phaser.svg", "res/Delay.svg",
    "res/FilterBank.svg", "res/OscBox.svg", "res/Chorus.svg",
};

// The component-library control that D-006 assigns to a parameter.
enum class Control { Knob, SmallKnob, SnapKnob, TwoWaySwitch, ThreeWaySwitch, Button };

bool isTap (const ParamSpec& spec) { return std::strcmp (spec.id, "tap") == 0; }

// The filter bank's eight level sliders ("level1" to "level8") are the only parameters whose id starts so.
bool isLevelSlider (const ParamSpec& spec) { return std::strncmp (spec.id, "level", 5) == 0; }

Control controlFor (const ParamSpec& spec) {
    if (spec.taper != Taper::Stepped) return isLevelSlider (spec) ? Control::SmallKnob : Control::Knob;
    if (isTap (spec)) return Control::Button;
    if (spec.numSteps == 2) return Control::TwoWaySwitch;
    if (spec.numSteps == 3) return Control::ThreeWaySwitch;
    return Control::SnapKnob;
}

// A log-taper parameter is stored by Rack as its knob position in [0, 1] and shown in the parameter's unit.
struct LogTaperQuantity : rack::engine::ParamQuantity {
    const ParamSpec* spec = nullptr;
    float getDisplayValue() override { return muff::fromNormalised (*spec, getValue()); }
    void setDisplayValue (float displayValue) override { setValue (muff::toNormalised (*spec, displayValue)); }
};

// Rack prints the unit straight after the number, so a non-empty unit gets a separating space.
std::string unitLabel (const ParamSpec& spec) { return *spec.unit ? std::string (" ") + spec.unit : std::string(); }

void configParameter (rack::engine::Module& module, int index, const ParamSpec& spec) {
    if (spec.taper == Taper::Stepped) {
        if (isTap (spec)) {
            module.configButton (index, spec.name);
        } else {
            const std::vector<std::string> labels (spec.stepNames, spec.stepNames + spec.numSteps);
            module.configSwitch (index, spec.min, spec.max, spec.def, spec.name, labels);
        }
    } else if (spec.taper == Taper::Log) {
        LogTaperQuantity* q = module.configParam<LogTaperQuantity> (index, 0.0f, 1.0f, muff::toNormalised (spec, spec.def),
                                                                    spec.name, unitLabel (spec));
        q->spec = &spec;
    } else {
        module.configParam (index, spec.min, spec.max, spec.def, spec.name, unitLabel (spec));
    }
}

// The value the effect expects for a Rack parameter value (see configParameter).
float effectValue (const ParamSpec& spec, float rackValue) {
    return spec.taper == Taper::Log ? muff::fromNormalised (spec, rackValue) : rackValue;
}

rack::math::Vec toPx (muff::layout::Vec2 mm) { return rack::window::mm2px (rack::math::Vec (mm.x, mm.y)); }

rack::app::ParamWidget* createControl (Control control, rack::math::Vec pos, rack::engine::Module* module, int index) {
    using namespace rack::componentlibrary;
    switch (control) {
        case Control::Knob: return rack::createParamCentered<RoundBlackKnob> (pos, module, index);
        case Control::SmallKnob: return rack::createParamCentered<RoundSmallBlackKnob> (pos, module, index);
        case Control::SnapKnob: return rack::createParamCentered<RoundBlackSnapKnob> (pos, module, index);
        case Control::TwoWaySwitch: return rack::createParamCentered<CKSS> (pos, module, index);
        case Control::ThreeWaySwitch: return rack::createParamCentered<CKSSThree> (pos, module, index);
        case Control::Button: return rack::createParamCentered<VCVButton> (pos, module, index);
    }
    return nullptr; // not reached: the switch covers every Control
}

} // namespace

void configureModule (rack::engine::Module& module, muff::EffectId id) {
    const muff::EffectInfo& info = muff::effectInfo (id);
    module.config (info.numParams, info.numInputs, info.numOutputs, info.numMeters);
    for (int i = 0; i < info.numParams; ++i) configParameter (module, i, info.params[i]);
    for (int i = 0; i < info.numInputs; ++i) module.configInput (i, info.inputs[i].name);
    for (int o = 0; o < info.numOutputs; ++o) module.configOutput (o, info.outputs[o].name);
    for (int k = 0; k < info.numMeters; ++k) module.configLight (k, info.meters[k].name);
    module.configBypass (0, 0); // Rack's own bypass passes the audio input straight to the first output
}

void processFrame (rack::engine::Module& module, muff::RackBridge& bridge, muff::RackIO& io) {
    muff::Effect& fx = bridge.effect();
    const muff::EffectInfo& info = fx.info();
    for (int i = 0; i < info.numParams; ++i) fx.setParam (i, effectValue (info.params[i], module.params[i].getValue()));
    for (int i = 0; i < info.numInputs; ++i) {
        rack::engine::Input& input = module.inputs[i];
        io.inConnected[i] = input.isConnected();
        // Input 0 is the audio input: a polyphonic cable is summed into it. Other inputs read channel 0.
        io.inVolts[i] = i == 0 ? input.getVoltageSum() : input.getVoltage (0);
    }
    for (int o = 0; o < info.numOutputs; ++o) io.outConnected[o] = module.outputs[o].isConnected();
    bridge.process (io);
    for (int o = 0; o < info.numOutputs; ++o) module.outputs[o].setVoltage (io.outVolts[o]);
    for (int k = 0; k < info.numMeters; ++k) module.lights[k].setBrightness (fx.getMeter (k));
}

void populateWidget (rack::app::ModuleWidget& widget, rack::engine::Module* module, muff::EffectId id) {
    using namespace rack::componentlibrary;
    const muff::EffectInfo& info = muff::effectInfo (id);
    const muff::layout::Panel& panel = muff::layout::kPanels[static_cast<int> (id)];
    assert (panel.numParams == info.numParams && panel.numInputs == info.numInputs);
    assert (panel.numOutputs == info.numOutputs && panel.numLights == info.numMeters);

    widget.setModule (module);
    widget.setPanel (APP->window->loadSvg (rack::asset::plugin (pluginInstance, kPanelFiles[static_cast<int> (id)])));
    for (int i = 0; i < info.numParams; ++i)
        widget.addParam (createControl (controlFor (info.params[i]), toPx (panel.params[i]), module, i));
    for (int i = 0; i < info.numInputs; ++i)
        widget.addInput (rack::createInputCentered<PJ301MPort> (toPx (panel.inputs[i]), module, i));
    for (int o = 0; o < info.numOutputs; ++o)
        widget.addOutput (rack::createOutputCentered<PJ301MPort> (toPx (panel.outputs[o]), module, o));
    for (int k = 0; k < info.numMeters; ++k)
        widget.addChild (rack::createLightCentered<MediumLight<GreenLight>> (toPx (panel.lights[k]), module, k));
}

} // namespace muffrack
