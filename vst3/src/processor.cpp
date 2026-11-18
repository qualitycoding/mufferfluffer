// mufferfluffer — VST3 shell: the juce::AudioProcessor of muff/vst3_shell.hpp (plan decision D-016).
// SPDX-License-Identifier: AGPL-3.0-only
//
// One processor wraps one muff::BlockRunner. Threads: the effect is touched only by prepareToPlay and processBlock
// (never concurrently, by JUCE's guarantee); parameter values cross from the host's threads through the parameter
// objects' own atomics; the tail length crosses back through a std::atomic<double>.
#include "editor.hpp"
#include "muff/adapters.hpp"
#include "muff/vst3_shell.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <cmath>
#include <limits>
#include <vector>

namespace muff {
namespace {

// Tail value the host sees for a given core tail (core.hpp: either at most 60 s or exactly 1.0e9f, "for ever").
double hostTailSeconds (float coreTail) noexcept {
    return coreTail >= 1.0e9f ? std::numeric_limits<double>::infinity() : (double) coreTail;
}

bool isSupportedSet (const juce::AudioChannelSet& set) {
    return set == juce::AudioChannelSet::mono() || set == juce::AudioChannelSet::stereo();
}

// Parses a state value strictly: a plain decimal or scientific number, nothing else. Returns false otherwise.
bool parseStateValue (const juce::String& text, float& result) {
    const juce::String t = text.trim();
    if (t.isEmpty() || ! t.containsOnly ("0123456789+-.eE") || ! t.containsAnyOf ("0123456789"))
        return false;
    const double v = t.getDoubleValue();
    if (! std::isfinite (v))
        return false;
    result = (float) juce::jlimit ((double) -std::numeric_limits<float>::max(), (double) std::numeric_limits<float>::max(), v);
    return true;
}

class Processor final : public juce::AudioProcessor {
public:
    explicit Processor (EffectId id);

