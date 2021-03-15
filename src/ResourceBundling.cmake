function(_vmpc_bundle_resources target_name)
    _bundle_mpc(${target_name})
    _bundle_img(${target_name})
endfunction()

function(_bundle_img target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../resources/img")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        #RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.gif"
        "${_rsrc_root_path}/*.png"
        "${_rsrc_root_path}/*.jpg"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/img")
    endforeach()
endfunction()

function(_bundle_mpc target_name)
    _bundle_fonts(${target_name})
    _bundle_screens(${target_name})
    _bundle_screens_bg(${target_name})
    _bundle_audio(${target_name})
endfunction()

function(_bundle_fonts target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../../mpc/resources/fonts")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.fnt"
        "${_rsrc_root_path}/*.bmp"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/fonts")
    endforeach()
endfunction()

function(_bundle_screens target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../../mpc/resources/screens")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.json"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/screens")
    endforeach()
endfunction()

function(_bundle_screens_bg target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../../mpc/resources/screens/bg")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.bmp"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/screens/bg")
    endforeach()
endfunction()

function(_bundle_audio target_name)
    set(_rsrc_root_path "${CMAKE_CURRENT_SOURCE_DIR}/../../mpc/resources/audio")

    file(
        GLOB_RECURSE _resource_list
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_rsrc_root_path}/*.wav"
    )

    foreach(_resource IN ITEMS ${_resource_list})
        target_sources(${target_name} PUBLIC ${_resource})
        set_source_files_properties(${_resource} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/audio")
    endforeach()
endfunction()