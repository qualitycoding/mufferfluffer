// ORACLE implementation of muff/vst3_shell.hpp (planning spike, R6): proves the frozen shell tests T-851..T-856
// are satisfiable against JUCE 9.0.5. Not the deliverable (no custom editor). SPDX-License-Identifier: AGPL-3.0-only
#include "muff/adapters.hpp"
#include "muff/vst3_shell.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include <limits>

namespace muff {
namespace {
class Processor final : public juce::AudioProcessor {
public:
    explicit Processor (EffectId id)
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          info_ (effectInfo (id)), runner_ (id) {
        for (int k = 0; k < info_.numParams; ++k) {
            const ParamSpec& s = info_.params[k];
            if (s.taper == Taper::Stepped) {
                juce::StringArray names; for (int i = 0; i < s.numSteps; ++i) names.add (s.stepNames[i]);
                addParameter (new juce::AudioParameterChoice (juce::ParameterID { s.id, 1 }, s.name, names, (int) s.def));
            } else {
                const ParamSpec* sp = &s;
                juce::NormalisableRange<float> range (s.min, s.max,
                    [sp] (float, float, float n) { return fromNormalised (*sp, n); },
                    [sp] (float, float, float v) { return toNormalised (*sp, v); });
                addParameter (new juce::AudioParameterFloat (juce::ParameterID { s.id, 1 }, s.name, range, s.def, juce::AudioParameterFloatAttributes().withLabel (s.unit)));
            }
        }
    }
    const juce::String getName() const override { return info_.name; }
    void prepareToPlay (double sampleRate, int) override { runner_.prepare (sampleRate); setLatencySamples (runner_.effect().latencySamples()); prepared_ = true; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override {
        const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
        const bool inOk = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
        const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
        return inOk && outOk && out.size() >= in.size();
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;
        const int n = buffer.getNumSamples();
        if (n <= 0 || ! prepared_) return;
        pushParameters();
        const int numIn = juce::jmin (2, getTotalNumInputChannels()), numOut = juce::jmin (2, getTotalNumOutputChannels());
        const float* ins[2] = { buffer.getReadPointer (0), numIn > 1 ? buffer.getReadPointer (1) : nullptr };
        float* outs[2] = { buffer.getWritePointer (0), numOut > 1 ? buffer.getWritePointer (1) : nullptr };
        runner_.process (ins, numIn, outs, numOut, n);
    }
    double getTailLengthSeconds() const override {
        const float t = const_cast<Processor*> (this)->runner_.effect().tailSeconds();
        return t >= 1.0e9f ? std::numeric_limits<double>::infinity() : (double) t;
    }
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& dest) override {
        juce::XmlElement xml (juce::String ("muff-") + info_.slug);
        xml.setAttribute ("apiVersion", "1");
        for (int k = 0; k < info_.numParams; ++k) {
            auto* c = xml.createNewChildElement ("param");
            c->setAttribute ("id", info_.params[k].id);
            c->setAttribute ("value", juce::String (value (k), 9));
        }
        copyXmlToBinary (xml, dest);
    }
    void setStateInformation (const void* data, int size) override {
        if (data == nullptr || size <= 0) return;
        const auto xml = getXmlFromBinary (data, size);
        if (xml == nullptr || xml->getTagName() != juce::String ("muff-") + info_.slug) return;
        for (auto* c : xml->getChildWithTagNameIterator ("param")) {
            const int k = findParam (info_, c->getStringAttribute ("id").toRawUTF8());
            const juce::String text = c->getStringAttribute ("value");
            if (k < 0 || ! text.containsOnly ("0123456789+-.eE") || text.isEmpty()) continue;
            const float v = (float) text.getDoubleValue();
            if (! std::isfinite (v)) continue;
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (getParameters()[k]);
            const ParamSpec& s = info_.params[k];
            const float clamped = juce::jlimit (s.min, s.max, v);
            p->setValueNotifyingHost (s.taper == Taper::Stepped ? (s.numSteps > 1 ? std::round (clamped) / (float) (s.numSteps - 1) : 0.0f) : toNormalised (s, clamped));
        }
    }
private:
    float value (int k) const {
        const ParamSpec& s = info_.params[k];
        if (s.taper == Taper::Stepped) return (float) static_cast<juce::AudioParameterChoice*> (getParameters()[k])->getIndex();
        return static_cast<juce::AudioParameterFloat*> (getParameters()[k])->get();
    }
    void pushParameters() { for (int k = 0; k < info_.numParams; ++k) runner_.effect().setParam (k, value (k)); }
    const EffectInfo& info_;
    BlockRunner runner_;
    bool prepared_ = false;
};
}
std::unique_ptr<juce::AudioProcessor> createProcessor (EffectId id) {
    (void) effectInfo (id);     // throws std::out_of_range for an invalid id
    return std::make_unique<Processor> (id);
}
}