    const juce::String getName() const override { return info_.name; }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layout) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    double getTailLengthSeconds() const override { return tailSeconds_.load (std::memory_order_relaxed); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    // Exactly one of the two pointers is set; both point at parameters owned by the AudioProcessor.
    struct HostParam {
        juce::AudioParameterFloat* continuous = nullptr;
        juce::AudioParameterChoice* stepped = nullptr;
    };

    float currentValue (int k) const noexcept;       // the parameter's value in its unit; reads one atomic
    void applyStateValue (int k, float value);      // clamps and sets from the host's thread
    juce::String stateTag() const { return juce::String ("muff-") + info_.slug; }

    const EffectInfo& info_;
    BlockRunner runner_;
    std::vector<HostParam> params_;                 // sized in the constructor, read-only afterwards
    std::atomic<double> tailSeconds_ { 0.0 };
    std::atomic<float> meters_[kMaxMeterLamps] {};  // written at the end of every block, read by the editor's timer
    bool prepared_ = false;                         // touched only by prepareToPlay and processBlock
};

Processor::Processor (EffectId id)
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      info_ (effectInfo (id)),
      runner_ (id) {
    params_.resize ((size_t) info_.numParams);
    for (int k = 0; k < info_.numParams; ++k) {
        const ParamSpec& spec = info_.params[k];
        const juce::ParameterID pid { spec.id, 1 };
        if (spec.taper == Taper::Stepped) {
            juce::StringArray names;
            for (int i = 0; i < spec.numSteps; ++i)
                names.add (spec.stepNames[i]);
            auto* p = new juce::AudioParameterChoice (pid, spec.name, names, (int) spec.def);
            params_[(size_t) k].stepped = p;
            addParameter (p);
        } else {
            // The parameter tables are static, so the lambdas may hold a pointer to the specification.
            const ParamSpec* sp = &spec;
            juce::NormalisableRange<float> range (spec.min, spec.max,
                                                  [sp] (float, float, float n) { return fromNormalised (*sp, n); },
                                                  [sp] (float, float, float v) { return toNormalised (*sp, v); });
            auto* p = new juce::AudioParameterFloat (pid, spec.name, range, spec.def,
                                                     juce::AudioParameterFloatAttributes().withLabel (spec.unit));
            params_[(size_t) k].continuous = p;
            addParameter (p);
        }
    }
    tailSeconds_.store (hostTailSeconds (runner_.effect().tailSeconds()), std::memory_order_relaxed);
}

void Processor::prepareToPlay (double sampleRate, int) {
    // An unsupported rate leaves the effect unprepared and the output silent rather than throwing into the host.
    prepared_ = std::isfinite (sampleRate) && sampleRate >= kMinSampleRate && sampleRate <= kMaxSampleRate;
    if (prepared_)
        runner_.prepare (sampleRate);
    setLatencySamples (runner_.effect().latencySamples());
    tailSeconds_.store (hostTailSeconds (runner_.effect().tailSeconds()), std::memory_order_relaxed);
}

bool Processor::isBusesLayoutSupported (const BusesLayout& layout) const {
    if (layout.inputBuses.size() != 1 || layout.outputBuses.size() != 1)
        return false;
    const auto in = layout.getMainInputChannelSet();
    const auto out = layout.getMainOutputChannelSet();
    return isSupportedSet (in) && isSupportedSet (out) && out.size() >= in.size();
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int numFrames = buffer.getNumSamples();
    if (numFrames <= 0)
        return;
    if (! prepared_) {
        buffer.clear();
        return;
    }

    Effect& fx = runner_.effect();
    for (int k = 0; k < info_.numParams; ++k)
        fx.setParam (k, currentValue (k));

    const int numChannels = buffer.getNumChannels();
    const int numIn = juce::jmin (2, getTotalNumInputChannels(), numChannels);
    const int numOut = juce::jmin (2, getTotalNumOutputChannels(), numChannels);
    if (numOut < 1) {
        buffer.clear();
        return;
    }
    // In-place: BlockRunner allows its input and output pointers to alias.
    const float* ins[2] = { numIn > 0 ? buffer.getReadPointer (0) : nullptr, numIn > 1 ? buffer.getReadPointer (1) : nullptr };
    float* outs[2] = { buffer.getWritePointer (0), numOut > 1 ? buffer.getWritePointer (1) : nullptr };
    runner_.process (ins, numIn, outs, numOut, numFrames);

    for (int ch = numOut; ch < numChannels; ++ch)
        buffer.clear (ch, 0, numFrames);

    tailSeconds_.store (hostTailSeconds (fx.tailSeconds()), std::memory_order_relaxed);
    for (int m = 0; m < info_.numMeters; ++m)
        meters_[m].store (fx.getMeter (m), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* Processor::createEditor() {
    return new Editor (*this, info_, meters_);
}

void Processor::getStateInformation (juce::MemoryBlock& destData) {
    juce::XmlElement xml (stateTag());
    xml.setAttribute ("apiVersion", "1");
    for (int k = 0; k < info_.numParams; ++k) {
        auto* child = xml.createNewChildElement ("param");
        child->setAttribute ("id", info_.params[k].id);
        child->setAttribute ("value", (double) currentValue (k));   // JUCE writes enough digits to round-trip
    }
    copyXmlToBinary (xml, destData);
}

void Processor::setStateInformation (const void* data, int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0)
        return;
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || xml->getTagName() != stateTag())
        return;
    for (auto* child : xml->getChildWithTagNameIterator ("param")) {
        const int k = findParam (info_, child->getStringAttribute ("id").toRawUTF8());
        float value = 0.0f;
        if (k >= 0 && parseStateValue (child->getStringAttribute ("value"), value))
            applyStateValue (k, value);
    }
}

float Processor::currentValue (int k) const noexcept {
    const HostParam& p = params_[(size_t) k];
    return p.stepped != nullptr ? (float) p.stepped->getIndex() : p.continuous->get();
}

void Processor::applyStateValue (int k, float value) {
    const ParamSpec& spec = info_.params[k];
    const HostParam& p = params_[(size_t) k];
    const float clamped = juce::jlimit (spec.min, spec.max, value);
    if (p.stepped != nullptr)
        p.stepped->setValueNotifyingHost (p.stepped->convertTo0to1 (std::round (clamped)));
    else
        p.continuous->setValueNotifyingHost (p.continuous->convertTo0to1 (clamped));
}

} // namespace

std::unique_ptr<juce::AudioProcessor> createProcessor (EffectId id) {
    (void) effectInfo (id);     // throws std::out_of_range for an invalid id, before anything is built
    return std::make_unique<Processor> (id);
}

} // namespace muff
