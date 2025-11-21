/**
 * Test program for Zoom extractor
 *
 * Usage:
 *   ./test_zoom_extractor <zoom_url> [cookies_file]
 *
 * Example:
 *   ./test_zoom_extractor "https://utec.zoom.us/rec/play/k-O3Gvpp31NECNZswOUC0bEdaC7GUA6tH4jzeIUSTsF-CIyVouzeKvgYczjOWKfuvRZ6JRQuxeV34Cs.5pPB_1cFFPVdPX2Z" test_cookies.txt
 */

#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/cookie_jar.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>

void print_info_dict(const ytdlp::core::InfoDict& info) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ZOOM RECORDING INFORMATION\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Print basic info
    if (info.contains("id")) {
        std::cout << "Video ID:    " << info["id"] << "\n";
    }
    if (info.contains("title")) {
        std::cout << "Title:       " << info["title"] << "\n";
    }
    if (info.contains("duration")) {
        std::cout << "Duration:    " << info["duration"] << " seconds\n";
    }

    // Print formats
    if (info.contains("formats") && info["formats"].is_array()) {
        std::cout << "\nAvailable Formats:\n";
        std::cout << std::string(80, '-') << "\n";

        int i = 1;
        for (const auto& format : info["formats"]) {
            std::cout << "Format #" << i++ << ":\n";

            if (format.contains("format_id")) {
                std::cout << "  ID:          " << format["format_id"] << "\n";
            }
            if (format.contains("format_note")) {
                std::cout << "  Note:        " << format["format_note"] << "\n";
            }
            if (format.contains("ext")) {
                std::cout << "  Extension:   " << format["ext"] << "\n";
            }
            if (format.contains("width") && format.contains("height")) {
                std::cout << "  Resolution:  " << format["width"] << "x" << format["height"] << "\n";
            }
            if (format.contains("filesize_approx")) {
                double size_mb = format["filesize_approx"].get<int64_t>() / (1024.0 * 1024.0);
                std::cout << "  File Size:   " << std::fixed << std::setprecision(2) << size_mb << " MB\n";
            }
            if (format.contains("url")) {
                std::string url = format["url"];
                if (url.length() > 80) {
                    url = url.substr(0, 77) + "...";
                }
                std::cout << "  URL:         " << url << "\n";
            }
            std::cout << "\n";
        }
    }

    // Print subtitles
    if (info.contains("subtitles") && info["subtitles"].is_object()) {
        std::cout << "Available Subtitles:\n";
        std::cout << std::string(80, '-') << "\n";

        for (auto& [lang, subs] : info["subtitles"].items()) {
            std::cout << "  " << lang << ": ";
            if (subs.is_array() && !subs.empty()) {
                std::cout << subs[0]["url"] << "\n";
            }
        }
        std::cout << "\n";
    }

    // Print raw JSON for debugging
    std::cout << "\nRaw JSON:\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << info.dump(2) << "\n";

    std::cout << std::string(80, '=') << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <zoom_url> [cookies_file]\n";
        std::cerr << "\nExample:\n";
        std::cerr << "  " << argv[0] << " \"https://utec.zoom.us/rec/play/VIDEO_ID\" test_cookies.txt\n";
        return 1;
    }

    std::string url = argv[1];
    std::string cookies_file = (argc > 2) ? argv[2] : "";

    std::cout << "Testing Zoom Extractor\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "URL: " << url << "\n";
    if (!cookies_file.empty()) {
        std::cout << "Cookies: " << cookies_file << "\n";
    }
    std::cout << std::string(80, '=') << "\n\n";

    try {
        // Check if URL is suitable
        if (!ytdlp::extractor::ZoomIE::suitable(url)) {
            std::cerr << "ERROR: URL is not a valid Zoom recording URL\n";
            std::cerr << "Expected format: https://DOMAIN.zoom.us/rec/play/VIDEO_ID\n";
            std::cerr << "             or: https://DOMAIN.zoom.us/rec/share/VIDEO_ID\n";
            return 1;
        }

        std::cout << "✓ URL is valid Zoom recording URL\n";

        // Extract video ID
        std::string video_id = ytdlp::extractor::ZoomIE::extract_id(url);
        std::cout << "✓ Extracted video ID: " << video_id << "\n\n";

        // Create YoutubeDL instance
        ytdlp::core::YoutubeDLParams params;
        params.quiet = false;

        ytdlp::core::YoutubeDL ydl(params);

        // Load cookies if provided
        if (!cookies_file.empty()) {
            std::cout << "Loading cookies from: " << cookies_file << "\n";
            try {
                ydl.load_cookies(cookies_file);
                std::cout << "✓ Cookies loaded\n\n";
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load cookies: " << e.what() << "\n\n";
            }
        }

        // Create Zoom extractor
        ytdlp::extractor::ZoomIE extractor(&ydl);

        std::cout << "Starting extraction...\n\n";

        // Extract video info
        auto info = extractor.extract(url);

        // Print results
        print_info_dict(info);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n" << std::string(80, '=') << "\n";
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << std::string(80, '=') << "\n";
        return 1;
    }
}
