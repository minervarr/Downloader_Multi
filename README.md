# yt-dlp-cpp

A C++17 library for video extraction, ported from [yt-dlp](https://github.com/yt-dlp/yt-dlp).

## Current Status

**Working Extractors:**
- Zoom (recordings and shares)

**In Development:**
- Core networking and utilities

## Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- Linux (primary target)

### System Dependencies

```bash
# Ubuntu/Debian
sudo apt install libcurl4-openssl-dev libssl-dev libfmt-dev libspdlog-dev nlohmann-json3-dev

# Fedora/RHEL
sudo dnf install libcurl-devel openssl-devel fmt-devel spdlog-devel json-devel

# Arch
sudo pacman -S curl openssl fmt spdlog nlohmann-json
```

## Building

### Quick Start (System Libraries)

```bash
mkdir build && cd build
cmake -DYTDLP_USE_SYSTEM_LIBS=ON ..
cmake --build .
```

### With Bundled Dependencies (Git Submodules)

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake -DYTDLP_USE_SYSTEM_LIBS=OFF ..
cmake --build .
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `YTDLP_USE_SYSTEM_LIBS` | `ON` | Use system-installed libraries |
| `BUILD_SHARED_LIBS` | `OFF` | Build shared library instead of static |
| `YTDLP_BUILD_TESTS` | `ON` | Build unit tests |
| `YTDLP_BUILD_CLI` | `ON` | Build CLI executable |

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Project Structure

```
.
├── include/ytdlp/       # Public headers
│   ├── core/            # Core classes (YoutubeDL, InfoDict)
│   ├── extractor/       # Site extractors (Zoom, etc.)
│   ├── networking/      # HTTP client, cookies
│   └── utils/           # Utility functions
├── src/                 # Implementation files
├── tests/unit/          # Catch2 unit tests
├── tools/               # Test/debug tools
└── extern/              # Git submodules (optional)
```

## Usage Example

```cpp
#include <ytdlp/extractor/zoom.hpp>
#include <ytdlp/core/youtube_dl.hpp>

int main() {
    ytdlp::core::YoutubeDL ydl;
    ytdlp::extractor::ZoomIE extractor(&ydl);

    if (extractor.suitable("https://example.zoom.us/rec/play/...")) {
        auto info = extractor.extract("https://example.zoom.us/rec/play/...");
        // Process video info...
    }
}
```

## License

This project is a port of yt-dlp and follows compatible licensing.
