# mufferfluffer — top-level build (plan step S-001 completes this file; the parts below are frozen build wiring).
# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 3.22)
project(mufferfluffer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)          # D-002
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(MUFF_BUILD_TESTS "Build the frozen core and adaptor tests" ON)
option(MUFF_BUILD_VST3 "Build the seven VST3 plug-ins (needs JUCE)" OFF)
# Directory holding the implementation of muff/core.hpp, muff/blocks.hpp and muff/adapters.hpp.
# Default: the project's own sources. The planning oracle is selected with -DMUFF_IMPL_DIR=research/spikes/oracle.
set(MUFF_IMPL_DIR "" CACHE PATH "Alternative implementation directory (planning oracle); empty = core/src + adapters/src")

if(MUFF_IMPL_DIR)
  file(GLOB MUFF_IMPL_SOURCES CONFIGURE_DEPENDS "${MUFF_IMPL_DIR}/*.cpp")
  add_library(muffcore STATIC ${MUFF_IMPL_SOURCES})
  target_include_directories(muffcore PRIVATE "${MUFF_IMPL_DIR}")
else()
  file(GLOB MUFF_IMPL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/adapters/src/*.cpp")
  add_library(muffcore STATIC ${MUFF_IMPL_SOURCES})
  target_include_directories(muffcore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core/src")
endif()
target_include_directories(muffcore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/core/include" "${CMAKE_CURRENT_SOURCE_DIR}/adapters/include")
set_target_properties(muffcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(muffcore PRIVATE /W4)
else()
  target_compile_options(muffcore PRIVATE -Wall -Wextra)
endif()

if(MUFF_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
if(MUFF_BUILD_VST3)
  add_subdirectory(vst3)
endif()
