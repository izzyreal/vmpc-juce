include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_SOURCE_DIR}/resources)

function(_bundle_vmpc_juce_resources _target_name)
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
