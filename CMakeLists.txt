cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#add_definitions(-DJUCE_ENABLE_REPAINT_DEBUGGING)
#add_compile_definitions(JUCE_IOS_AUDIO_LOGGING=1)

include(FetchContent)
include(cmake/ResourceBundling.cmake)
include(cmake/ASIOSDK.cmake)
include(cmake/ClangFormat.cmake)

find_package(Git)

# Note that IOS (as of CMake 3.14) is not set until the project() call is made.
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(VMPC_VERSION 0.9.0)
else()
  set(VMPC_VERSION 0.9.0.17)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_SUPPRESS_REGENERATION true)

if(APPLE)
  if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE INTERNAL "Minimum iOS deployment version")
  else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE INTERNAL "Minimum OS X deployment version")
  endif()
endif()

set (JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set (JUCE_COPY_PLUGIN_AFTER_BUILD ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

if (APPLE)
    project(vmpc2000xl VERSION ${VMPC_VERSION} LANGUAGES C CXX OBJC OBJCXX)
else()
    project(vmpc2000xl VERSION ${VMPC_VERSION} LANGUAGES C CXX)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

string(TIMESTAMP CONFIGURE_TIMESTAMP "%a, %d %b %Y %H:%M:%S")

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/build_info/VmpcJuceBuildInfo.hpp "#pragma once\nnamespace vmpc_juce::build_info { inline const char *getVersionString(){ return \"${VMPC_VERSION}\"; } inline const char *getTimeStampString(){ return \"${CONFIGURE_TIMESTAMP}\"; }}")

# The version.txt file can be picked up by the CI/CD, so it can inject version info
# into binaries, installers, GitHub releases, etc.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt "${VMPC_VERSION}")

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/izzyreal/JUCE.git
  GIT_TAG        7.0.9-vmpc
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/JUCE
  GIT_SHALLOW    ON
)

FetchContent_Declare(melatonin_blur
    GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
    GIT_TAG origin/main
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_blur)

FetchContent_Declare(juce-raw-keyboard-input-module
  GIT_REPOSITORY https://github.com/izzyreal/juce-raw-keyboard-input-module.git
  GIT_TAG        external-focus-provider
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/juce-raw-keyboard-input-module
)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(mpc
  GIT_REPOSITORY https://github.com/izzyreal/mpc.git
  GIT_TAG        master
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/editables/mpc
)

set(BUILD_EXAMPLES OFF)
set(BUILD_TESTS OFF)
set(INSTALL_PROJECT OFF)

if(APPLE)
if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
FetchContent_Declare(miniz
  GIT_REPOSITORY https://github.com/richgel999/miniz
  GIT_TAG        master
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/deps/miniz
  GIT_SHALLOW    ON
)
message(STATUS "Downloading miniz...")
FetchContent_MakeAvailable(miniz)
endif()
endif()

message(STATUS "Downloading mpc...")
FetchContent_MakeAvailable(mpc)

message(STATUS "Downloading JUCE...")
FetchContent_MakeAvailable(JUCE)

message(STATUS "Downloading juce-raw-keyboard-input-module...")
FetchContent_MakeAvailable(juce-raw-keyboard-input-module)

message(STATUS "Downloading melatonin_blur...")
FetchContent_MakeAvailable(melatonin_blur)

message(STATUS "Downloading json...")
FetchContent_MakeAvailable(json)

if (IOS)
    set(IOS_SPECIFIC_SETTINGS
        REQUIRES_FULL_SCREEN TRUE
        IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
        BACKGROUND_AUDIO_ENABLED TRUE
        APP_GROUPS_ENABLED TRUE
        APP_GROUP_IDS group.nl.izmar.vmpc2000xl
        FILE_SHARING_ENABLED TRUE
        DOCUMENT_BROWSER_ENABLED TRUE
        TARGETED_DEVICE_FAMILY "2"
        FORMATS AUv3 Standalone)
else()
    set(NON_IOS_SPECIFIC_SETTINGS
        VST3_CATEGORIES Instrument Drum OnlyRT Sampler Stereo
        VST_NUM_MIDI_INS 1
        VST_NUM_MIDI_OUTS 1
        FORMATS LV2 VST3 AU AUv3 Standalone
        LV2URI "http://www.izmar.nl/VMPC2000XL")
endif()

# Single call to juce_add_plugin with conditional settings
juce_add_plugin(vmpc2000xl
    BUNDLE_ID nl.izmar.vmpc2000xl
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
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
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT "This enables VMPC2000XL to use Bluetooth MIDI devices"
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS com.apple.security.device.audio-input com.apple.security.device.bluetooth
    PRODUCT_NAME "VMPC2000XL"
    ${IOS_SPECIFIC_SETTINGS}
    ${NON_IOS_SPECIFIC_SETTINGS})

if (IOS)
    function(add_dsym target)
        set_target_properties(${target} PROPERTIES
            XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "$(inherited)"
            XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=RelWithDebInfo] "YES"
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
    endfunction()

    add_dsym(vmpc2000xl_Standalone)
    add_dsym(vmpc2000xl_AUv3)
endif()

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}/src/main")

target_include_directories(vmpc2000xl PUBLIC
  ${_src_root_path}
  ${mpc_SOURCE_DIR}/src/main
)

if (APPLE)
  file(
    GLOB_RECURSE _source_list
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path}/*.c"
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

if (IOS)
  target_link_libraries(vmpc2000xl PRIVATE miniz)
endif()

target_link_libraries(vmpc2000xl
  PRIVATE
    juce::juce_audio_utils
    nlohmann_json::nlohmann_json
    melatonin_blur
    raw_keyboard_input
    mpc
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

_bundle_vmpc_juce_resources(vmpc2000xl)

if (APPLE)
  execute_process(
    COMMAND python3 macos-codesign-details-extractor.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE DEVELOPMENT_TEAM)
  set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}")
  message("\nSetting macOS and iOS development team to organisational unit: ${DEVELOPMENT_TEAM}\n"
    "This is derived from the first valid Apple Development certificate in your local keychain.\n"
    "You can tweak it in the Build Settings tab of your root project.\n")

if (NOT IOS)
    target_link_libraries(vmpc2000xl PRIVATE "-framework Carbon")
endif()
endif()

if(MSVC)
    getAndIncludeASIOSDK(asiosdk_2.3.3_2019-06-14)
    target_compile_options(vmpc2000xl PRIVATE "/MP")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

if(GIT_FOUND)
  if (EXISTS ${CMAKE_SOURCE_DIR}/editables/mpc)
    execute_process(COMMAND ${GIT_EXECUTABLE} remote set-url --push origin git@github.com:izzyreal/mpc
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/editables/mpc
                    RESULT_VARIABLE GIT_REMOTE_RESULT)
    if(NOT GIT_REMOTE_RESULT EQUAL "0")
        message(FATAL_ERROR "git remote set-url --push origin git@github.com:izzyreal/mpc failed with ${GIT_REMOTE_RESULT}. Please contact your local Git guru if you want to push changes.")
    endif()
  endif()
endif()

if (PROJECT_IS_TOP_LEVEL)
    # Create symlink to compile_commands.json for IDE to pick it up
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    )
endif()

set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /build
    /\\\\.DS_Store
)
include(CPack)
