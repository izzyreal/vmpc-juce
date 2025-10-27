find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
    message(STATUS "clang-format found: ${CLANG_FORMAT}")
else()
    message(WARNING "clang-format not found! Code style checks and formatting targets will be skipped.")
endif()

if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_SOURCE_DIR}/include/*.h"
        "${CMAKE_SOURCE_DIR}/include/*.hpp"
    )

    add_custom_target(format-vmpc-juce
        COMMAND ${CLANG_FORMAT} -i ${ALL_CXX_SOURCE_FILES}
        COMMENT "Formatting all source files with clang-format"
    )

    add_custom_target(check-format-vmpc-juce
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_CXX_SOURCE_FILES}
        COMMENT "Checking code style with clang-format"
    )
endif()

