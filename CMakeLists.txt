cmake_minimum_required(VERSION 3.13)

include(CPM.cmake)

include(ResourceBundling.cmake)

set(VMPC_VERSION 0.4.4)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{APPLE_M1})
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "Minimum OS X deployment version")
endif()

set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(vmpc2000xl VERSION ${VMPC_VERSION})

add_definitions(-DJUCE_MODAL_LOOPS_PERMITTED)

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/version.h "#pragma once\nnamespace version { static char const* get(){ return \"${VMPC_VERSION}\";}}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt "${VMPC_VERSION}")

CPMAddPackage("gh:juce-framework/JUCE#7.0.1")
CPMAddPackage("gh:izzyreal/juce-raw-keyboard-input-module#main")
CPMAddPackage("gh:izzyreal/mpc#no-conan")
CPMAddPackage("gh:izzyreal/ctoot#no-conan")
CPMAddPackage("gh:izzyreal/moduru#no-conan")

juce_disable_default_flags()

include_directories(${MPC_SOURCE_DIR}/src/main ${CTOOT_SOURCE_DIR}/src/main ${MODURU_SOURCE_DIR}/src/main)
link_directories(${MPC_BINARY_DIR} ${CTOOT_BINARY_DIR} ${MODURU_BINARY_DIR})

juce_add_plugin(vmpc2000xl
    
    # Relevant to iOS only
    REQUIRES_FULL_SCREEN TRUE
    BACKGROUND_AUDIO_ENABLED TRUE
    APP_GROUPS_ENABLED TRUE
    APP_GROUP_IDS group.com.Izmar.vmpc2000xl
    FILE_SHARING_ENABLED TRUE
    PLIST_TO_MERGE "<plist version=\"1.0\"><dict><key>LSSupportsOpeningDocumentsInPlace</key><true/></dict></plist>"

    # Relevant to macOS and iOS
    BUNDLE_ID com.Izmar.vmpc2000xl

    # Relevant to all builds
    ICON_BIG "src/main/resources/icon.png"
    COMPANY_NAME Izmar
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE Izmr
    PLUGIN_CODE 2kXL
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Allow VMPC2000XL to record from the microphone?"
    FORMATS LV2 VST3 AU AUv3 Standalone
    LV2URI "http://www.izmar.nl/VMPC2000XL"
    PRODUCT_NAME "VMPC2000XL")

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}/src/main")
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
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if (IOS)
  target_compile_definitions(vmpc2000xl PUBLIC JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0)
else()
  target_compile_definitions(vmpc2000xl PUBLIC JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)  
endif()

if (UNIX AND NOT APPLE)
    include(FindPkgConfig)
    pkg_search_module(udisks2 REQUIRED udisks2)
    target_link_libraries(vmpc2000xl PRIVATE ${udisks2_LIBRARIES} X11)
endif()

target_link_libraries(vmpc2000xl PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    raw_keyboard_input
    mpc ctoot moduru)

_bundle_vmpc_juce_resources(vmpc2000xl)

if(MSVC)
	target_compile_options(vmpc2000xl PRIVATE "/MP")
endif()
