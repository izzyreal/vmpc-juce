cmake_minimum_required(VERSION 3.13)

include(FetchContent)
include(cmake/ResourceBundling.cmake)
include(cmake/ASIOSDK.cmake)

# Note that IOS (as of CMake 3.14) is not set until the project() call is made.
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(VMPC_VERSION 0.5.0)
else()
  set(VMPC_VERSION 0.5.0.2)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_SUPPRESS_REGENERATION true)
add_compile_definitions(RAPIDJSON_NOMEMBERITERATORCLASS)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "Minimum OS X deployment version")
endif()

set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(vmpc2000xl VERSION ${VMPC_VERSION})

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/version.h "#pragma once\nnamespace version { static char const* get(){ return \"${VMPC_VERSION}\";}}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt "${VMPC_VERSION}")

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/izzyreal/JUCE.git
  GIT_TAG        patched-for-ios-7.0.5
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/JUCE
)

FetchContent_Declare(juce-raw-keyboard-input-module
  GIT_REPOSITORY https://github.com/izzyreal/juce-raw-keyboard-input-module.git
  GIT_TAG        main
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/juce-raw-keyboard-input-module
)

FetchContent_Declare(mpc
  GIT_REPOSITORY https://github.com/izzyreal/mpc.git
  GIT_TAG        master
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/mpc
)

FetchContent_Declare(ctoot
  GIT_REPOSITORY https://github.com/izzyreal/ctoot.git
  GIT_TAG        master
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/ctoot
)

FetchContent_Declare(moduru
  GIT_REPOSITORY https://github.com/izzyreal/moduru.git
  GIT_TAG        master
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/moduru
)

FetchContent_Declare(tl-expected
  GIT_REPOSITORY https://github.com/TartanLlama/expected.git
  GIT_TAG        v1.0.0
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/deps/expected
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)

FetchContent_MakeAvailable(moduru mpc ctoot JUCE juce-raw-keyboard-input-module)

FetchContent_GetProperties(tl-expected)

if(NOT tl-expected_POPULATED)
  FetchContent_Populate(tl-expected)
endif()

juce_add_plugin(vmpc2000xl
    
    # Relevant to iOS only
    REQUIRES_FULL_SCREEN TRUE
    BACKGROUND_AUDIO_ENABLED TRUE
    APP_GROUPS_ENABLED TRUE
    APP_GROUP_IDS group.nl.izmar.vmpc2000xl
    FILE_SHARING_ENABLED TRUE
    DOCUMENT_BROWSER_ENABLED TRUE

    # Relevant to macOS and iOS
    BUNDLE_ID nl.izmar.vmpc2000xl

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


target_include_directories(vmpc2000xl PUBLIC
  ${mpc_SOURCE_DIR}/src/main
  ${ctoot_SOURCE_DIR}/src/main
  ${moduru_SOURCE_DIR}/src/main
  ${tl-expected_SOURCE_DIR}/include
)

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}/src/main")

if (IOS)
  file(
    GLOB_RECURSE _source_list
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path}/*.c*"
    "${_src_root_path}/*.h*"
        "${_src_root_path}/*.m*"
  )
else()
  file(
    GLOB_RECURSE _source_list
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path}/*.c*"
    "${_src_root_path}/*.h*"
  )
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

target_sources(vmpc2000xl PRIVATE ${_source_list})

target_compile_definitions(vmpc2000xl
    PUBLIC
    JUCE_JACK=1
    JUCE_ASIO=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

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

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
  execute_process(
    COMMAND python3 macos-codesign-details-extractor.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE DEVELOPMENT_TEAM)
  set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}")
  message("\nSetting iOS development team to organisational unit: ${DEVELOPMENT_TEAM}\n"
    "This is derived from the first valid Apple Development certificate in your local keychain.\n"
    "You can tweak it in the Build Settings tab of your root project.\n")
endif()

if(MSVC)
  getAndIncludeASIOSDK(asiosdk_2.3.3_2019-06-14)
	target_compile_options(vmpc2000xl PRIVATE "/MP")
endif()

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /build
    /\\\\.DS_Store
)
include(CPack)
