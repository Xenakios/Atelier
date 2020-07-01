# CC := clang
# CXX := clang++

# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I./eurorack 
FLAGS += -DTEST
# CFLAGS += -fsanitize=undefined
# CXXFLAGS += -fsanitize=undefined

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
# LDFLAGS += -fuse-ld=lld

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += eurorack/stmlib/utils/random.cc
SOURCES += eurorack/stmlib/dsp/atan.cc
SOURCES += eurorack/stmlib/dsp/units.cc

SOURCES += $(wildcard eurorack/plaits/dsp/*.cc)
SOURCES += $(wildcard eurorack/plaits/dsp/engine/*.cc)
SOURCES += $(wildcard eurorack/plaits/dsp/speech/*.cc)
SOURCES += $(wildcard eurorack/plaits/dsp/physical_modelling/*.cc)
SOURCES += eurorack/plaits/resources.cc

# SOURCES += eurorack/clouds/dsp/pvoc/spectral_clouds_transformation.cc

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

