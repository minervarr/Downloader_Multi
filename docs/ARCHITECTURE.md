# yt-dlp-cpp Architecture

This document describes the architectural design, patterns, and decisions for the yt-dlp C++ port.

**Last Updated:** 2025-10-23
**Status:** Phase 1 - Foundation

---

## Table of Contents

1. [Design Philosophy](#design-philosophy)
2. [Architecture Overview](#architecture-overview)
3. [Module Design](#module-design)
4. [Data Structures](#data-structures)
5. [Error Handling](#error-handling)
6. [Memory Management](#memory-management)
7. [Concurrency Model](#concurrency-model)
8. [Type System Design](#type-system-design)
9. [Plugin Architecture](#plugin-architecture)
10. [Testing Strategy](#testing-strategy)

---

## Design Philosophy

### Core Principles

1. **Performance First**: Primary goal is to achieve 2-5x performance improvement over Python
2. **Type Safety**: Use C++17's strong typing to catch errors at compile-time
3. **Modern C++**: Embrace C++17 features (optional, variant, string_view, filesystem)
4. **Zero-Copy Where Possible**: Minimize data copying using string_view and move semantics
5. **Debuggability**: Clear error messages, meaningful types, structured logging
6. **Maintainability**: Clean separation of concerns, modular design
7. **Testability**: Every component must be unit-testable

### Non-Goals

- Full Python API compatibility (we're optimizing for C++)
- GUI support (CLI only, like original yt-dlp)
- Python module interoperability
- Support for Python plugins

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         CLI Layer                            │
│                  (Command-line Interface)                    │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────┐
│                     Core Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  YoutubeDL   │  │   Options    │  │  InfoDict    │     │
│  │ (orchestrator)│  │  (config)    │  │  (results)   │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└───────────┬─────────────┬─────────────┬───────────────────┘
            │             │             │
┌───────────▼─────┐  ┌───▼──────────┐  ┌▼──────────────────┐
│   Extractors     │  │  Downloaders  │  │ Post-processors  │
│                  │  │               │  │                  │
│ • YouTube        │  │ • HTTP        │  │ • FFmpeg merge   │
│ • Vimeo          │  │ • HLS         │  │ • Thumbnails     │
│ • Generic        │  │ • DASH        │  │ • Metadata       │
│ • 500+ sites     │  │ • Fragments   │  │ • SponsorBlock   │
└───────────┬──────┘  └───┬───────────┘  └───────┬──────────┘
            │             │                       │
┌───────────▼─────────────▼───────────────────────▼──────────┐
│                    Utility Layer                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Networking│  │  Parsing │  │  String  │  │   File   │  │
│  │  (HTTP,   │  │ (JSON,   │  │  Utils   │  │  System  │  │
│  │  cookies) │  │  HTML)   │  │          │  │          │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Dependency Graph

```
CLI
 └─> Core::YoutubeDL
      ├─> Core::Options
      ├─> Core::InfoDict
      ├─> Extractors::InfoExtractor (base)
      │    └─> Extractors::* (YouTube, Vimeo, etc.)
      ├─> Downloaders::DownloaderBase
      │    └─> Downloaders::* (HTTP, HLS, DASH)
      ├─> PostProcessors::PostProcessorBase
      │    └─> PostProcessors::* (FFmpeg, Thumbnail)
      └─> Utils::* (Networking, Parsing, String, File)
```

---

## Module Design

### 1. Core Module (`core/`)

**Responsibility:** Main orchestration and configuration

**Key Classes:**
- `YoutubeDL`: Main orchestrator class (like Python's YoutubeDL)
- `Options`: Configuration and command-line options
- `InfoDict`: Video metadata and format information

**Design Pattern:** Facade pattern (YoutubeDL provides unified interface)

### 2. Extractor Module (`extractor/`)

**Responsibility:** Extract video information from websites

**Key Classes:**
- `InfoExtractor`: Abstract base class for all extractors
- `GenericExtractor`: Handles common video embeds
- `YoutubeExtractor`: YouTube-specific extraction
- 500+ site-specific extractors

**Design Pattern:**
- Template Method (InfoExtractor defines algorithm, subclasses implement steps)
- Factory (ExtractorFactory registers and creates extractors)
- Strategy (different extraction strategies for different sites)

### 3. Downloader Module (`downloader/`)

**Responsibility:** Download videos using various protocols

**Key Classes:**
- `DownloaderBase`: Abstract base for all downloaders
- `HttpDownloader`: Standard HTTP downloads
- `HlsDownloader`: HLS streaming protocol
- `DashDownloader`: DASH streaming protocol
- `FragmentDownloader`: Fragment-based downloads

**Design Pattern:**
- Strategy (different download strategies)
- Observer (progress callbacks)
- Chain of Responsibility (fallback downloaders)

### 4. Post-processor Module (`postprocessor/`)

**Responsibility:** Process downloaded files

**Key Classes:**
- `PostProcessorBase`: Abstract base
- `FFmpegPostProcessor`: FFmpeg integration
- `ThumbnailEmbedder`: Embed thumbnails
- `MetadataWriter`: Write metadata tags

**Design Pattern:**
- Chain of Responsibility (multiple post-processors in sequence)
- Command (encapsulate processing operations)

### 5. Networking Module (`networking/`)

**Responsibility:** HTTP communication and cookie management

**Key Classes:**
- `HttpClient`: HTTP/HTTPS requests (wraps libcurl)
- `Request`: HTTP request representation
- `Response`: HTTP response representation
- `CookieJar`: Cookie storage and management

**Design Pattern:**
- Builder (for constructing requests)
- Singleton (for shared cookie jar)

### 6. Utils Module (`utils/`)

**Responsibility:** Utility functions used across modules

**Components:**
- `string_utils`: URL encoding, sanitization, parsing
- `filesystem_utils`: Path handling, file operations
- `json_utils`: JSON parsing helpers
- `regex_utils`: Compiled regex caching

**Design Pattern:** Utility functions (no state)

---

## Data Structures

### InfoDict - Video Metadata

```cpp
namespace ytdlp::core {

struct Format {
    std::string format_id;
    std::string ext;                     // File extension
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int> fps;
    std::optional<int> vcodec;
    std::optional<int> acodec;
    std::optional<int> abr;              // Audio bitrate
    std::optional<int> vbr;              // Video bitrate
    std::optional<int64_t> filesize;
    std::string url;
    std::string protocol;
    std::optional<std::string> format_note;
    float quality;                       // Computed quality score
};

struct InfoDict {
    // Essential fields
    std::string id;
    std::string title;
    std::string url;
    std::string extractor;
    std::string webpage_url;

    // Optional metadata
    std::optional<std::string> description;
    std::optional<std::string> uploader;
    std::optional<std::string> uploader_id;
    std::optional<std::string> uploader_url;
    std::optional<int> duration;         // seconds
    std::optional<int> view_count;
    std::optional<int> like_count;
    std::optional<std::string> upload_date; // YYYYMMDD

    // Formats
    std::vector<Format> formats;
    std::optional<Format> requested_format;

    // Thumbnails
    struct Thumbnail {
        std::string url;
        std::optional<int> width;
        std::optional<int> height;
        std::optional<std::string> id;
    };
    std::vector<Thumbnail> thumbnails;

    // Subtitles
    std::map<std::string, std::vector<SubtitleFormat>> subtitles;

    // Playlist info (if applicable)
    std::optional<std::string> playlist;
    std::optional<int> playlist_index;
    std::optional<int> n_entries;

    // Additional metadata
    std::map<std::string, std::any> extra;  // For site-specific fields
};

} // namespace ytdlp::core
```

### Options - Configuration

```cpp
namespace ytdlp::core {

struct Options {
    // URLs
    std::vector<std::string> urls;

    // Format selection
    std::optional<std::string> format;
    bool prefer_free_formats{false};

    // Output
    std::string output_template{"%(title)s-%(id)s.%(ext)s"};
    std::optional<std::string> output_directory;
    bool overwrite{false};
    bool continue_dl{true};

    // Download
    int max_downloads{-1};
    int retries{10};
    int fragment_retries{10};
    std::optional<int> sleep_interval;
    std::optional<int> max_sleep_interval;

    // Network
    std::optional<std::string> proxy;
    std::optional<int> socket_timeout;
    std::optional<std::string> source_address;
    bool use_cookies{true};
    std::optional<std::string> cookies_from_browser;

    // Authentication
    std::optional<std::string> username;
    std::optional<std::string> password;
    std::optional<std::string> netrc;

    // Post-processing
    bool extract_audio{false};
    std::optional<std::string> audio_format;
    std::optional<int> audio_quality;
    bool embed_thumbnail{false};
    bool embed_metadata{true};
    bool embed_subs{false};
    std::optional<std::string> ffmpeg_location;

    // Logging
    enum class LogLevel { QUIET, ERROR, WARNING, INFO, DEBUG };
    LogLevel log_level{LogLevel::INFO};
    bool progress{true};

    // Extractor options
    std::map<std::string, std::any> extractor_args;

    // Hooks (callbacks)
    using ProgressHook = std::function<void(const ProgressInfo&)>;
    std::vector<ProgressHook> progress_hooks;

    using PostHook = std::function<void(const InfoDict&)>;
    std::vector<PostHook> post_hooks;
};

} // namespace ytdlp::core
```

---

## Error Handling

### Strategy: Hybrid Approach

1. **Exceptions for Rare Errors**: Network failures, file I/O errors, invalid configuration
2. **std::optional for Expected Failures**: Missing metadata, optional fields
3. **std::expected for Operations with Expected Errors** (C++23 or use library)

### Custom Exception Hierarchy

```cpp
namespace ytdlp {

class YtdlpException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Network errors
class NetworkException : public YtdlpException {
    using YtdlpException::YtdlpException;
};

class HttpException : public NetworkException {
public:
    HttpException(int status_code, std::string message)
        : NetworkException(std::move(message))
        , status_code_(status_code) {}

    int status_code() const { return status_code_; }
private:
    int status_code_;
};

// Extraction errors
class ExtractionException : public YtdlpException {
    using YtdlpException::YtdlpException;
};

class UnsupportedSiteException : public ExtractionException {
    using ExtractionException::ExtractionException;
};

class VideoUnavailableException : public ExtractionException {
    using ExtractionException::ExtractionException;
};

// Download errors
class DownloadException : public YtdlpException {
    using YtdlpException::YtdlpException;
};

// Post-processing errors
class PostProcessingException : public YtdlpException {
    using YtdlpException::YtdlpException;
};

} // namespace ytdlp
```

### Error Handling Guidelines

```cpp
// 1. Use exceptions for rare, exceptional errors
try {
    auto response = http_client.get(url);
    if (response.status_code() != 200) {
        throw HttpException(response.status_code(), "Failed to fetch page");
    }
} catch (const NetworkException& e) {
    spdlog::error("Network error: {}", e.what());
    return std::nullopt;
}

// 2. Use std::optional for expected missing values
std::optional<std::string> extract_title(const std::string& html) {
    // Try to find title
    if (auto match = regex_search(html, title_pattern)) {
        return match->str();
    }
    return std::nullopt;  // Title not found (expected case)
}

// 3. Use error codes for performance-critical paths
enum class ParseResult {
    SUCCESS,
    INVALID_FORMAT,
    MISSING_FIELD,
    PARSE_ERROR
};

ParseResult parse_format(const json& j, Format& out) {
    if (!j.contains("url")) return ParseResult::MISSING_FIELD;
    // ...
    return ParseResult::SUCCESS;
}
```

---

## Memory Management

### Principles

1. **RAII**: Resource Acquisition Is Initialization
2. **Smart Pointers**: Use std::unique_ptr and std::shared_ptr
3. **Value Semantics**: Prefer passing by value with move semantics
4. **Avoid Raw Pointers**: Only use raw pointers for non-owning references

### Smart Pointer Guidelines

```cpp
// Unique ownership
std::unique_ptr<Extractor> create_extractor(const std::string& url);

// Shared ownership (rare, avoid if possible)
std::shared_ptr<CookieJar> get_cookie_jar();

// Non-owning reference (prefer references over raw pointers)
void process_info(const InfoDict& info);  // Good
void process_info(InfoDict* info);        // Avoid
void process_info(const InfoDict* info);  // Avoid
```

### String Handling

```cpp
// Use std::string_view for read-only string operations (zero-copy)
std::string_view extract_domain(std::string_view url);

// Use std::string for owned strings
std::string sanitize_filename(std::string_view filename);

// Move strings when transferring ownership
InfoDict extract_info() {
    InfoDict info;
    info.title = std::move(expensive_title);  // No copy
    return info;  // RVO (Return Value Optimization)
}
```

---

## Concurrency Model

### Thread Safety Requirements

1. **HttpClient**: Thread-safe (uses libcurl multi-handle)
2. **CookieJar**: Thread-safe (mutex-protected)
3. **Extractors**: Not thread-safe individually, but multiple instances can run in parallel
4. **Downloaders**: Each download in separate thread, progress reporting synchronized

### Parallel Download Strategy

```cpp
class FragmentDownloader {
public:
    void download_fragments(const std::vector<Fragment>& fragments) {
        // Thread pool for parallel downloads
        ThreadPool pool(std::thread::hardware_concurrency());

        std::vector<std::future<DownloadResult>> futures;
        for (const auto& fragment : fragments) {
            futures.push_back(pool.enqueue([&fragment, this]() {
                return download_fragment(fragment);
            }));
        }

        // Wait for all downloads, report progress
        for (auto& future : futures) {
            auto result = future.get();
            report_progress(result);
        }
    }
};
```

### Synchronization Primitives

- **std::mutex**: For protecting shared state (CookieJar, progress)
- **std::shared_mutex**: For read-heavy operations (C++17)
- **std::atomic**: For lock-free counters (download progress)
- **std::future/std::promise**: For async operations

---

## Type System Design

### Python's Dynamic Typing → C++ Static Typing

**Challenge:** Python code uses dynamic dicts, we need static types.

**Solution:** Hybrid approach

```cpp
// Approach 1: Strongly-typed structs (preferred)
struct InfoDict {
    std::string title;
    std::optional<int> duration;
    std::vector<Format> formats;
    // ... all known fields

    // Escape hatch for unknown fields
    std::map<std::string, std::any> extra;
};

// Approach 2: Type-erased map (for flexibility)
using DynamicDict = std::map<std::string, std::any>;

// Helper to safely extract values
template<typename T>
std::optional<T> get_optional(const DynamicDict& dict, const std::string& key) {
    auto it = dict.find(key);
    if (it == dict.end()) return std::nullopt;
    try {
        return std::any_cast<T>(it->second);
    } catch (const std::bad_any_cast&) {
        return std::nullopt;
    }
}
```

---

## Plugin Architecture

### Extractor Registration

```cpp
// Auto-registration pattern
class ExtractorFactory {
public:
    static ExtractorFactory& instance() {
        static ExtractorFactory factory;
        return factory;
    }

    void register_extractor(
        const std::string& name,
        std::function<std::unique_ptr<InfoExtractor>()> creator
    ) {
        extractors_[name] = std::move(creator);
    }

    std::unique_ptr<InfoExtractor> create(const std::string& url) {
        // Match URL pattern to extractor
        for (const auto& [name, creator] : extractors_) {
            if (can_handle(name, url)) {
                return creator();
            }
        }
        return nullptr;  // No matching extractor
    }

private:
    std::map<std::string, std::function<std::unique_ptr<InfoExtractor>()>> extractors_;
};

// Auto-register helper
template<typename ExtractorType>
struct ExtractorRegistrar {
    ExtractorRegistrar(const std::string& name) {
        ExtractorFactory::instance().register_extractor(
            name,
            []() { return std::make_unique<ExtractorType>(); }
        );
    }
};

// Usage in extractor implementation
static ExtractorRegistrar<YoutubeExtractor> youtube_registrar("youtube");
```

---

## Testing Strategy

### Unit Tests (Catch2)

```cpp
TEST_CASE("URL encoding works correctly", "[utils][string]") {
    SECTION("spaces are encoded") {
        REQUIRE(url_encode("hello world") == "hello%20world");
    }

    SECTION("special characters are encoded") {
        REQUIRE(url_encode("foo&bar=baz") == "foo%26bar%3Dbaz");
    }

    SECTION("safe characters are not encoded") {
        REQUIRE(url_encode("abc123-_.~") == "abc123-_.~");
    }
}
```

### Integration Tests

```cpp
TEST_CASE("YouTube extraction works", "[integration][youtube]") {
    // Use recorded HTTP responses (VCR pattern)
    MockHttpClient client("fixtures/youtube_response.json");

    YoutubeExtractor extractor;
    extractor.set_http_client(&client);

    auto info = extractor.extract("https://www.youtube.com/watch?v=dQw4w9WgXcQ");

    REQUIRE(info.has_value());
    REQUIRE(info->title == "Rick Astley - Never Gonna Give You Up");
    REQUIRE(info->formats.size() > 0);
}
```

### Performance Tests

```cpp
TEST_CASE("URL encoding performance", "[benchmark][utils]") {
    std::string input = generate_random_string(1000000);

    BENCHMARK("url_encode 1MB string") {
        return url_encode(input);
    };
}
```

---

## Future Considerations

### Phase 2+ Architecture Refinements

1. **Caching Layer**: Cache webpage responses, regex compilations
2. **Plugin System**: Dynamic loading of extractors (dlopen/LoadLibrary)
3. **Configuration System**: YAML/TOML config files
4. **Logging Infrastructure**: Structured logging with context
5. **Metrics/Telemetry**: Performance monitoring
6. **Internationalization**: Multi-language support

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)
- [C++17 STL Cookbook](https://www.packtpub.com/product/c-17-stl-cookbook/9781787120495)

---

**Document Status:** Living document, will be updated as architecture evolves through development phases.
