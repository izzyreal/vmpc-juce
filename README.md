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
or Linux:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "CodeBlocks - Ninja"
cmake --build build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3 vmpc2000xl_LV2
```

Note that on Linux you need some system dependencies. I don't have an exhaustive list that will work for all distributions and installations. What I can provide is how the binaries that are published on izmar.nl are built, which is in an official Ubuntu 20.04 Docker container, using the below Dockerfile:
```dockerfile
FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
  && apt-get -y install build-essential \
  && apt-get install -y wget \
  && rm -rf /var/lib/apt/lists/* \
  && wget https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /opt/cmake-3.24.1 \
      && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.24.1 \
      && rm /tmp/cmake-install.sh \
      && ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin
RUN rm -vf /var/lib/apt/lists/* && apt-get update
RUN apt-get -y install git
RUN apt-get -y install libasound2-dev libjack-jackd2-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrender-dev
RUN apt-get -y install ninja-build
RUN apt -y install libxrandr-dev libudisks2-dev libglib2.0-dev
```

The targets described above are also the currently supported targets for each platform.

The above generators are just some examples. If you experience issues with generators other than the ones mentioned here, [file an issue here](https://github.com/izzyreal/vmpc-juce/issues).

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
