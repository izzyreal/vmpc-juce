#!/bin/sh

tarcommand=tar

if [[ "$OSTYPE" == "darwin"* ]]; then
  tarcommand=gtar
fi

gh=https://github.com

############## Editable dependencies ################

mkdir -p editables && cd editables

git clone $gh/izzyreal/akaifat -b no-conan
git clone $gh/izzyreal/moduru -b no-conan
git clone $gh/izzyreal/ctoot -b no-conan
git clone $gh/izzyreal/mpc -b no-conan
git clone $gh/izzyreal/juce-raw-keyboard-input-module

cd ..

#####################################################

############# Non-editable dependencies #############

mkdir -p deps && cd deps

gh_opts="-c advice.detachedHead=false --depth 1"

git clone $gh/juce-framework/JUCE -b 7.0.1 $gh_opts
git clone $gh/Tencent/rapidjson -b v1.1.0 $gh_opts
git clone $gh/catchorg/Catch2 -b v3.1.0 $gh_opts
git clone $gh/gulrak/filesystem -b v1.5.12 $gh_opts
git clone $gh/TartanLlama/expected -b v1.0.0 $gh_opts

cd ..

#####################################################

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
  ./editables/ctoot \
  ./editables/juce-raw-keyboard-input-module \
  ./editables/moduru \
  ./editables/mpc \
  ./deps/JUCE \
  ./deps/rapidjson \
  ./deps/Catch2 \
  ./deps/filesystem \
  ./deps/expected