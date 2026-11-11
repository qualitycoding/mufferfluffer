// mufferfluffer — VST3 shell: one juce::AudioProcessor per effect (frozen by D-016).
// SPDX-License-Identifier: AGPL-3.0-only
//
// Contract of the processor returned by createProcessor(id), with I = effectInfo(id):
//  * getName() == I.name. No MIDI in or out. One program. hasEditor() == true. Single precision only.
//  * Buses: one input bus and one output bus. Supported layouts: mono->mono, mono->stereo, stereo->stereo.
//    Every other layout is rejected by isBusesLayoutSupported().
//  * Parameters: exactly I.numParams, in table order. Parameter k has ID I.params[k].id (version hint 1) and name
//    I.params[k].name.  Linear and Log tapers are juce::AudioParameterFloat whose range maps a normalised value n to
//    muff::fromNormalised(spec, n) and back with muff::toNormalised; default spec.def; label spec.unit.
//    Stepped tapers are juce::AudioParameterChoice with the step names and default index (int) spec.def.
//  * processBlock: copies every parameter's current value into the effect (Effect::setParam) before the first sample
//    of the block, then runs muff::BlockRunner over the block. It never allocates. A block of 0 samples is a no-op.
//  * prepareToPlay(sampleRate, blockSize) may be called repeatedly; it calls BlockRunner::prepare and
//    setLatencySamples(effect.latencySamples()).
//  * getTailLengthSeconds(): effect.tailSeconds(), or +infinity when that is >= 1.0e9.
//  * State: getStateInformation writes juce::AudioProcessor::copyXmlToBinary of an XML element whose tag is
//    "muff-" + I.slug with an attribute apiVersion="1" and one child <param id="..." value="..."/> per parameter
//    (value = the parameter's value in its unit, full float precision).
//    setStateInformation accepts only such an element with the same tag: known ids are applied (clamped by the
//    parameter), unknown ids and missing ids are ignored. Anything else (wrong tag, not XML, empty, truncated)
//    leaves every parameter unchanged and must not throw.
#pragma once

#include "muff/core.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace muff {

// Throws std::out_of_range for an invalid id.
std::unique_ptr<juce::AudioProcessor> createProcessor (EffectId id);

} // namespace muff
