#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/cookie_jar.hpp"
#include <iostream>
#include <memory>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>

using namespace ytdlp;

/**
 * Download a file from URL to output path using streaming.
 */
bool download_file(const std::string& url, const std::string& output_path,
                   networking::CurlHttpClient& http_client) {
    fmt::print("Downloading from: {}\n", url);
    fmt::print("Saving to: {}\n", output_path);

    // Create headers for Zoom-compatible download
    std::map<std::string, std::string> headers = {
        {"User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"},
        {"Accept", "*/*"},
        {"Accept-Language", "en-US,en;q=0.9"},
        {"Referer", "https://zoom.us/"}
    };

    // Progress tracking
    auto start_time = std::chrono::steady_clock::now();
    int64_t last_bytes = 0;
    auto last_update = start_time;

    // Progress callback - shows download progress
    auto progress_callback = [&](int64_t downloaded, int64_t total) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();

        // Update every 500ms to avoid spamming
        if (elapsed > 500 || downloaded == total) {
            double speed = 0.0;
            if (elapsed > 0) {
                speed = (downloaded - last_bytes) / (elapsed / 1000.0);
            }

            if (total > 0) {
                double percent = (100.0 * downloaded) / total;
                double total_mb = total / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);

                int64_t remaining = total - downloaded;
                int eta_seconds = (speed > 0) ? static_cast<int>(remaining / speed) : 0;

                fmt::print("\r[download] {:.1f}% of {:.2f}MB at {:.2f}MB/s ETA {:02d}:{:02d}",
                          percent, total_mb, speed_mb, eta_seconds / 60, eta_seconds % 60);
                std::fflush(stdout);
            } else {
                double downloaded_mb = downloaded / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);
                fmt::print("\r[download] {:.2f}MB at {:.2f}MB/s", downloaded_mb, speed_mb);
                std::fflush(stdout);
            }

            last_bytes = downloaded;
            last_update = now;
        }
    };

    try {
        bool success = http_client.download_to_file(url, output_path, headers, progress_callback);

        if (success) {
            fmt::print("\nDownload complete!\n");
            return true;
        } else {
            fmt::print("\n");
            return false;
        }

    } catch (const std::exception& e) {
        fmt::print("\nDownload failed: {}\n", e.what());
        return false;
    }
}

void print_usage(const char* program_name) {
    fmt::print("Usage: {} [options] <url>\n", program_name);
    fmt::print("\nOptions:\n");
    fmt::print("  -o, --output <file>    Output filename (default: auto-generated)\n");
    fmt::print("  -c, --cookies <file>   Netscape cookie file for authentication\n");
    fmt::print("  -i, --info             Print video info only (don't download)\n");
    fmt::print("  -q, --quiet            Quiet mode\n");
    fmt::print("  -h, --help             Show this help\n");
    fmt::print("\nExamples:\n");
    fmt::print("  {} https://zoom.us/rec/play/xxx\n", program_name);
    fmt::print("  {} -c cookies.txt -o meeting.mp4 https://zoom.us/rec/play/xxx\n", program_name);
}

