// mufferfluffer — VST3 editor: this project's own generic layout (plan decision D-016). Declarations only.
// SPDX-License-Identifier: AGPL-3.0-only
//
// One rotary slider per continuous parameter, one combo box per stepped parameter, a momentary button for "tap", and
// one lamp per meter, in rows of at most six controls. The editor never touches the muff::Effect: it reads the meter
// values that the processor publishes in std::atomic<float> members at the end of every block.
#pragma once

#include "muff/core.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <memory>
#include <vector>

namespace muff {

inline constexpr int kMaxMeterLamps = 3;    // the tables hold at most three meters per effect

class Editor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    // meters points at info.numMeters atomics owned by the processor, which outlives its editor.
    Editor (juce::AudioProcessor& owner, const EffectInfo& info, const std::atomic<float>* meters);
    ~Editor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct Control;     // one cell of the grid: caption, widget and the attachment that connects it to a parameter

    void timerCallback() override;
    void addControl (int paramIndex);
    juce::Rectangle<int> lampArea() const;

    const EffectInfo& info_;
    const std::atomic<float>* meters_;
    float shown_[kMaxMeterLamps] {};
    std::vector<std::unique_ptr<Control>> controls_;
};

} // namespace muff
