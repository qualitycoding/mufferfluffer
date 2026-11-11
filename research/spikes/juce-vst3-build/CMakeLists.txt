# Spike (R6): does a VST3-only JUCE 9.0.5 plug-in configure, build and link in the planning sandbox?
cmake_minimum_required(VERSION 3.22)
project(MuffSpike VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(${JUCE_SOURCE_DIR_OVERRIDE} juce EXCLUDE_FROM_ALL)
juce_add_plugin(MuffSpike
    COMPANY_NAME "Mufferfluffer"
    PLUGIN_MANUFACTURER_CODE Muff
    PLUGIN_CODE Mfs0
    FORMATS VST3
    PRODUCT_NAME "MuffSpike"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(MuffSpike PRIVATE Plugin.cpp)
target_compile_definitions(MuffSpike PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(MuffSpike PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
