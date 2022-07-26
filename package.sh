#!/bin/sh

tarcommand=tar

if [[ "$OSTYPE" == "darwin"* ]]; then
  tarcommand=gtar
fi

git submodule update --recursive
$tarcommand \
  -cvpzf vmpc2000xl-0.4.4.tar.gz \
  --transform 's,^,vmpc2000xl-0.4.4/,' \
  ./CPM.cmake \
  ./LICENSE.txt \
  ./README.md \
  ./plugin-compatibility-matrix.md \
  ./resources \
  ./src \
  ./CMakeLists.txt \
  ./CMakeRC.cmake \
  ./ResourceBundling.cmake \
  ./build.sh \
  ./ctoot \
  ./juce-raw-keyboard-input-module \
  ./moduru \
  ./mpc