# vmpc-juce, a JUCE implementation of VMPC2000XL, the Akai MPC2000XL emulator

<p align="center">
  <img src="https://izmar.nl/images/vmpc2000xl-full-gui-screenshot.jpg" width=500 />
</p>

## Building from source just to run, try or use VMPC2000XL or its tests

VMPC2000XL supports a typical CMake workflow, for example on Windows:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3 vmpc2000xl_LV2
```
and on macOS:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "Xcode"
cmake --build build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3 vmpc2000xl_LV2 vmpc2000xl_AU
```
and on macOS to build for iOS:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "Xcode" -DCMAKE_SYSTEM_NAME="iOS"
cmake --build build --config Release --target vmpc2000xl_Standalone
```
or Linux:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "CodeBlocks - Ninja"
cmake --build build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3 vmpc2000xl_LV2
```

Linux CI uses the tracked [Dockerfile](ci/linux/Dockerfile), based on Ubuntu 20.04 with CMake 3.31.5. Both Linux jobs build this image from the checkout before running their steps; Docker caches unchanged image layers. System dependencies, including the XInput development headers needed for Linux touch support, are maintained in that file.

To build the same AMD64 image locally, run this from the repository root:
```sh
docker build --platform linux/amd64 -t ubuntu-vmpc ci/linux
```

On an Apple silicon Mac with Apple Container and Rosetta available:
```sh
container build --platform linux/amd64 -t ubuntu-vmpc ci/linux
container run --rm --arch amd64 --rosetta ubuntu-vmpc cmake --version
```

On Debian you should be able to build after installing these packages:
```sh
sudo apt-get install \
  build-essential \
  wget \
  cmake \
  git \
  ninja-build \
  libasound2-dev \
  libjack-jackd2-dev \
  libfreetype6-dev \
  libx11-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxext-dev \
  libxinerama-dev \
  libxrender-dev \
  libxrandr-dev \
  libxi-dev \
  libudisks2-dev \
  libglib2.0-dev
```

The targets described above are also the currently supported targets for each platform. Note that the standalone builds for macOS and iOS contain the AUv3 as well. Also note that AUv3 is only built if you're using the Xcode generator. AUv2 works fine with other generators.

The above generators are just some examples. If you experience issues with generators other than the ones mentioned here, [file an issue here](https://github.com/izzyreal/vmpc-juce/issues).

If you're actively developing and you've already run the CMake generation command, and you've added new source files, and you don't have internet available, use the following to regenerate the build files:
```
cmake -B build -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

## Modifying and contributing to VMPC2000XL and its dependencies
Just skip the second `cmake -B ...` statement in the above examples and you have the IDE project that you can use for this flow.
The code that is meant to be edited as part of VMPC2000XL will be located in `vmpc-juce/editables` after a successful `cmake -G` run.
These editables are part of the IDE project that a `cmake -G` run produces in `vmpc-juce/build`.
You may notice the editables being in a "detached HEAD" state (a Git thing where you're not currently on any branch), so it's required to do some `git checkout master` (or `git checkout main` if there is no `master`) and `git checkout -b <your_feature_branch_name>` before you can commit your changes.

## Creating a source package for offline building

To create a source package that can be used for building VMPC2000XL offline, run (from the root of this repo):
```bash
mkdir build && cd build
cmake ..
make package_source
```

It creates a `vmpc2000xl-0.4.4-Source.tar.gz` which, unpacked, will yield a directory called `vmpc2000xl-0.4.4-Source`.

Finally, to build all available VMPC2000XL binaries run:
```bash
cd vmpc2000xl-0.4.4-Source
mkdir build && cd build
cmake ..
make vmpc2000xl_All -j 4
```
In this example the Makefile generator is used, but here too you can use any generator you like.

## License
This project is licensed under the GNU General Public License (GPL) version 3 or later.  
It uses the JUCE framework, which is licensed under the GNU Affero General Public License (AGPL).  
See the [LICENSE.txt](LICENSE.txt) file for details.

## Using This Software
If you use or distribute this software, you must comply with the GPL and AGPL terms.

--------------------
MPC® and Akai Professional® are a registered trademarks of inMusic Brands. Inc. This emulator is not affiliated with inMusic and use of the MPC® and Akai Professional® names has not been authorized, sponsored or otherwise approved by inMusic.
