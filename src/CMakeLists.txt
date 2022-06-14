cmake_minimum_required(VERSION 3.13)

include(../CPM.cmake)

include(ResourceBundling.cmake)

set(VMPC_VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{APPLE_M1})
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "Minimum OS X deployment version")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(vmpc-fltk VERSION ${VMPC_VERSION})

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/main/version.h "#pragma once\nnamespace version { static char const* get(){ return \"${VMPC_VERSION}\";}}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.txt "${VMPC_VERSION}")

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
else()
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
endif()

conan_basic_setup(NO_OUTPUT_DIRS)

set(_src_root_path "${CMAKE_CURRENT_SOURCE_DIR}/main")
file(
        GLOB_RECURSE _source_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_src_root_path}/*.cpp*"
        "${_src_root_path}/*.hpp*"
        "${_src_root_path}/*.h*"
)

CPMAddPackage(
    NAME fltk
    GIT_TAG branch-1.3
    GITHUB_REPOSITORY fltk/fltk
    OPTIONS "FLTK_BUILD_TEST OFF"
)

CPMAddPackage("gh:PortAudio/portaudio#v19.7.0")

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

add_executable(vmpc-fltk ${_source_list})

if (APPLE)
    target_link_libraries(vmpc-fltk PUBLIC "-framework Cocoa")
endif()

if (UNIX AND NOT APPLE)
    include(FindPkgConfig)
    pkg_search_module(udisks2 REQUIRED udisks2)
    target_link_libraries(vmpc-fltk PRIVATE ${udisks2_LIBRARIES} X11)
endif()


target_link_libraries(vmpc-fltk PUBLIC fltk)
target_link_libraries(vmpc-fltk PUBLIC ${CONAN_LIBS})
target_include_directories(vmpc-fltk PUBLIC ${fltk_SOURCE_DIR})
target_include_directories(vmpc-fltk PUBLIC ${fltk_BINARY_DIR})
target_link_libraries(${vmpc-fltk} fltk fltk_images) 
target_link_libraries(vmpc-fltk PUBLIC portaudio_static)

if(MSVC)
	target_compile_options(vmpc-fltk PRIVATE "/MP")
endif()
