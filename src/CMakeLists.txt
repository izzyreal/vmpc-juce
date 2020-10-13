cmake_minimum_required(VERSION 3.15)
project(vmpc2000xl VERSION 0.2.0)
list(APPEND CMAKE_PREFIX_PATH "C:/Users/Izmar/git/JUCE/install")
find_package(JUCE CONFIG REQUIRED)
juce_disable_default_flags()

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
else()
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
endif()

conan_basic_setup(NO_OUTPUT_DIRS)

juce_add_plugin(vmpc2000xl
    ICON_BIG "main/resources/icon.png"
    COMPANY_NAME Izmar
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE Izmr               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE 2kXL                            # A unique four-character plugin id with at least one upper-case character
    FORMATS Standalone VST3                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "vmpc2000xl")        # The name of the final executable, which can differ from the target name

set(CMAKE_CXX_STANDARD 17)
set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}/main")
file(
        GLOB_RECURSE _source_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_src_root_path}/*.cpp*"
        "${_src_root_path}/*.hpp*"
        "${_src_root_path}/*.h*"
)

target_sources(vmpc2000xl PRIVATE
        ${_source_list})

target_compile_definitions(vmpc2000xl
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(vmpc2000xl PRIVATE
    juce::juce_audio_utils mpc moduru ctoot)

if(MSVC)
	target_compile_options(vmpc2000xl PRIVATE "/MP")
endif()