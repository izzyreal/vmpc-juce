#!/bin/sh
cwd=$(pwd)
mkdir -p build
cmake \
  -B build \
  -G "Ninja" \
  -DCPM_moduru_SOURCE="$cwd/moduru" \
  -DCPM_ctoot_SOURCE="$cwd/ctoot" \
  -DCPM_mpc_SOURCE="$cwd/mpc" \
  -DCPM_JUCE_SOURCE="$cwd/JUCE" \
  -DCPM_juce-raw-keyboard-input-module_SOURCE="$cwd/juce-raw-keyboard-input-module" \
  -DCMAKE_BUILD_TYPE="Release"
ninja vmpc2000xl_All
ninja -C build vmpc2000xl_Standalone