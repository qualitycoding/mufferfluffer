// mufferfluffer — plug-in entry point of every VST3 target (plan decision D-005).
// SPDX-License-Identifier: AGPL-3.0-only
//
// MUFF_EFFECT_ID is the numeric muff::EffectId of the target, defined by vst3/CMakeLists.txt.
#include "muff/vst3_shell.hpp"

#ifndef MUFF_EFFECT_ID
#error "MUFF_EFFECT_ID must be defined by the build"
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return muff::createProcessor (static_cast<muff::EffectId> (MUFF_EFFECT_ID)).release();
}
