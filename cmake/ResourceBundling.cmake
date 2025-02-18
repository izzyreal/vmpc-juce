include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_SOURCE_DIR}/resources)
set(_mpc_resources_root ${CMAKE_SOURCE_DIR}/editables/mpc/resources)

function(_bundle_vmpc_juce_resources _target_name)

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
    # generator is used.
    if (TARGET vmpc2000xl_AUv3)
      target_sources(vmpc2000xl_AUv3 PRIVATE ${VMPC_JUCE_RESOURCES})
    else()
      target_sources(vmpc2000xl_Standalone PRIVATE ${VMPC_JUCE_RESOURCES})
    endif()
    
    if (NOT IOS)
      target_sources(vmpc2000xl_AU PRIVATE ${VMPC_JUCE_RESOURCES})
      target_sources(vmpc2000xl_VST3 PRIVATE ${VMPC_JUCE_RESOURCES})
    endif()

    file(GLOB_RECURSE MPC_RESOURCES "${_mpc_resources_root}/*")
    list(FILTER MPC_RESOURCES EXCLUDE REGEX "\\.DS_Store$")

    foreach(RESOURCE ${MPC_RESOURCES})
        get_filename_component(SOURCE_DIR "${RESOURCE}" DIRECTORY)
        string(REPLACE "${_mpc_resources_root}" "" RELATIVE_DIR "${SOURCE_DIR}")
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources${RELATIVE_DIR}")
    endforeach()

    # We check if the AUv3 target exists, because it's only generated if the Xcode
    # generator is used.
    if (TARGET vmpc2000xl_AUv3)
      target_sources(vmpc2000xl_AUv3 PRIVATE ${MPC_RESOURCES})
    else()
      target_sources(vmpc2000xl_Standalone PRIVATE ${MPC_RESOURCES})
    endif()
    
    if (NOT IOS)
      target_sources(vmpc2000xl_AU PRIVATE ${MPC_RESOURCES})
      target_sources(vmpc2000xl_VST3 PRIVATE ${MPC_RESOURCES})
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
