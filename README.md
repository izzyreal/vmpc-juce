# vmpc-juce, a JUCE implementation of VMPC2000XL, the Akai MPC2000XL emulator

## Quick start

Requirements:
- Visual Studio 2019, Xcode or Make
- [CMake](https://cmake.org/)
- [Python](https://www.python.org/downloads/)
- [Conan](https://docs.conan.io/en/latest/installation.html)

Once you have these tools installed, run the following commands:
```
conan remote add jfrog-izmar https://izmar.jfrog.io/artifactory/api/conan/dev
git clone https://github.com/izzyreal/vmpc-juce
cd vmpc-juce
md build && cd build
conan install .. --build missing && conan build ..
```

This will build a self-contained standalone desktop executable for Linux, MacOS or Windows.

To hack around in the code in an IDE please refer to https://github.com/izzyreal/vmpc-workspace
