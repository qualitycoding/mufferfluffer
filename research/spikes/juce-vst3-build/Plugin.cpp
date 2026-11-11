#include <juce_audio_utils/juce_audio_utils.h>

class SpikeProcessor final : public juce::AudioProcessor
{
public:
    SpikeProcessor()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          state (*this, nullptr, "state", { std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gain", 1 }, "Gain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f) })
    {}
    const juce::String getName() const override { return "MuffSpike"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainInputChannelSet() == l.getMainOutputChannelSet(); }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override { b.applyGain (state.getRawParameterValue ("gain")->load()); }
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& d) override { if (auto x = state.copyState().createXml()) copyXmlToBinary (*x, d); }
    void setStateInformation (const void* p, int n) override { if (auto x = getXmlFromBinary (p, n)) state.replaceState (juce::ValueTree::fromXml (*x)); }
private:
    juce::AudioProcessorValueTreeState state;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpikeProcessor(); }
