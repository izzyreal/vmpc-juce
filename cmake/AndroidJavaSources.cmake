function(vmpc_stage_android_java_sources juce_source_dir raw_keyboard_source_dir)
  if(NOT ANDROID)
    return()
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_variant "debug")
  else()
    set(_variant "release")
  endif()

  set(_source_roots
    "${juce_source_dir}/modules/juce_core/native/javacore/init"
    "${juce_source_dir}/modules/juce_core/native/javacore/app"
    "${juce_source_dir}/modules/juce_gui_basics/native/javaopt/app"
    "${raw_keyboard_source_dir}/android/java")

  foreach(_source_root IN LISTS _source_roots)
    if(NOT IS_DIRECTORY "${_source_root}")
      message(FATAL_ERROR "Android Java source directory not found: ${_source_root}")
    endif()
  endforeach()

  set(_output_base
    "${CMAKE_SOURCE_DIR}/android/app/build/generated/source/vmpcNativeDependencies")
  set(_output_dir "${_output_base}/${_variant}")
  file(MAKE_DIRECTORY "${_output_base}")

  file(LOCK "${_output_base}/${_variant}.lock"
    GUARD FUNCTION
    TIMEOUT 120
    RESULT_VARIABLE _lock_result)
  if(NOT _lock_result STREQUAL "0")
    message(FATAL_ERROR
      "Failed to lock Android Java staging directory for ${_variant}: ${_lock_result}")
  endif()

  file(MAKE_DIRECTORY "${_output_dir}")
  foreach(_source_root IN LISTS _source_roots)
    file(COPY "${_source_root}/" DESTINATION "${_output_dir}")
  endforeach()
endfunction()
