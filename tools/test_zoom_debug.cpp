/**
 * Debug version - saves webpage to file
 */

#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/request.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <zoom_url> [cookies_file]\n";
        return 1;
    }

    std::string url = argv[1];
    std::string cookies_file = (argc > 2) ? argv[2] : "";

    try {
        // Create YoutubeDL instance
        ytdlp::core::YoutubeDLParams params;
        params.quiet = false;
        ytdlp::core::YoutubeDL ydl(params);

        // Load cookies if provided
        if (!cookies_file.empty()) {
            std::cout << "Loading cookies from: " << cookies_file << "\n";
            ydl.load_cookies(cookies_file);
        }

        // Download webpage directly
        auto& http_client = ydl.http_client();
        ytdlp::networking::Request request(url);

        std::cout << "Downloading webpage...\n";
        auto response = http_client.execute(request);

        std::cout << "Status: " << response.status() << "\n";
        std::cout << "Is success: " << response.is_success() << "\n";

        std::string webpage = response.read_all();
        std::cout << "Downloaded " << webpage.size() << " bytes\n";

        // Save to file
        std::ofstream out("zoom_webpage.html");
        out << webpage;
        out.close();

        std::cout << "\nWebpage saved to: zoom_webpage.html\n";

        // Search for window.__data__
        size_t pos = webpage.find("window.__data__");
        if (pos != std::string::npos) {
            std::cout << "\nFound 'window.__data__' at position: " << pos << "\n";
            std::cout << "Context:\n";
            size_t start = (pos > 100) ? pos - 100 : 0;
            size_t end = std::min(pos + 500, webpage.size());
            std::cout << webpage.substr(start, end - start) << "\n";
        } else {
            std::cout << "\n'window.__data__' NOT found in webpage!\n";

            // Check for other common patterns
            if (webpage.find("__NEXT_DATA__") != std::string::npos) {
                std::cout << "Found '__NEXT_DATA__' instead\n";
            }
            if (webpage.find("password") != std::string::npos) {
                std::cout << "Found 'password' - might need authentication\n";
            }

            // Show first 2000 chars
            std::cout << "\nFirst 2000 characters:\n";
            std::cout << webpage.substr(0, std::min<size_t>(2000, webpage.size())) << "\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
