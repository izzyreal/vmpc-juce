include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_SOURCE_DIR}/resources)
set(_mpc_resources_root ${CMAKE_SOURCE_DIR}/editables/mpc/resources)

function(_bundle_vmpc_juce_resources _target_name)

  file(GLOB_RECURSE VMPC_JUCE_RESOURCES "${_vmpc_juce_resources_root}/*")
  list(FILTER VMPC_JUCE_RESOURCES EXCLUDE REGEX "\\.DS_Store$")

  if (APPLE)
    set_source_files_properties(${VMPC_JUCE_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/img)

    target_sources(vmpc2000xl_AU PRIVATE ${VMPC_JUCE_RESOURCES})
    target_sources(vmpc2000xl_AUv3 PRIVATE ${VMPC_JUCE_RESOURCES})
    target_sources(vmpc2000xl_VST3 PRIVATE ${VMPC_JUCE_RESOURCES})

    file(GLOB_RECURSE MPC_RESOURCES "${_mpc_resources_root}/*")
    list(FILTER MPC_RESOURCES EXCLUDE REGEX "\\.DS_Store$")

    foreach(RESOURCE ${MPC_RESOURCES})
        get_filename_component(SOURCE_DIR "${RESOURCE}" DIRECTORY)
        string(REPLACE "${_mpc_resources_root}" "" RELATIVE_DIR "${SOURCE_DIR}")
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources${RELATIVE_DIR}")
    endforeach()

    target_sources(vmpc2000xl_AU PRIVATE ${MPC_RESOURCES})
    target_sources(vmpc2000xl_AUv3 PRIVATE ${MPC_RESOURCES})
    target_sources(vmpc2000xl_VST3 PRIVATE ${MPC_RESOURCES})
  elseif (MSVC)    
    
    set(_resources_rc_file "${CMAKE_BINARY_DIR}/resources.rc")
    file(WRITE "${_resources_rc_file}" "")

    foreach(RESOURCE ${VMPC_JUCE_RESOURCES})
        string(REPLACE "${_vmpc_juce_resources_root}/" "" RELATIVE_RESOURCE "${RESOURCE}")
        file(APPEND "${_resources_rc_file}" "${RELATIVE_RESOURCE} RCDATA \"${RESOURCE}\"\n")
    endforeach()

    # To do: deduplicate. Both Standalone and VST3 targets need the same file, but it just needs to be copied into
    # their own build dirs as a post-build step.
    add_library(vmpc_juce_standalone_resources MODULE ${_resources_rc_file})
    set_target_properties(vmpc_juce_standalone_resources PROPERTIES LINK_FLAGS "/ENTRY:dummyFunction /NOENTRY")
    get_target_property(VMPC2000XL_STANDALONE_OUTPUT_DIR vmpc2000xl_Standalone RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(vmpc_juce_standalone_resources PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${VMPC2000XL_STANDALONE_OUTPUT_DIR})
    add_dependencies(vmpc2000xl_Standalone vmpc_juce_standalone_resources)

    add_library(vmpc_juce_vst3_resources MODULE ${_resources_rc_file})
    set_target_properties(vmpc_juce_vst3_resources PROPERTIES LINK_FLAGS "/ENTRY:dummyFunction /NOENTRY")
    get_target_property(VMPC2000XL_VST3_OUTPUT_DIR vmpc2000xl_VST3 LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(vmpc_juce_vst3_resources PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${VMPC2000XL_VST3_OUTPUT_DIR})
    add_dependencies(vmpc2000xl_VST3 vmpc_juce_vst3_resources)
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
