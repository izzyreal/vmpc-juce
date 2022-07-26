# vmpc-juce, a JUCE implementation of VMPC2000XL, the Akai MPC2000XL emulator

## Creating a source package for offline building

To create a source package that can be used for building VMPC2000XL offline, run `./package.sh`.

It creates a `vmpc2000xl-0.4.4.tar.gz` which, unpacked, will yield a directory called `vmpc2000xl-0.4.4`.

To build VMPC2000XL run:
```bash
cd vmpc2000xl-0.4.4
./build.sh
```