int main(int argc, char** argv) {
    try {
        // Simple argument parsing
        std::string url;
        std::string output;
        std::string cookie_file;
        bool quiet = false;
        bool info_only = false;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_usage(argv[0]);
                return 0;
            } else if (arg == "-q" || arg == "--quiet") {
                quiet = true;
            } else if (arg == "-i" || arg == "--info") {
                info_only = true;
            } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                output = argv[++i];
            } else if ((arg == "-c" || arg == "--cookies") && i + 1 < argc) {
                cookie_file = argv[++i];
            } else if (arg[0] != '-') {
                url = arg;
            }
        }

        if (url.empty()) {
            print_usage(argv[0]);
            return 1;
        }

        if (!quiet) {
            fmt::print("yt-dlp-cpp - Zoom Video Downloader (C++ Port)\n");
            fmt::print("=============================================\n\n");
        }

        // Initialize YoutubeDL
        core::YoutubeDLParams params;
        params.quiet = quiet;
        core::YoutubeDL ydl(params);

        // Load cookies if provided
        if (!cookie_file.empty()) {
            if (!quiet) {
                fmt::print("Loading cookies from: {}\n", cookie_file);
            }
            auto cookie_jar = std::make_shared<networking::CookieJar>();
            cookie_jar->load(cookie_file);
            ydl.http_client().set_cookie_jar(cookie_jar);
            if (!quiet) {
                fmt::print("Loaded {} cookies\n", cookie_jar->size());
            }
        }

        // Create Zoom extractor
        extractor::ZoomIE zoom(&ydl);

        // Check if URL is supported
        if (!zoom.suitable(url)) {
            fmt::print(stderr, "Error: URL is not a valid Zoom recording URL\n");
            fmt::print(stderr, "Supported patterns:\n");
            fmt::print(stderr, "  - https://zoom.us/rec/play/...\n");
            fmt::print(stderr, "  - https://zoom.us/rec/share/...\n");
            fmt::print(stderr, "  - https://*.zoom.us/rec/play/...\n");
            return 1;
        }

        if (!quiet) {
            fmt::print("Extracting video info...\n");
        }

        // Extract video information
        core::InfoDict info = zoom.extract(url);

        if (!quiet || info_only) {
            fmt::print("\nVideo Information:\n");
            fmt::print("------------------\n");

            if (info.contains("title")) {
                fmt::print("Title: {}\n", info["title"].get<std::string>());
            }
            if (info.contains("id")) {
                fmt::print("ID: {}\n", info["id"].get<std::string>());
            }
            if (info.contains("duration")) {
                int duration = info["duration"].get<int>();
                fmt::print("Duration: {}:{:02d}\n", duration / 60, duration % 60);
            }

            if (info.contains("formats") && info["formats"].is_array()) {
                fmt::print("\nAvailable formats: {}\n", info["formats"].size());

                auto formats = info["formats"].get<std::vector<core::InfoDict>>();
                for (size_t i = 0; i < formats.size() && i < 10; i++) {
                    const auto& fmt = formats[i];
                    fmt::print("  Format #{}: ", i+1);
                    if (fmt.contains("format_id")) {
                        fmt::print("{} ", fmt["format_id"].get<std::string>());
                    }
                    if (fmt.contains("ext")) {
                        fmt::print("{} ", fmt["ext"].get<std::string>());
                    }
                    if (fmt.contains("height")) {
                        fmt::print("{}p ", fmt["height"].get<int>());
                    }
                    fmt::print("\n");
                }
            }
        }

        if (info_only) {
            return 0;
        }

        // Get download URL
        std::string download_url;
        if (info.contains("url") && info["url"].is_string()) {
            download_url = info["url"].get<std::string>();
        } else if (info.contains("formats") && info["formats"].is_array()) {
            auto formats = info["formats"].get<std::vector<core::InfoDict>>();
            for (const auto& fmt : formats) {
                if (fmt.contains("url") && fmt["url"].is_string()) {
                    download_url = fmt["url"].get<std::string>();
                    break;
                }
            }
        }

        if (download_url.empty()) {
            fmt::print(stderr, "Error: No download URL found\n");
            return 1;
        }

        // Generate output filename if not specified
        if (output.empty()) {
            if (info.contains("title") && info["title"].is_string()) {
                output = info["title"].get<std::string>() + ".mp4";
            } else if (info.contains("id") && info["id"].is_string()) {
                output = info["id"].get<std::string>() + ".mp4";
            } else {
                output = "zoom_recording.mp4";
            }
            // Sanitize filename
            for (char& c : output) {
                if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                    c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                    c = '_';
                }
            }
        }

        if (!quiet) {
            fmt::print("\nDownloading to: {}\n\n", output);
        }

        // Download the video
        bool success = download_file(download_url, output, ydl.http_client());

        if (success) {
            if (!quiet) {
                fmt::print("\n✓ Download successful!\n");
                fmt::print("  Saved to: {}\n", output);
            }
            return 0;
        } else {
            fmt::print(stderr, "\n✗ Download failed\n");
            return 1;
        }

    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}
