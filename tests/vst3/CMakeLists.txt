# FROZEN - DO NOT MODIFY. Build wiring of the frozen VST3 shell tests (needs the muff_vst3_sources target).
juce_add_console_app(muff_vst3_tests PRODUCT_NAME "muff_vst3_tests")
target_sources(muff_vst3_tests PRIVATE test_vst3.cpp)
target_include_directories(muff_vst3_tests PRIVATE "${CMAKE_SOURCE_DIR}/tests/core")
target_link_libraries(muff_vst3_tests PRIVATE muff_vst3_sources Catch2::Catch2)
add_test(NAME muff_vst3_tests COMMAND muff_vst3_tests)
