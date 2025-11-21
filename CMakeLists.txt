cmake_minimum_required(VERSION 3.15)
project(ytdlp-cpp
    VERSION 0.1.0
    DESCRIPTION "C++17 video extraction library"
    LANGUAGES CXX
)

# ==============================================================================
# Options
# ==============================================================================
option(YTDLP_USE_SYSTEM_LIBS "Use system-installed libraries instead of submodules" ON)
option(YTDLP_BUILD_TESTS "Build unit tests" ON)
option(YTDLP_BUILD_CLI "Build CLI executable" ON)
option(YTDLP_BUILD_TOOLS "Build debug/test tools" OFF)
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# ==============================================================================
# C++ Standard
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
message(STATUS "")
message(STATUS "=== Configuring Dependencies ===")

# Always required from system
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

if(YTDLP_USE_SYSTEM_LIBS)
    message(STATUS "Using system libraries")

    find_package(fmt QUIET)
    find_package(spdlog QUIET)
    find_package(nlohmann_json QUIET)

    if(NOT fmt_FOUND)
        message(FATAL_ERROR "fmt not found. Install with: sudo apt install libfmt-dev")
    endif()
    if(NOT spdlog_FOUND)
        message(FATAL_ERROR "spdlog not found. Install with: sudo apt install libspdlog-dev")
    endif()
    if(NOT nlohmann_json_FOUND)
        message(FATAL_ERROR "nlohmann_json not found. Install with: sudo apt install nlohmann-json3-dev")
    endif()

    message(STATUS "  [OK] fmt")
    message(STATUS "  [OK] spdlog")
    message(STATUS "  [OK] nlohmann_json")
else()
    message(STATUS "Using bundled libraries from extern/")

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/CMakeLists.txt")
        message(FATAL_ERROR "Submodules not initialized. Run: git submodule update --init --recursive")
    endif()

    set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/fmt EXCLUDE_FROM_ALL)

    set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/spdlog EXCLUDE_FROM_ALL)

    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/json EXCLUDE_FROM_ALL)

    message(STATUS "  [OK] fmt (bundled)")
    message(STATUS "  [OK] spdlog (bundled)")
    message(STATUS "  [OK] nlohmann_json (bundled)")
endif()

message(STATUS "  [OK] CURL (system)")
message(STATUS "  [OK] OpenSSL (system)")

# ==============================================================================
# Library Sources
# ==============================================================================
set(YTDLP_SOURCES
    src/core/youtube_dl.cpp
    src/networking/curl_http_client.cpp
    src/networking/request.cpp
    src/networking/response.cpp
    src/networking/cookie.cpp
    src/networking/cookie_jar.cpp
    src/networking/http_header_dict.cpp
    src/extractor/info_extractor.cpp
    src/extractor/zoom.cpp
    src/extractor/generic.cpp
    src/utils/string_utils.cpp
    src/utils/json_utils.cpp
    src/utils/filesystem_utils.cpp
    src/utils/network_utils.cpp
    src/utils/date_utils.cpp
    src/utils/number_utils.cpp
    src/utils/parse_utils.cpp
    src/utils/encode_utils.cpp
)

# ==============================================================================
# Static Library
# ==============================================================================
add_library(ytdlp_static STATIC ${YTDLP_SOURCES})
target_include_directories(ytdlp_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(ytdlp_static
    PUBLIC
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
set_target_properties(ytdlp_static PROPERTIES
    OUTPUT_NAME ytdlp
    POSITION_INDEPENDENT_CODE ON
)

# ==============================================================================
# Shared Library (optional)
# ==============================================================================
if(BUILD_SHARED_LIBS)
    add_library(ytdlp_shared SHARED ${YTDLP_SOURCES})
    target_include_directories(ytdlp_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(ytdlp_shared
        PUBLIC
            CURL::libcurl
            OpenSSL::SSL
            OpenSSL::Crypto
            fmt::fmt
            spdlog::spdlog
            nlohmann_json::nlohmann_json
    )
    set_target_properties(ytdlp_shared PROPERTIES
        OUTPUT_NAME ytdlp
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

add_library(ytdlp::ytdlp ALIAS ytdlp_static)

# ==============================================================================
# CLI Executable
# ==============================================================================
if(YTDLP_BUILD_CLI)
    add_executable(ytdlp-cli src/main.cpp)
    target_link_libraries(ytdlp-cli PRIVATE ytdlp_static)
    set_target_properties(ytdlp-cli PROPERTIES OUTPUT_NAME ytdlp)
endif()

# ==============================================================================
# Debug Tools (optional)
# ==============================================================================
if(YTDLP_BUILD_TOOLS)
    add_executable(test_zoom_extractor tools/test_zoom_extractor.cpp)
    target_link_libraries(test_zoom_extractor PRIVATE ytdlp_static)

    add_executable(test_zoom_debug tools/test_zoom_debug.cpp)
    target_link_libraries(test_zoom_debug PRIVATE ytdlp_static)
endif()

# ==============================================================================
# Tests
# ==============================================================================
if(YTDLP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ==============================================================================
# Summary
# ==============================================================================
message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  System libs:      ${YTDLP_USE_SYSTEM_LIBS}")
message(STATUS "  Shared lib:       ${BUILD_SHARED_LIBS}")
message(STATUS "  Build tests:      ${YTDLP_BUILD_TESTS}")
message(STATUS "  Build CLI:        ${YTDLP_BUILD_CLI}")
message(STATUS "")
