from conans import ConanFile, CMake

class Pkg(ConanFile):
    name = "vmpc-juce"
    description = "JUCE implementation of VMPC2000XL, an Akai MPC2000XL emulator"
    license = "GPL-3.0"
    version = "0.1"
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake", "cmake_paths"
    exports_sources = ["src/*", "resources/*"]
    requires = ("juce/0.1@izmar/dev", "mpc/0.1@izmar/dev", "ctoot/0.1@izmar/dev", "moduru/0.1@izmar/dev")
    url = "https://github.com/izzyreal/vmpc-juce.git"
	
    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="src")
        self.run('cmake --build . --target vmpc2000xl_Standalone')
