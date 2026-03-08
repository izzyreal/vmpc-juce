function(fetchcontent_run_git out_result out_output out_error)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" ${ARGN}
    RESULT_VARIABLE _git_result
    OUTPUT_VARIABLE _git_output
    ERROR_VARIABLE _git_error
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  set(${out_result} "${_git_result}" PARENT_SCOPE)
  set(${out_output} "${_git_output}" PARENT_SCOPE)
  set(${out_error} "${_git_error}" PARENT_SCOPE)
endfunction()

function(fetchcontent_resolve_git_hash repository ref out_hash_var)
  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required to resolve FetchContent hash for ${repository} (${ref})")
  endif()
  fetchcontent_run_git(_git_result _git_output _git_error ls-remote "${repository}" "${ref}")
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

function(fetchcontent_try_prepare_git_source dep_name source_dir repository git_tag)
  set(_needs_clone OFF)
  if(EXISTS "${source_dir}/.git")
    fetchcontent_run_git(_fetch_result _fetch_output _fetch_error -C "${source_dir}" fetch --depth 1 origin "${git_tag}")
    if(NOT _fetch_result EQUAL 0)
      set(_needs_clone ON)
    endif()
  else()
    set(_needs_clone ON)
  endif()

  if(_needs_clone)
    file(REMOVE_RECURSE "${source_dir}")
    get_filename_component(_source_parent "${source_dir}" DIRECTORY)
    file(MAKE_DIRECTORY "${_source_parent}")
    fetchcontent_run_git(_clone_result _clone_output _clone_error clone --no-checkout "${repository}" "${source_dir}")
    if(NOT _clone_result EQUAL 0)
      message(FATAL_ERROR "FetchContent ${dep_name}: git clone failed for ${repository} -> ${source_dir}: ${_clone_error}")
    endif()
    fetchcontent_run_git(_fetch_result _fetch_output _fetch_error -C "${source_dir}" fetch --depth 1 origin "${git_tag}")
    if(NOT _fetch_result EQUAL 0)
      message(FATAL_ERROR "FetchContent ${dep_name}: git fetch failed for ${git_tag} in ${source_dir}: ${_fetch_error}")
    endif()
  endif()

  fetchcontent_run_git(_reset_result _reset_output _reset_error -C "${source_dir}" reset --hard FETCH_HEAD)
  if(NOT _reset_result EQUAL 0)
    message(FATAL_ERROR "FetchContent ${dep_name}: git reset failed in ${source_dir}: ${_reset_error}")
  endif()

  fetchcontent_run_git(_clean_result _clean_output _clean_error -C "${source_dir}" clean -fdx)
  if(NOT _clean_result EQUAL 0)
    message(FATAL_ERROR "FetchContent ${dep_name}: git clean failed in ${source_dir}: ${_clean_error}")
  endif()

  if(NOT EXISTS "${source_dir}/CMakeLists.txt")
    message(FATAL_ERROR "FetchContent ${dep_name}: prepared cache source is missing CMakeLists.txt at ${source_dir}")
  endif()
endfunction()

function(fetchcontent_prepare_cached_git_source dep_name source_dir repository git_tag)
  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required to prepare FetchContent cache for ${dep_name}")
  endif()
  if("${FETCHCONTENT_CACHE_ROOT}" STREQUAL "")
    message(FATAL_ERROR "FetchContent ${dep_name}: FETCHCONTENT_CACHE_ROOT is required for cached git source preparation")
  endif()
  get_filename_component(_source_parent "${source_dir}" DIRECTORY)
  file(MAKE_DIRECTORY "${_source_parent}")
  set(_locks_dir "${FETCHCONTENT_CACHE_ROOT}/.locks")
  file(MAKE_DIRECTORY "${_locks_dir}")
  string(MD5 _lock_key "${source_dir}")
  set(_lock_path "${_locks_dir}/${dep_name}-${_lock_key}.lock")
  file(LOCK "${_lock_path}" GUARD FUNCTION TIMEOUT 300 RESULT_VARIABLE _lock_result)
  if(NOT _lock_result STREQUAL "0")
    message(FATAL_ERROR "FetchContent ${dep_name}: failed to acquire cache lock ${_lock_path}: ${_lock_result}")
  endif()

  fetchcontent_try_prepare_git_source("${dep_name}" "${source_dir}" "${repository}" "${git_tag}")
  message(STATUS "FetchContent ${dep_name}: prepared cached source ${source_dir}")
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
  if(NOT "${FETCHCONTENT_CACHE_ROOT}" STREQUAL "")
    fetchcontent_prepare_cached_git_source("${dep_name}" "${_source_dir}" "${repository}" "${_git_tag}")
    set(${_dep_override_var} "${_source_dir}" PARENT_SCOPE)
  elseif(IS_DIRECTORY "${_source_dir}")
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
