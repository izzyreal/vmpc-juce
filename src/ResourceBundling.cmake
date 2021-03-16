function(_vmpc_bundle_resources _target_name)
  # mpc
  _bundle(${_target_name} ../../mpc/resources/screens json screens)
  _bundle(${_target_name} ../../mpc/resources/screens/bg bmp screens/bg)
  _bundle(${_target_name} ../../mpc/resources/fonts fnt fonts)
  _bundle(${_target_name} ../../mpc/resources/fonts bmp fonts)
  _bundle(${_target_name} ../../mpc/resources/audio wav audio)
  
  # vmpc-juce
  _bundle(${_target_name} ../resources/img jpg img)
  _bundle(${_target_name} ../resources/img png img)
  _bundle(${_target_name} ../resources/img gif img)
endfunction()

function(_bundle _target_name _src_sub_dir _extension _group)
  set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/${_src_sub_dir}")

  file(
    GLOB_RECURSE _resource_list
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_rsrc_root_path}/*.${_extension}"
    )

  foreach(_resource IN ITEMS ${_resource_list})
    set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_group}")
    source_group("Resources/${_group}" FILES "${_resource}")
    target_sources(${_target_name} PUBLIC ${_resource})
  endforeach()  
endfunction()
