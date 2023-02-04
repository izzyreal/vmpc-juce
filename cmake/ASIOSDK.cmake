macro(getAndIncludeASIOSDK ASIO_SDK_VERSION)

    list(APPEND CMAKE_MESSAGE_INDENT "  ")

    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/deps/asiosdk/include/asiosdk/common)
        file(
            MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/asiosdk/include/asiosdk
        )
        message("Fetching ASIO SDK")
        file(
            DOWNLOAD https://download.steinberg.net/sdk_downloads/${ASIO_SDK_VERSION}.zip "${CMAKE_CURRENT_BINARY_DIR}/asiosdk.zip"
            TIMEOUT 60
        )
        message("Extracting ASIO SDK")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_BINARY_DIR}/asiosdk.zip")

        file(
            RENAME ${CMAKE_CURRENT_BINARY_DIR}/${ASIO_SDK_VERSION}/common ${CMAKE_CURRENT_SOURCE_DIR}/deps/asiosdk/include/asiosdk/common
        )
        # tidy up the download and extracted files that no longer needed
        file(
            REMOVE ${CMAKE_CURRENT_BINARY_DIR}/asiosdk.zip
        )
        file(
            REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${ASIO_SDK_VERSION}
        )
    endif()
    include_directories(deps/asiosdk/include/asiosdk/common)
endmacro(getAndIncludeASIOSDK)