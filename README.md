# vmpc-juce, a JUCE implementation of VMPC2000XL, the Akai MPC2000XL emulator

## Building from source just to run, try or use VMPC2000XL or its tests

VMPC2000XL supports a typical CMake workflow, for example on Windows:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "Visual Studio 17 2022"
cmake -B build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3
```
and on macOS:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "Xcode"
cmake -B build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3
```
or Linux:
```
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce && mkdir build
cmake -B build -G "CodeBlocks - Ninja"
cmake -B build --config Release --target vmpc2000xl_Standalone vmpc2000xl_VST3 vmpc2000xl_LV2
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
