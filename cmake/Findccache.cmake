# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 Maxim Radugin <maxim@radugin.com>

#[=======================================================================[.rst:
Findccache
----------

This module finds the compiler cache executable on the system and routes 
compilation and linking calls via compiler cache executable for Xcode, 
Visual Studio, Ninja and Unix Makefiles generators. 

Add path where Findccache.cmake is located to CMAKE_MODULE_PATH variable.
For example, if Findccache.cmake is located next to CMakeLists.txt file, use::

  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

``CMAKE_MODULE_PATH`` should be set before calling find_package().
See example project.

For more information on find package modules see:
https://cmake.org/cmake/help/latest/command/find_package.html

Adding following line after ``project()`` will try to enable use of compiler 
cache::

  find_package(ccache)

As of time of writing ccache does not yet support MSVC /Zi flag and requires 
/Z7 instead. If possible, it is advised to switch CMP0141 to NEW and set 
CMAKE_MSVC_DEBUG_INFORMATION_FORMAT to ``Embedded`` in the root CMakeLists.txt 
before first ``project()`` or ``enable_language()`` call. If ``CMP0141`` is set 
to ``OLD``, this script will try the old way of switching debug information 
format by manipulating ``CMAKE_<lang>_FLAGS_<config>`` variables, which is known 
to be unreliable in some cases::

  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
  cmake_policy(SET CMP0141 NEW)

  project(...)

This module will look only for ``ccache`` program by default, force override 
``CCACHE_PROGRAMS`` cache variable to look for for alternative compiler
cache programs::

  set(CCACHE_PROGRAMS "sccache;buildcache;ccache" CACHE STRING _ FORCE)

The following variables are provided to indicate compiler cache support:

``CCACHE_FOUND``
  Variable indicating if the compiler cache executable was found.

``CCACHE_SUPPORTED``
  Variable indicating if the ccache supports current generator.

#]=======================================================================]

set(CCACHE_PROGRAMS "ccache" CACHE STRING "eligible compiler cache programs to look for")

find_program(CCACHE_PROGRAM NAMES ${CCACHE_PROGRAMS} DOC "compiler cache executable")

set(CCACHE_PROGRAM_RESOLVED "${CCACHE_PROGRAM}")
if (WIN32 AND CCACHE_PROGRAM)
    string(REPLACE "\\" "/" _ccache_program_unix "${CCACHE_PROGRAM}")
    string(TOLOWER "${_ccache_program_unix}" _ccache_program_unix_lower)
    if (_ccache_program_unix_lower MATCHES "/chocolatey/bin/ccache\\.exe$")
        get_filename_component(_ccache_bin_dir "${CCACHE_PROGRAM}" DIRECTORY)
        get_filename_component(_choco_root "${_ccache_bin_dir}" DIRECTORY)
        file(GLOB_RECURSE _choco_ccache_candidates
            "${_choco_root}/lib/ccache/tools/*/ccache.exe"
            "${_choco_root}/lib/ccache/tools/ccache.exe"
        )
        if (_choco_ccache_candidates)
            list(SORT _choco_ccache_candidates)
            list(GET _choco_ccache_candidates -1 CCACHE_PROGRAM_RESOLVED)
        endif()
    endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set xxx_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(ccache
    DEFAULT_MSG
    CCACHE_PROGRAM
)

