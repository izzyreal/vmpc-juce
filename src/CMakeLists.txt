cmake_minimum_required(VERSION 3.13)

set(VMPC_VERSION 0.3.0.0)
#set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(vmpc2000xl VERSION ${VMPC_VERSION})

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/main/version.h "#pragma once\nnamespace version { static char* get(){ return \"${VMPC_VERSION}\";}}")

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake)

macro(conan_target_link_libraries_keyword target)
    if(CONAN_TARGETS)
        target_link_libraries(${target} PUBLIC ${CONAN_TARGETS})
    else()
        target_link_libraries(${target} PUBLIC ${CONAN_LIBS})
        foreach(_LIB ${CONAN_LIBS_RELEASE})
            target_link_libraries(${target} PUBLIC optimized ${_LIB})
        endforeach()
        foreach(_LIB ${CONAN_LIBS_DEBUG})
            target_link_libraries(${target} PUBLIC debug ${_LIB})
        endforeach()
    endif()
endmacro()

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
    FORMATS Standalone VST3 AU                 # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
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

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

target_sources(vmpc2000xl PRIVATE ${_source_list})

target_compile_definitions(vmpc2000xl
    PUBLIC
    JUCE_JACK=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(vmpc2000xl PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

conan_target_link_libraries_keyword(vmpc2000xl)

target_link_libraries(vmpc2000xl PRIVATE juce::juce_recommended_lto_flags)

if(MSVC)
	target_compile_options(vmpc2000xl PRIVATE "/MP")
endif()
