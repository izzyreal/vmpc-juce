include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_SOURCE_DIR}/resources)

function(_generate_vmpc_juce_required_resource_manifest)
  file(GLOB_RECURSE VMPC_JUCE_REQUIRED_RESOURCES "${_vmpc_juce_resources_root}/*")
  list(FILTER VMPC_JUCE_REQUIRED_RESOURCES EXCLUDE REGEX "\\.DS_Store$")
  list(SORT VMPC_JUCE_REQUIRED_RESOURCES)

  set(_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${_generated_dir}")

  set(_header_path "${_generated_dir}/VmpcJuceRequiredResources.hpp")
  set(_header_content "#pragma once\n\n#include <array>\n#include <string_view>\n\nnamespace vmpc_juce::required_resources\n{\n")
  list(LENGTH VMPC_JUCE_REQUIRED_RESOURCES _required_count)
  string(APPEND _header_content
         "inline constexpr std::array<std::string_view, ${_required_count}> paths{\n")

  foreach(_resource IN LISTS VMPC_JUCE_REQUIRED_RESOURCES)
    file(RELATIVE_PATH _relative_path "${_vmpc_juce_resources_root}" "${_resource}")
    string(APPEND _header_content "    \"${_relative_path}\",\n")
  endforeach()

  string(APPEND _header_content "};\n}\n")
  file(WRITE "${_header_path}" "${_header_content}")
endfunction()

function(_bundle_vmpc_juce_resources _target_name)
  _generate_vmpc_juce_required_resource_manifest()
  set(_mpc_resources_root ${CMAKE_SOURCE_DIR}/editables/mpc/resources)
  if (DEFINED mpc_SOURCE_DIR AND NOT "${mpc_SOURCE_DIR}" STREQUAL "")
    set(_mpc_resources_root ${mpc_SOURCE_DIR}/resources)
  endif()
  message(STATUS "ResourceBundling: mpc resources root=${_mpc_resources_root}")


  file(GLOB_RECURSE VMPC_JUCE_RESOURCES "${_vmpc_juce_resources_root}/*")
  list(FILTER VMPC_JUCE_RESOURCES EXCLUDE REGEX "\\.DS_Store$")

  if (APPLE)

    foreach(resource ${VMPC_JUCE_RESOURCES})
      get_filename_component(parent_dir ${resource} DIRECTORY)
      get_filename_component(final_segment ${parent_dir} NAME)
      set(resource_path "Resources/${final_segment}")
      set_source_files_properties(${resource} PROPERTIES MACOSX_PACKAGE_LOCATION ${resource_path})
    endforeach()

    # We check if the AUv3 target exists, because it's only generated if the Xcode
    # generator is used. If it does exist, the Standalone will make use of the AUv3's
    # resources, leveraging the fact that the AUv3 is an extension that is embedded in
    # the .app bundle.
    if (TARGET vmpc2000xl_AUv3)
      target_sources(vmpc2000xl_AUv3 PRIVATE ${VMPC_JUCE_RESOURCES})
    else()
      target_sources(vmpc2000xl_Standalone PRIVATE ${VMPC_JUCE_RESOURCES})
    endif()

    if (NOT IOS)
      foreach(resource ${VMPC_JUCE_RESOURCES})
        file(RELATIVE_PATH rel_path "${_vmpc_juce_resources_root}" "${resource}")
        get_filename_component(rel_dir "${rel_path}" DIRECTORY)

        add_custom_command(TARGET vmpc2000xl_LV2 PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vmpc2000xl_LV2>/resources/${rel_dir}"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${resource}" "$<TARGET_FILE_DIR:vmpc2000xl_LV2>/resources/${rel_path}"
        )
      endforeach()

      target_sources(vmpc2000xl_AU PRIVATE ${VMPC_JUCE_RESOURCES})
      target_sources(vmpc2000xl_VST3 PRIVATE ${VMPC_JUCE_RESOURCES})
      target_sources(vmpc2000xl_LV2 PRIVATE ${VMPC_JUCE_RESOURCES})
    endif()

    file(GLOB_RECURSE MPC_RESOURCES "${_mpc_resources_root}/*")
    list(FILTER MPC_RESOURCES EXCLUDE REGEX "\\.DS_Store$")

    foreach(RESOURCE ${MPC_RESOURCES})
        get_filename_component(SOURCE_DIR "${RESOURCE}" DIRECTORY)
        string(REPLACE "${_mpc_resources_root}" "" RELATIVE_DIR "${SOURCE_DIR}")
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources${RELATIVE_DIR}")
    endforeach()

    if (NOT IOS)
      foreach(resource ${MPC_RESOURCES})
        file(RELATIVE_PATH rel_path "${_mpc_resources_root}" "${resource}")
        get_filename_component(rel_dir "${rel_path}" DIRECTORY)

        add_custom_command(TARGET vmpc2000xl_LV2 PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vmpc2000xl_LV2>/resources/${rel_dir}"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${resource}" "$<TARGET_FILE_DIR:vmpc2000xl_LV2>/resources/${rel_path}"
        )
      endforeach()
    endif()

    # We check if the AUv3 target exists, because it's only generated if the Xcode
    # generator is used. If it does exist, the Standalone will make use of the AUv3's
    # resources, leveraging the fact that the AUv3 is an extension that is embedded in
    # the .app bundle.
    if (TARGET vmpc2000xl_AUv3)
      target_sources(vmpc2000xl_AUv3 PRIVATE ${MPC_RESOURCES})
    else()
      target_sources(vmpc2000xl_Standalone PRIVATE ${MPC_RESOURCES})
    endif()
    
    if (NOT IOS)
      target_sources(vmpc2000xl_AU PRIVATE ${MPC_RESOURCES})
      target_sources(vmpc2000xl_VST3 PRIVATE ${MPC_RESOURCES})
      target_sources(vmpc2000xl_LV2 PRIVATE ${MPC_RESOURCES})
    endif()
    
  else()
    cmrc_add_resource_library(
            vmpc_juce_resources
            ALIAS vmpcjuce::rc
            NAMESPACE vmpcjuce
            WHENCE ${_vmpc_juce_resources_root}
            ${VMPC_JUCE_RESOURCES}
    )
    target_link_libraries(${_target_name} PUBLIC vmpcjuce::rc)
  endif()
endfunction()
