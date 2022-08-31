#!/bin/sh

generator=${1:-Ninja}

mkdir -p build

cwd=$(pwd)

cmake \
  -B build \
  -G "${generator}" \
  -DCMAKE_BUILD_TYPE="Release"
