#!/bin/sh

mkdir -p build

cwd=$(pwd)

cmake \
  -B build \
  -G "Xcode" \
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
  -Dexpected_INCLUDE_DIR="$cwd/deps/expected/include" \
  \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=9.3 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64COMBINED \
  -DENABLE_ARC=0
