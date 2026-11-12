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
