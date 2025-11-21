/**
 * Test calling Zoom API directly
 */

#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/request.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_id> [cookies_file]\n";
        return 1;
    }

    std::string file_id = argv[1];
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

        // Try the API endpoint directly
        std::string api_url = "https://utec.zoom.us/nws/recording/1.0/play/info/" + file_id;

        std::cout << "Trying API endpoint: " << api_url << "\n\n";

        auto& http_client = ydl.http_client();
        ytdlp::networking::Request request(api_url);

        auto response = http_client.execute(request);

        std::cout << "Status: " << response.status() << "\n";
        std::cout << "Is success: " << response.is_success() << "\n\n";

        if (response.is_success()) {
            std::string json_text = response.read_all();
            std::cout << "Response size: " << json_text.size() << " bytes\n\n";

            try {
                auto json = nlohmann::json::parse(json_text);
                std::cout << "JSON Response:\n";
                std::cout << json.dump(2) << "\n";
            } catch (const std::exception& e) {
                std::cout << "Not JSON, raw response:\n";
                std::cout << json_text << "\n";
            }
        } else {
            std::string body = response.read_all();
            std::cout << "Error response:\n" << body << "\n";
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
