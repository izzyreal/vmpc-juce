from conans import ConanFile, CMake
from sys import platform

class Pkg(ConanFile):
    name = "vmpc-juce"
    description = "JUCE implementation of VMPC2000XL, an Akai MPC2000XL emulator"
    license = "GPL-3.0"
    version = "0.1"
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake", "cmake_paths"
    exports_sources = ["src/*", "resources/*"]
    requires = ("mpc/0.1@izmar/dev", "ctoot/0.2@izmar/dev", "moduru/0.2@izmar/dev")
    url = "https://github.com/izzyreal/vmpc-juce.git"
	
    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        if platform == "linux":
            self.run('cmake --build . --target vmpc2000xl_Standalone vmpc2000xl_LV2')
        else:
            self.run('cmake --build . --target vmpc2000xl_Standalone vmpc2000xl_VST3')
