# vmpc-juce, a JUCE implementation of VMPC2000XL, the Akai MPC2000XL emulator

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
