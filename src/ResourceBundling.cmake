function(_vmpc_bundle_resources target_name)
    _bundle_img(${target_name})
endfunction()

function(_bundle_img target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../resources")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.gif"
        "${_rsrc_root_path}/*.png"
        "${_rsrc_root_path}/*.jpg"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endforeach()
endfunction()