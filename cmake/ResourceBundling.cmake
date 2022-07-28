include(cmake/CMakeRC.cmake)

set(_vmpc_juce_resources_root ${CMAKE_CURRENT_SOURCE_DIR}/../resources)

function(_bundle_vmpc_juce_resources _target_name)
  set(total_list "")

  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img jpg "${total_list}")
  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img png "${total_list}")
  _add_resource_files(${_target_name} ${_vmpc_juce_resources_root} img gif "${total_list}")
  
  cmrc_add_resource_library(
    vmpc_juce_resources
    ALIAS vmpcjuce::rc
    NAMESPACE vmpcjuce
    WHENCE ${_vmpc_juce_resources_root}
    ${total_list}
    )
  target_link_libraries(${_target_name} PUBLIC vmpcjuce::rc)
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
