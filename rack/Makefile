# SPDX-License-Identifier: AGPL-3.0-only
# VCV Rack 2 plug-in build (D-006): the module shells in src/ plus the shared DSP core and host adaptors.
RACK_DIR ?= ../Rack-SDK

FLAGS += -std=c++17 -I../core/include -I../adapters/include

SOURCES += $(wildcard src/*.cpp) $(wildcard ../core/src/*.cpp) $(wildcard ../adapters/src/*.cpp)

# plugin.mk copies DISTRIBUTABLES with their relative paths, so the licence is copied in beside the manifest first.
DISTRIBUTABLES += res LICENSE

LICENSE: ../LICENSE
	cp ../LICENSE LICENSE

include $(RACK_DIR)/plugin.mk

dist: LICENSE