if (CCACHE_FOUND)
    set(CCACHE_SUPPORTED YES)

    if (CMAKE_GENERATOR STREQUAL "Xcode")
        set(c_launcher_file "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_C_COMPILER}\" \"$@\"\n")
        set(cxx_launcher_file "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"\n")
        file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/launch-c CONTENT "${c_launcher_file}")
        file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/launch-cxx CONTENT "${cxx_launcher_file}")
        execute_process(COMMAND chmod a+rx
            "${CMAKE_BINARY_DIR}/launch-c"
            "${CMAKE_BINARY_DIR}/launch-cxx"
        )

        # Set Xcode project attributes to route compilation and linking through our scripts
        set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c"   CACHE INTERNAL _)
        set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx" CACHE INTERNAL _)
        set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c"   CACHE INTERNAL _)
        set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx" CACHE INTERNAL _)
    elseif (CMAKE_GENERATOR MATCHES "Visual Studio")
        # Copy original ccache.exe and rename to cl.exe, this way intermediate cmd file is not needed.
        # On Windows with Chocolatey, CCACHE_PROGRAM may be a shim in chocolatey/bin.
        # Copying the shim breaks because it resolves targets relative to its own path.
        file(COPY_FILE ${CCACHE_PROGRAM_RESOLVED} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)

        # Set Visual Studio global variables:
        # - Use above cl.exe (ccache.exe) as a compiler 
        # - Enable parallel compilation
        list(APPEND CMAKE_VS_GLOBALS
            "CLToolExe=cl.exe"
            "CLToolPath=${CMAKE_BINARY_DIR}"
            "UseMultiToolTask=true"
            "UseStructuredOutput=false"
        )
    elseif(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Unix Makefiles")
        # Support Unix Makefiles and Ninja
        set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(WARNING "Unsupported generator for compiler cache: ${CMAKE_GENERATOR}")
        set(CCACHE_SUPPORTED NO)
    endif()

    # For MSVC compiler need to check and/or set compatible debug information format 
    if (CCACHE_SUPPORTED AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        cmake_policy(GET CMP0141 msvc_debug_info_format_policy)
        if (msvc_debug_info_format_policy STREQUAL "NEW")
            if (NOT CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)
                message(WARNING "CMAKE_MSVC_DEBUG_INFORMATION_FORMAT is not set, set it to \"Embedded\" for compiler cache to work")
                set(CCACHE_SUPPORTED NO)
            elseif (NOT CMAKE_MSVC_DEBUG_INFORMATION_FORMAT STREQUAL "Embedded")
                message(WARNING "Unsupported MSVC debug information format for compiler cache: ${CMAKE_MSVC_DEBUG_INFORMATION_FORMAT}")
                set(CCACHE_SUPPORTED NO)    
            endif()
        else()
            # Use the old way to try to switch from /Zi or /ZI to /Z7 (it is known not to work for some third-party CMake projects)
    
            # Determine if this is being generated by a multi-config CMake generator
            get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

            # If the user has not provided us with the build target then we generate sensible defaults
            if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
                if (IS_MULTI_CONFIG)
                    message(WARNING "Multi-config generator is used without build types defined, setting CMAKE_CONFIGURATION_TYPES using default values")
                    set(CMAKE_CONFIGURATION_TYPES "Debug;MinSizeRel;Release;RelWithDebInfo")
                else()
                    message(WARNING "Single-config generator is used without build type defined, setting CMAKE_BUILD_TYPE using default value")
                    set(CMAKE_BUILD_TYPE "Debug")
                endif()
            endif()

            # For cache to work debug information should be included within object files (/Z7 flag)
            # and will be combined in pdb during linking phase
            set(flag_pattern "[/\-]Z[iI]")
            if (IS_MULTI_CONFIG)
                foreach(config ${CMAKE_CONFIGURATION_TYPES})
                    string(TOUPPER "${config}" config)
                    string(REGEX REPLACE "${flag_pattern}" "/Z7" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
                    string(REGEX REPLACE "${flag_pattern}" "/Z7" CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_${config}}")
                endforeach()
            else()
                string(TOUPPER "${CMAKE_BUILD_TYPE}" config)
                string(REGEX REPLACE "${flag_pattern}" "/Z7" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
                string(REGEX REPLACE "${flag_pattern}" "/Z7" CMAKE_C_FLAGS_${config} "${CMAKE_C_FLAGS_${config}}")
            endif()
        endif()
    endif()
    message(STATUS "Using compiler cache: ${CCACHE_SUPPORTED}")
    mark_as_advanced(CCACHE_PROGRAM)
endif()
