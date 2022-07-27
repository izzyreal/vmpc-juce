#!/bin/sh

mkdir -p build

cwd=$(pwd)

cmake \
  -B build \
  -G "Ninja" \
  \
  -DCPM_mpc_SOURCE="$cwd/editables/mpc" \
  -DCPM_akaifat_SOURCE="$cwd/editables/akaifat" \
  -DCPM_moduru_SOURCE="$cwd/editables/moduru" \
  -DCPM_ctoot_SOURCE="$cwd/editables/ctoot" \
  -DCPM_juce-raw-keyboard-input-module_SOURCE="$cwd/editables/juce-raw-keyboard-input-module" \
  \
  -DCPM_JUCE_SOURCE="$cwd/deps/JUCE" \
  -DCPM_rapidjson_SOURCE="$cwd/deps/rapidjson/include" \
  -DCPM_Catch2_SOURCE="$cwd/deps/Catch2" \
  -DCPM_filesystem_SOURCE="$cwd/deps/filesystem" \
  \
  -Dexpected_SOURCE_DIR="$cwd/deps/expected" \
  \
  -DCMAKE_BUILD_TYPE="Release"

ninja -C build vmpc2000xl_All