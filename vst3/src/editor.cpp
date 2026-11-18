// mufferfluffer — VST3 editor implementation (plan decision D-016).
// SPDX-License-Identifier: AGPL-3.0-only
#include "editor.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace muff {
namespace {

constexpr int kColumnWidth = 120;           // D-016: pixels per control column
constexpr int kRowHeight = 140;             // D-016: pixels per row
constexpr int kHeaderHeight = 40;           // D-016: pixels for the title and the lamps
constexpr int kMaxColumns = 6;              // D-016: at most six controls in a row
constexpr int kMinColumns = 4;              // keeps the title and the lamps inside narrow windows
constexpr int kCaptionHeight = 20;
constexpr int kLampSlotWidth = 72;
constexpr int kLampDiameter = 12;
constexpr int kTimerHz = 30;                // D-016: the lamps are refreshed at 30 Hz
constexpr float kLampRepaintStep = 0.02f;   // a smaller change in a meter is not worth a repaint
constexpr const char* kTapId = "tap";

const juce::Colour kBackground { 0xff2e3035 };
const juce::Colour kInk { 0xffe9e6dc };
const juce::Colour kLampOn { 0xff7fd66b };
const juce::Colour kLampOff { 0xff3c4a39 };

juce::RangedAudioParameter& parameterAt (juce::AudioProcessor& processor, int index) {
    return *dynamic_cast<juce::RangedAudioParameter*> (processor.getParameters()[index]);
}

} // namespace

// A cell of the grid. The attachments are declared after the widgets, so they are destroyed first.
struct Editor::Control {
    juce::Label caption;
    juce::Slider slider;
    juce::ComboBox choice;
    juce::TextButton tap;
    std::unique_ptr<juce::SliderParameterAttachment> sliderAttachment;
    std::unique_ptr<juce::ComboBoxParameterAttachment> choiceAttachment;
    std::unique_ptr<juce::ParameterAttachment> tapAttachment;
    bool tapDown = false;
};

Editor::Editor (juce::AudioProcessor& owner, const EffectInfo& info, const std::atomic<float>* meters)
    : juce::AudioProcessorEditor (owner), info_ (info), meters_ (meters) {
    for (int k = 0; k < info_.numParams; ++k)
        addControl (k);
    const int columns = std::max (kMinColumns, std::min (kMaxColumns, info_.numParams));
    const int rows = (info_.numParams + kMaxColumns - 1) / kMaxColumns;
    setSize (columns * kColumnWidth, kHeaderHeight + rows * kRowHeight);
    startTimerHz (kTimerHz);
}

Editor::~Editor() { stopTimer(); }

void Editor::addControl (int paramIndex) {
    const ParamSpec& spec = info_.params[paramIndex];
    juce::RangedAudioParameter& parameter = parameterAt (*getAudioProcessor(), paramIndex);
    auto control = std::make_unique<Control>();

    control->caption.setText (spec.name, juce::dontSendNotification);
    control->caption.setJustificationType (juce::Justification::centred);
    control->caption.setColour (juce::Label::textColourId, kInk);
    addAndMakeVisible (control->caption);

    if (std::string (spec.id) == kTapId) {
        control->tap.setButtonText ("Tap");
        Control* raw = control.get();
        control->tapAttachment = std::make_unique<juce::ParameterAttachment> (
            parameter, [raw] (float value) { raw->tap.setToggleState (value > 0.5f, juce::dontSendNotification); });
        control->tap.onStateChange = [raw] {
            const bool down = raw->tap.isDown();
            if (down == raw->tapDown)
                return;
            raw->tapDown = down;
            raw->tapAttachment->setValueAsCompleteGesture (down ? 1.0f : 0.0f);     // momentary: 1 while pressed
        };
        control->tapAttachment->sendInitialUpdate();
        addAndMakeVisible (control->tap);
    } else if (spec.taper == Taper::Stepped) {
        for (int i = 0; i < spec.numSteps; ++i)
            control->choice.addItem (spec.stepNames[i], i + 1);
        control->choiceAttachment = std::make_unique<juce::ComboBoxParameterAttachment> (parameter, control->choice);
        control->choiceAttachment->sendInitialUpdate();
        addAndMakeVisible (control->choice);
    } else {
        control->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        control->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kColumnWidth - 20, 18);
        control->sliderAttachment = std::make_unique<juce::SliderParameterAttachment> (parameter, control->slider);
        addAndMakeVisible (control->slider);
    }
    controls_.push_back (std::move (control));
}

void Editor::paint (juce::Graphics& g) {
    g.fillAll (kBackground);
    g.setColour (kInk);
    g.setFont (20.0f);
    g.drawText (info_.name, 12, 0, getWidth() / 2, kHeaderHeight, juce::Justification::centredLeft);
    g.setFont (11.0f);
    const juce::Rectangle<int> area = lampArea();
    for (int m = 0; m < info_.numMeters; ++m) {
        const int x = area.getX() + m * kLampSlotWidth;
        const float level = std::min (1.0f, std::max (0.0f, shown_[m]));
        g.setColour (kLampOff.interpolatedWith (kLampOn, level));
        g.fillEllipse ((float) x, (float) (area.getCentreY() - kLampDiameter / 2), (float) kLampDiameter, (float) kLampDiameter);
        g.setColour (kInk);
        g.drawText (info_.meters[m].name, x + kLampDiameter + 4, area.getY(), kLampSlotWidth - kLampDiameter - 4, area.getHeight(),
                    juce::Justification::centredLeft);
    }
}

juce::Rectangle<int> Editor::lampArea() const {
    const int width = info_.numMeters * kLampSlotWidth;
    return { getWidth() - width - 8, 0, width, kHeaderHeight };
}

void Editor::resized() {
    for (size_t i = 0; i < controls_.size(); ++i) {
        const int column = (int) i % kMaxColumns;
        const int row = (int) i / kMaxColumns;
        juce::Rectangle<int> cell (column * kColumnWidth, kHeaderHeight + row * kRowHeight, kColumnWidth, kRowHeight);
        Control& control = *controls_[i];
        control.caption.setBounds (cell.removeFromTop (kCaptionHeight));
        if (control.tapAttachment != nullptr)
            control.tap.setBounds (cell.withSizeKeepingCentre (80, 40));
        else if (control.choiceAttachment != nullptr)
            control.choice.setBounds (cell.withSizeKeepingCentre (kColumnWidth - 20, 28));
        else
            control.slider.setBounds (cell.reduced (6));
    }
}

void Editor::timerCallback() {
    bool changed = false;
    for (int m = 0; m < info_.numMeters; ++m) {
        const float now = meters_[m].load (std::memory_order_relaxed);
        if (std::abs (now - shown_[m]) >= kLampRepaintStep) {
            shown_[m] = now;
            changed = true;
        }
    }
    if (changed)
        repaint (lampArea());
}

} // namespace muff
