// FROZEN - DO NOT MODIFY. VST3 shell tests (T-851 .. T-856): the contract in vst3/include/muff/vst3_shell.hpp.
#include "muff/adapters.hpp"
#include "muff/vst3_shell.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

using namespace muff;
using Catch::Approx;

namespace {
long gAllocCount = 0;
bool gAllocArmed = false;
const EffectId kAll[7] = { EffectId::Lowpass, EffectId::RingMod, EffectId::Phaser, EffectId::Delay, EffectId::FilterBank, EffectId::OscBox, EffectId::Chorus };

struct Rng { std::uint32_t s; float uni() { s = s * 1664525u + 1013904223u; return (float) ((s >> 8) * (1.0 / 16777216.0)); } };

// value in the parameter's unit that the core must receive for a host-normalised value n
float coreValue (const ParamSpec& s, juce::AudioProcessorParameter* p) {
    if (s.taper == Taper::Stepped) return (float) dynamic_cast<juce::AudioParameterChoice*> (p)->getIndex();
    return dynamic_cast<juce::AudioParameterFloat*> (p)->get();
}
void fillTone (juce::AudioBuffer<float>& b, long start, double fs) {
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (ch, i, (float) ((ch == 0 ? 0.4 : 0.2) * std::sin (6.283185307179586 * 330.0 * (double) (start + i) / fs)));
}
}
#ifndef MUFF_NO_ALLOC_HOOK
void* operator new (std::size_t n) { if (gAllocArmed) ++gAllocCount; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void* operator new[] (std::size_t n) { if (gAllocArmed) ++gAllocCount; if (void* p = std::malloc (n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
#endif

int main (int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;
    return Catch::Session().run (argc, argv);
}

TEST_CASE ("T-851 the host parameters mirror the core tables", "[vst3]") {
    REQUIRE_THROWS_AS (createProcessor ((EffectId) 12), std::out_of_range);
    for (EffectId id : kAll) {
        const EffectInfo& I = effectInfo (id);
        auto proc = createProcessor (id);
        INFO (I.slug);
        REQUIRE (proc->getName() == juce::String (I.name));
        REQUIRE_FALSE (proc->acceptsMidi()); REQUIRE_FALSE (proc->producesMidi());
        REQUIRE (proc->hasEditor());
        REQUIRE (proc->getNumPrograms() == 1);
        REQUIRE (proc->getProgramName (0) == juce::String ("Default"));
        REQUIRE_FALSE (proc->supportsDoublePrecisionProcessing());
        const auto& params = proc->getParameters();
        REQUIRE (params.size() == I.numParams);
        for (int k = 0; k < I.numParams; ++k) {
            const ParamSpec& s = I.params[k];
            INFO (s.id);
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[k]);
            REQUIRE (ranged != nullptr);
            REQUIRE (ranged->getParameterID() == juce::String (s.id));
            REQUIRE (ranged->getVersionHint() == 1);
            REQUIRE (ranged->getName (64) == juce::String (s.name));
            REQUIRE (ranged->isAutomatable());
            if (s.taper == Taper::Stepped) {
                auto* c = dynamic_cast<juce::AudioParameterChoice*> (params[k]);
                REQUIRE (c != nullptr);
                REQUIRE (c->choices.size() == s.numSteps);
                for (int i = 0; i < s.numSteps; ++i) REQUIRE (c->choices[i] == juce::String (s.stepNames[i]));
                REQUIRE (c->getIndex() == (int) s.def);
            } else {
                auto* f = dynamic_cast<juce::AudioParameterFloat*> (params[k]);
                REQUIRE (f != nullptr);
                REQUIRE (f->get() == Approx (s.def).margin (1e-4 * (s.max - s.min)));
                REQUIRE (f->getLabel() == juce::String (s.unit));
                for (float n : { 0.0f, 0.2f, 0.5f, 0.77f, 1.0f }) {
                    REQUIRE (ranged->convertFrom0to1 (n) == Approx (fromNormalised (s, n)).epsilon (1e-4).margin (1e-5));
                    REQUIRE (ranged->convertTo0to1 (fromNormalised (s, n)) == Approx (n).margin (2e-4));
                }
            }
        }
    }
}

TEST_CASE ("T-852 processBlock equals the core driven with the host's parameter values", "[vst3]") {
    const double fs = 48000.0; const int block = 64;
    for (EffectId id : kAll) {
        const EffectInfo& I = effectInfo (id);
        INFO (I.slug);
        auto proc = createProcessor (id);
        juce::AudioProcessor::BusesLayout stereo; stereo.inputBuses.add (juce::AudioChannelSet::stereo()); stereo.outputBuses.add (juce::AudioChannelSet::stereo());
        REQUIRE (proc->setBusesLayout (stereo));
        proc->setRateAndBufferSizeDetails (fs, block);
        proc->prepareToPlay (fs, block);
        BlockRunner ref (id); ref.prepare (fs);
        Rng rng { 1234u + (std::uint32_t) id };
        const auto& params = proc->getParameters();
        juce::AudioBuffer<float> buf (2, block), want (2, block);
        juce::MidiBuffer midi;
        double worst = 0.0;
        for (int b = 0; b < 150; ++b) {
            if (b % 50 == 0) {      // new parameter values at a block boundary
                for (int k = 0; k < I.numParams; ++k) {
                    if (std::string (I.params[k].id) == "active") continue;
                    params[k]->setValueNotifyingHost (rng.uni());
                }
                for (int k = 0; k < I.numParams; ++k) ref.effect().setParam (k, coreValue (I.params[k], params[k]));
            }
            fillTone (buf, (long) b * block, fs); want.makeCopyOf (buf);
            proc->processBlock (buf, midi);
            const float* ins[2] = { want.getReadPointer (0), want.getReadPointer (1) }; float* outs[2] = { want.getWritePointer (0), want.getWritePointer (1) };
            ref.process (ins, 2, outs, 2, block);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) worst = std::max (worst, (double) std::fabs (buf.getSample (ch, i) - want.getSample (ch, i)));
        }
        REQUIRE (worst <= 1.0e-6);
        REQUIRE (proc->getLatencySamples() == ref.effect().latencySamples());
        const float tail = ref.effect().tailSeconds();
        if (tail >= 1.0e9f) REQUIRE (std::isinf (proc->getTailLengthSeconds()));
        else REQUIRE (proc->getTailLengthSeconds() == Approx (tail).epsilon (1e-4));
        juce::AudioBuffer<float> empty (2, 0);
        proc->processBlock (empty, midi);       // a zero-length block is a no-op
        proc->prepareToPlay (96000.0, 256);     // repeated prepare is allowed
        proc->releaseResources();
    }
}

TEST_CASE ("T-853 state round-trips and hostile state is ignored", "[vst3][security]") {
    for (EffectId id : kAll) {
        const EffectInfo& I = effectInfo (id);
        INFO (I.slug);
        auto a = createProcessor (id), b = createProcessor (id);
        Rng rng { 99u + (std::uint32_t) id };
        for (auto* p : a->getParameters()) p->setValueNotifyingHost (rng.uni());
        juce::MemoryBlock state;
        a->getStateInformation (state);
        REQUIRE (state.getSize() > 0);
        b->setStateInformation (state.getData(), (int) state.getSize());
        for (int k = 0; k < I.numParams; ++k) REQUIRE (coreValue (I.params[k], b->getParameters()[k]) == Approx (coreValue (I.params[k], a->getParameters()[k])).margin (1e-5 * (I.params[k].max - I.params[k].min)));
        // the stored element has the documented shape
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        REQUIRE (xml != nullptr);
        REQUIRE (xml->getTagName() == juce::String ("muff-") + I.slug);
        REQUIRE (xml->getStringAttribute ("apiVersion") == "1");
        REQUIRE (xml->getNumChildElements() == I.numParams);
        // hostile input: nothing changes, nothing throws
        std::vector<float> before; for (int k = 0; k < I.numParams; ++k) before.push_back (coreValue (I.params[k], b->getParameters()[k]));
        auto unchanged = [&] () { for (int k = 0; k < I.numParams; ++k) if (coreValue (I.params[k], b->getParameters()[k]) != before[(size_t) k]) return false; return true; };
        b->setStateInformation (nullptr, 0); REQUIRE (unchanged());
        std::vector<char> junk (4096); for (char& c : junk) c = (char) (rng.uni() * 255.0f);
        b->setStateInformation (junk.data(), (int) junk.size()); REQUIRE (unchanged());
        b->setStateInformation (state.getData(), (int) state.getSize() / 2); REQUIRE (unchanged());      // truncated
        juce::XmlElement other ("muff-somethingelse"); other.setAttribute ("apiVersion", "1");
        for (int k = 0; k < I.numParams; ++k) { auto* c = other.createNewChildElement ("param"); c->setAttribute ("id", I.params[k].id); c->setAttribute ("value", (double) I.params[k].max); }
        juce::MemoryBlock foreign; juce::AudioProcessor::copyXmlToBinary (other, foreign);
        b->setStateInformation (foreign.getData(), (int) foreign.getSize()); REQUIRE (unchanged());      // wrong tag
        // right tag: out-of-range values are clamped, unknown ids ignored, missing ids keep their value
        juce::XmlElement mine (juce::String ("muff-") + I.slug); mine.setAttribute ("apiVersion", "1");
        { auto* c = mine.createNewChildElement ("param"); c->setAttribute ("id", I.params[0].id); c->setAttribute ("value", 1.0e9); }
        { auto* c = mine.createNewChildElement ("param"); c->setAttribute ("id", "no_such_parameter"); c->setAttribute ("value", 3.0); }
        { auto* c = mine.createNewChildElement ("param"); c->setAttribute ("id", I.params[1].id); c->setAttribute ("value", "not a number"); }
        juce::MemoryBlock partial; juce::AudioProcessor::copyXmlToBinary (mine, partial);
        b->setStateInformation (partial.getData(), (int) partial.getSize());
        REQUIRE (coreValue (I.params[0], b->getParameters()[0]) == I.params[0].max);
        for (int k = 2; k < I.numParams; ++k) REQUIRE (coreValue (I.params[k], b->getParameters()[k]) == before[(size_t) k]);
        REQUIRE (std::isfinite (coreValue (I.params[1], b->getParameters()[1])));
    }
}

TEST_CASE ("T-854 bus layouts: mono or stereo in, never fewer outputs than inputs", "[vst3]") {
    using Set = juce::AudioChannelSet;
    auto layout = [] (const Set& in, const Set& out) { juce::AudioProcessor::BusesLayout l; l.inputBuses.add (in); l.outputBuses.add (out); return l; };
    for (EffectId id : kAll) {
        auto proc = createProcessor (id);
        REQUIRE (proc->getBusCount (true) == 1); REQUIRE (proc->getBusCount (false) == 1);
        REQUIRE (proc->getBusesLayout().getMainInputChannelSet() == Set::stereo());      // default layout
        REQUIRE (proc->getBusesLayout().getMainOutputChannelSet() == Set::stereo());
        REQUIRE (proc->getBypassParameter() == nullptr);
        REQUIRE (proc->checkBusesLayoutSupported (layout (Set::mono(), Set::mono())));
        REQUIRE (proc->checkBusesLayoutSupported (layout (Set::mono(), Set::stereo())));
        REQUIRE (proc->checkBusesLayoutSupported (layout (Set::stereo(), Set::stereo())));
        REQUIRE_FALSE (proc->checkBusesLayoutSupported (layout (Set::stereo(), Set::mono())));
        REQUIRE_FALSE (proc->checkBusesLayoutSupported (layout (Set::create5point1(), Set::create5point1())));
        REQUIRE_FALSE (proc->checkBusesLayoutSupported (layout (Set::disabled(), Set::stereo())));
        // mono in, stereo out: channel 0 holds the input, both channels are written
        REQUIRE (proc->setBusesLayout (layout (Set::mono(), Set::stereo())));
        proc->setRateAndBufferSizeDetails (48000.0, 64); proc->prepareToPlay (48000.0, 64);
        BlockRunner ref (id); ref.prepare (48000.0);
        juce::AudioBuffer<float> buf (2, 64); juce::MidiBuffer midi; std::vector<float> in (64), L (64), R (64);
        double worst = 0.0;
        for (int b = 0; b < 40; ++b) {
            for (int i = 0; i < 64; ++i) { in[(size_t) i] = (float) (0.3 * std::sin (6.283185307179586 * 500.0 * (b * 64 + i) / 48000.0)); buf.setSample (0, i, in[(size_t) i]); buf.setSample (1, i, 99.0f); }
            proc->processBlock (buf, midi);
            const float* ins[1] = { in.data() }; float* outs[2] = { L.data(), R.data() };
            ref.process (ins, 1, outs, 2, 64);
            for (int i = 0; i < 64; ++i) worst = std::max ({ worst, (double) std::fabs (buf.getSample (0, i) - L[(size_t) i]), (double) std::fabs (buf.getSample (1, i) - R[(size_t) i]) });
        }
        INFO (effectInfo (id).slug);
        REQUIRE (worst <= 1.0e-6);
    }
}

TEST_CASE ("T-857 a mono track hears what a mono host is promised (the filter bank keeps all eight filters)", "[vst3]") {
    using Set = juce::AudioChannelSet;
    for (EffectId id : kAll) {
        auto proc = createProcessor (id);
        juce::AudioProcessor::BusesLayout l; l.inputBuses.add (Set::mono()); l.outputBuses.add (Set::mono());
        REQUIRE (proc->setBusesLayout (l));
        proc->setRateAndBufferSizeDetails (48000.0, 64); proc->prepareToPlay (48000.0, 64);
        BlockRunner ref (id); ref.prepare (48000.0);
        juce::AudioBuffer<float> buf (1, 64); juce::MidiBuffer midi; std::vector<float> in (64), M (64);
        double worst = 0.0, energy = 0.0;
        for (int b = 0; b < 40; ++b) {
            for (int i = 0; i < 64; ++i) { in[(size_t) i] = (float) (0.3 * std::sin (6.283185307179586 * 300.0 * (b * 64 + i) / 48000.0)); buf.setSample (0, i, in[(size_t) i]); }
            proc->processBlock (buf, midi);
            const float* ins[1] = { in.data() }; float* outs[1] = { M.data() };
            ref.process (ins, 1, outs, 1, 64);
            for (int i = 0; i < 64; ++i) { worst = std::max (worst, (double) std::fabs (buf.getSample (0, i) - M[(size_t) i])); energy += (double) M[(size_t) i] * M[(size_t) i]; }
        }
        INFO (effectInfo (id).slug);
        REQUIRE (worst <= 1.0e-6);
        REQUIRE (energy > 1.0e-3);
    }
}

#ifndef MUFF_NO_ALLOC_HOOK
TEST_CASE ("T-855 processBlock performs no heap allocation", "[vst3][realtime]") {
    for (EffectId id : kAll) {
        auto proc = createProcessor (id);
        proc->setRateAndBufferSizeDetails (48000.0, 256); proc->prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi;
        Rng rng { 5u + (std::uint32_t) id };
        proc->processBlock (buf, midi);         // first block may touch lazily initialised state
        long allocs = 0;
        for (int b = 0; b < 200; ++b) {
            if (b % 20 == 0) for (auto* p : proc->getParameters()) p->setValueNotifyingHost (rng.uni());    // host thread: may allocate
            fillTone (buf, (long) b * 256, 48000.0);
            gAllocCount = 0; gAllocArmed = true;
            proc->processBlock (buf, midi);
            gAllocArmed = false;
            allocs += gAllocCount;
        }
        INFO (effectInfo (id).slug);
        REQUIRE (allocs == 0);
    }
}
#endif

TEST_CASE ("T-856 output stays finite for hostile audio input", "[vst3][security]") {
    for (EffectId id : kAll) {
        auto proc = createProcessor (id);
        proc->setRateAndBufferSizeDetails (44100.0, 128); proc->prepareToPlay (44100.0, 128);
        juce::AudioBuffer<float> buf (2, 128); juce::MidiBuffer midi;
        bool ok = true;
        for (int b = 0; b < 100; ++b) {
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 128; ++i) {
                float v = (float) std::sin (0.1 * (b * 128 + i));
                if ((b * 128 + i) % 37 == 0) v = std::numeric_limits<float>::quiet_NaN();
                if ((b * 128 + i) % 53 == 0) v = std::numeric_limits<float>::infinity();
                if ((b * 128 + i) % 61 == 0) v = -1.0e30f;
                buf.setSample (ch, i, v);
            }
            proc->processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 128; ++i) ok = ok && std::isfinite (buf.getSample (ch, i)) && std::fabs (buf.getSample (ch, i)) <= kOutputLimit;
        }
        INFO (effectInfo (id).slug);
        REQUIRE (ok);
    }
}
