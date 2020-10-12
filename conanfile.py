from conans import ConanFile, CMake

class Pkg(ConanFile):
    name = "vmpc"
    description = "JUCE implementation of vMPC2000XL, an Akai MPC2000XL emulator"
    version = "0.1"
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake"
    requires = ("mpc/0.1@izmar/dev", "ctoot/0.1@izmar/dev", "moduru/0.1@izmar/dev")
    url = "https://github.com/izzyreal/vmpc-juce.git"
	
    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder=".")
        cmake.build()
