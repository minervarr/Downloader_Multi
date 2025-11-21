# ==============================================================================
# Tests Configuration
# ==============================================================================

# Find or use bundled Catch2
if(YTDLP_USE_SYSTEM_LIBS)
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "System Catch2 not found, using bundled version")
        add_subdirectory(${CMAKE_SOURCE_DIR}/extern/Catch2 ${CMAKE_BINARY_DIR}/extern/Catch2 EXCLUDE_FROM_ALL)
    endif()
else()
    add_subdirectory(${CMAKE_SOURCE_DIR}/extern/Catch2 ${CMAKE_BINARY_DIR}/extern/Catch2 EXCLUDE_FROM_ALL)
endif()

include(CTest)
include(Catch)

# ==============================================================================
# Helper function
# ==============================================================================
function(add_ytdlp_test TEST_NAME)
    add_executable(${TEST_NAME} unit/${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME}
        PRIVATE
            ytdlp_static
            Catch2::Catch2WithMain
    )
    catch_discover_tests(${TEST_NAME})
endfunction()

# ==============================================================================
# Core Tests
# ==============================================================================
add_ytdlp_test(zoom_extractor_test)
add_ytdlp_test(youtube_dl_test)
add_ytdlp_test(info_extractor_test)
add_ytdlp_test(generic_extractor_test)

# ==============================================================================
# Networking Tests
# ==============================================================================
add_ytdlp_test(http_header_dict_test)
add_ytdlp_test(request_test)
add_ytdlp_test(response_test)
add_ytdlp_test(cookie_test)
add_ytdlp_test(cookie_jar_test)
add_ytdlp_test(curl_http_client_test)

# ==============================================================================
# Utils Tests
# ==============================================================================
add_ytdlp_test(string_utils_test)
add_ytdlp_test(json_utils_test)
add_ytdlp_test(filesystem_utils_test)
add_ytdlp_test(network_utils_test)
add_ytdlp_test(date_utils_test)
add_ytdlp_test(number_utils_test)
add_ytdlp_test(parse_utils_test)
add_ytdlp_test(encode_utils_test)
