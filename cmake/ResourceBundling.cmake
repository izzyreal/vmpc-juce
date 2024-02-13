include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_CURRENT_SOURCE_DIR}/resources)

function(_bundle_vmpc_juce_resources _target_name)
  set(total_list "")

  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img jpg "${total_list}")
  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img png "${total_list}")
  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img gif "${total_list}")

  if (APPLE)

    file(GLOB_RECURSE MPC_RESOURCES "${CMAKE_SOURCE_DIR}/editables/mpc/resources/*")

    list(FILTER MPC_RESOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/editables/mpc/resources/test/.*")

    foreach(RESOURCE ${MPC_RESOURCES})
        get_filename_component(SOURCE_DIR "${RESOURCE}" DIRECTORY)
        string(REPLACE "${CMAKE_SOURCE_DIR}/editables/mpc/resources" "" RELATIVE_DIR "${SOURCE_DIR}")
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources${RELATIVE_DIR}")
    endforeach()

    target_sources(vmpc2000xl_AUv3 PRIVATE ${MPC_RESOURCES})

    set_source_files_properties(
            ${total_list}
            PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources/img
    )

    target_sources(vmpc2000xl_AUv3 PRIVATE
            ${total_list}
    )

    string(REPLACE "resources" "Resources" total_list_upper_case ${total_list})

    set_target_properties(vmpc2000xl PROPERTIES
            RESOURCE ${total_list_upper_case}
    )
  else()
    cmrc_add_resource_library(
            vmpc_juce_resources
            ALIAS vmpcjuce::rc
            NAMESPACE vmpcjuce
            WHENCE ${_vmpc_juce_resources_root}
            ${total_list}
    )
    target_link_libraries(${_target_name} PUBLIC vmpcjuce::rc)
  endif()
endfunction()

function(_add_resource_files _target_name _rsrc_root_path _sub_dir _extension _total_list)
  file(
    GLOB _list
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    "${_rsrc_root_path}/${_sub_dir}/*.${_extension}"
    )

  list (APPEND _total_list ${_list})
  set (total_list ${_total_list} PARENT_SCOPE)
endfunction()
