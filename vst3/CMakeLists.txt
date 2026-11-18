# mufferfluffer — VST3 build. Plan step S-021 adds the seven juce_add_plugin targets below the marked line.
# SPDX-License-Identifier: AGPL-3.0-only
include(FetchContent)
# JUCE 9.0.5 pinned by commit (plan/ENVIRONMENT.md). Offline: -DFETCHCONTENT_SOURCE_DIR_JUCE=<checkout>.
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG d60b7a78030661574a615b586d9325ba7a7fa7dd)
FetchContent_MakeAvailable(JUCE)

# Sources of the shell, shared by every plug-in target and by the shell tests.
set(MUFF_VST3_IMPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Directory holding the implementation of muff/vst3_shell.hpp")
file(GLOB MUFF_VST3_SOURCES CONFIGURE_DEPENDS "${MUFF_VST3_IMPL_DIR}/*.cpp")
add_library(muff_vst3_sources INTERFACE)
target_sources(muff_vst3_sources INTERFACE ${MUFF_VST3_SOURCES})
target_include_directories(muff_vst3_sources INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(muff_vst3_sources INTERFACE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(muff_vst3_sources INTERFACE muffcore juce::juce_audio_utils juce::juce_recommended_config_flags)

if(MUFF_BUILD_TESTS)
  add_subdirectory("${CMAKE_SOURCE_DIR}/tests/vst3" "${CMAKE_BINARY_DIR}/tests_vst3")
endif()

# ---- S-021: plug-in targets go below this line -------------------------------------------------------------

# One plug-in target per effect (D-005): FORMATS VST3 only; every target compiles the shared shell sources, the editor and
# its own entry file, which creates the processor for MUFF_EFFECT_ID (the numeric muff::EffectId).
function(muff_add_plugin target code product effect_id slug category)
  juce_add_plugin(${target}
    FORMATS VST3
    PRODUCT_NAME "${product}"
    COMPANY_NAME "Mufferfluffer"
    PLUGIN_MANUFACTURER_CODE Muff
    PLUGIN_CODE ${code}
    BUNDLE_ID org.qualitycoding.mufferfluffer.${slug}
    COPY_PLUGIN_AFTER_BUILD FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx ${category})
  target_sources(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/entry/entry.cpp")
  target_compile_definitions(${target} PRIVATE MUFF_EFFECT_ID=${effect_id})
  target_link_libraries(${target} PRIVATE muff_vst3_sources juce::juce_recommended_warning_flags)
endfunction()

muff_add_plugin(MuffLowpass    Mfl1 "Fluff Lowpass" 0 lowpass    Filter)
muff_add_plugin(MuffRingMod    Mfr1 "Fluff Ring"    1 ringmod    Modulation)
muff_add_plugin(MuffPhaser     Mfp1 "Fluff Phase"   2 phaser     Modulation)
muff_add_plugin(MuffDelay      Mfd1 "Fluff Delay"   3 delay      Delay)
muff_add_plugin(MuffFilterBank Mfb1 "Fluff Bank"    4 filterbank Filter)
muff_add_plugin(MuffOscBox     Mfo1 "Fluff Osc"     5 oscbox     Generator)
muff_add_plugin(MuffChorus     Mfc1 "Fluff Swarm"   6 chorus     Modulation)
