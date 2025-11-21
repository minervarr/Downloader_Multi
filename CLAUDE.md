# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## Project Overview

**yt-dlp-cpp** is a C++17 library port of yt-dlp for video extraction. Currently focused on Zoom extractor.

## Build Commands

```bash
# Standard build (uses system libraries)
mkdir -p build && cd build
cmake -DYTDLP_USE_SYSTEM_LIBS=ON ..
cmake --build .

# Build with submodules
git submodule update --init --recursive
cmake -DYTDLP_USE_SYSTEM_LIBS=OFF ..
cmake --build .

# Run tests
ctest --output-on-failure
```

## Code Organization

```
include/ytdlp/
├── core/           # YoutubeDL, InfoDict
├── extractor/      # Site extractors (Zoom works)
├── networking/     # HTTP client, cookies, requests
└── utils/          # String, JSON, filesystem utilities

src/                # Implementation (mirrors include/)
tests/unit/         # Catch2 unit tests
extern/             # Git submodules (fmt, json, spdlog, Catch2, cxxopts)
```

## Namespace

All code in `namespace ytdlp`:
- `ytdlp::core` - Core classes
- `ytdlp::extractor` - Site extractors
- `ytdlp::networking` - HTTP/cookie handling
- `ytdlp::utils` - Utilities

## Key Dependencies

**Required (system or submodule):**
- libcurl - HTTP networking
- OpenSSL - Crypto (system only)
- nlohmann/json - JSON parsing
- fmt - String formatting
- spdlog - Logging

**Testing:**
- Catch2 v3

## Adding New Code

1. Header in `include/ytdlp/<module>/`
2. Implementation in `src/<module>/`
3. Test in `tests/unit/`
4. Add to CMakeLists.txt if new file

## Testing Pattern

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("descriptive name", "[module][feature]") {
    SECTION("subsection") {
        REQUIRE(actual == expected);
    }
}
```

## Important Notes

- Focus on Zoom extractor (other extractors are WIP)
- Use `std::string_view` for function parameters
- Use `std::optional` for missing values
- Prefer system libraries on Linux
