function(fetchcontent_resolve_git_hash repository ref out_hash_var)
  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required to resolve FetchContent hash for ${repository} (${ref})")
  endif()
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" ls-remote "${repository}" "${ref}"
    RESULT_VARIABLE _git_result
    OUTPUT_VARIABLE _git_output
    ERROR_VARIABLE _git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if(NOT _git_result EQUAL 0)
    message(FATAL_ERROR "git ls-remote failed for ${repository} ${ref}: ${_git_error}")
  endif()
  if("${_git_output}" STREQUAL "")
    message(FATAL_ERROR "git ls-remote returned no output for ${repository} ${ref}")
  endif()
  string(REGEX MATCH "^[0-9a-fA-F]+" _git_hash "${_git_output}")
  if("${_git_hash}" STREQUAL "")
    message(FATAL_ERROR "failed to parse git hash from ls-remote output: ${_git_output}")
  endif()
  string(TOLOWER "${_git_hash}" _git_hash)
  set(${out_hash_var} "${_git_hash}" PARENT_SCOPE)
endfunction()

function(fetchcontent_resolve_source dep_key cache_subdir default_path repository git_ref out_source_dir out_git_tag out_hash)
  string(TOUPPER "${dep_key}" dep_upper)
  set(dep_var "FETCHCONTENT_SOURCE_DIR_${dep_upper}")
  set(dep_source_hash "")
  set(dep_source_dir "${default_path}")
  set(dep_git_tag "${git_ref}")
  set(cache_root "${FETCHCONTENT_CACHE_ROOT}")

  if(DEFINED ${dep_var} AND NOT "${${dep_var}}" STREQUAL "")
    set(dep_source_dir "${${dep_var}}")
  elseif(NOT "${cache_root}" STREQUAL "")
    if(NOT "${repository}" STREQUAL "" AND NOT "${git_ref}" STREQUAL "")
      fetchcontent_resolve_git_hash("${repository}" "${git_ref}" dep_source_hash)
      set(dep_source_dir "${cache_root}/${cache_subdir}-${dep_source_hash}/${cache_subdir}")
      set(dep_git_tag "${dep_source_hash}")
    else()
      set(dep_source_dir "${cache_root}/${cache_subdir}")
    endif()
  endif()

  if(NOT "${dep_source_hash}" STREQUAL "")
    message(STATUS "FetchContent ${dep_key}: ${git_ref} -> ${dep_source_hash}")
  endif()
  set(${out_source_dir} "${dep_source_dir}" PARENT_SCOPE)
  set(${out_git_tag} "${dep_git_tag}" PARENT_SCOPE)
  set(${out_hash} "${dep_source_hash}" PARENT_SCOPE)
endfunction()

function(fetchcontent_declare_cached_git dep_name cache_subdir default_path repository git_ref)
  fetchcontent_resolve_source(
    "${dep_name}"
    "${cache_subdir}"
    "${default_path}"
    "${repository}"
    "${git_ref}"
    _source_dir
    _git_tag
    _source_hash
  )

  string(TOUPPER "${dep_name}" _dep_upper)
  set(_dep_override_var "FETCHCONTENT_SOURCE_DIR_${_dep_upper}")
  if(IS_DIRECTORY "${_source_dir}")
    file(GLOB _cached_entries LIST_DIRECTORIES true "${_source_dir}/*")
    list(LENGTH _cached_entries _cached_entry_count)
    if(_cached_entry_count GREATER 0)
      set(${_dep_override_var} "${_source_dir}" PARENT_SCOPE)
      message(STATUS "FetchContent ${dep_name}: reusing cached source ${_source_dir}")
    endif()
  endif()

  FetchContent_Declare(${dep_name}
    GIT_REPOSITORY "${repository}"
    GIT_TAG "${_git_tag}"
    SOURCE_DIR "${_source_dir}"
    ${ARGN}
  )
  set("${dep_name}_FETCHCONTENT_SOURCE_DIR" "${_source_dir}" PARENT_SCOPE)
  set("${dep_name}_FETCHCONTENT_GIT_TAG" "${_git_tag}" PARENT_SCOPE)
  set("${dep_name}_FETCHCONTENT_SOURCE_HASH" "${_source_hash}" PARENT_SCOPE)
endfunction()

function(fetchcontent_declare_cached_url dep_name cache_subdir default_path url)
  fetchcontent_resolve_source(
    "${dep_name}"
    "${cache_subdir}"
    "${default_path}"
    ""
    ""
    _source_dir
    _git_tag
    _source_hash
  )

  string(TOUPPER "${dep_name}" _dep_upper)
  set(_dep_override_var "FETCHCONTENT_SOURCE_DIR_${_dep_upper}")
  if(IS_DIRECTORY "${_source_dir}")
    file(GLOB _cached_entries LIST_DIRECTORIES true "${_source_dir}/*")
    list(LENGTH _cached_entries _cached_entry_count)
    if(_cached_entry_count GREATER 0)
      set(${_dep_override_var} "${_source_dir}" PARENT_SCOPE)
      message(STATUS "FetchContent ${dep_name}: reusing cached source ${_source_dir}")
    endif()
  endif()

  FetchContent_Declare(${dep_name}
    URL "${url}"
    SOURCE_DIR "${_source_dir}"
    ${ARGN}
  )
  set("${dep_name}_FETCHCONTENT_SOURCE_DIR" "${_source_dir}" PARENT_SCOPE)
endfunction()
