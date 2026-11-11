# FROZEN - DO NOT MODIFY. Build wiring of the frozen C++ tests.
include(FetchContent)
# Catch2 v3.16.1 pinned by commit (plan/ENVIRONMENT.md). Offline: -DFETCHCONTENT_SOURCE_DIR_CATCH2=<checkout>.
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG 08092139210881f01a34e60c5f2fd6afa0ea4024)
FetchContent_MakeAvailable(Catch2)

add_executable(muff_core_tests
  core/test_common.cpp
  core/test_m1.cpp
  core/test_m2.cpp
  core/test_m3.cpp
  core/test_adapters.cpp)
target_include_directories(muff_core_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
target_link_libraries(muff_core_tests PRIVATE muffcore Catch2::Catch2WithMain)
option(MUFF_SANITIZER_BUILD "Sanitizer build: leave out the operator-new hook of T-011" OFF)
if(MUFF_SANITIZER_BUILD)
  target_compile_definitions(muff_core_tests PRIVATE MUFF_NO_ALLOC_HOOK=1)
endif()

add_executable(muff_bench perf/bench.cpp)
target_link_libraries(muff_bench PRIVATE muffcore)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(muff_core_tests)
