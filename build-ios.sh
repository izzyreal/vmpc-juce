#!/bin/sh

mkdir -p build

cwd=$(pwd)

cmake \
  -B build \
  -G "Xcode" \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=9.3 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
  -DPLATFORM=OS64COMBINED \
  -DENABLE_ARC=0
