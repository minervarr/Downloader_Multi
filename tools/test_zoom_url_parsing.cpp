/**
 * Minimal Zoom URL parsing test
 * Tests URL validation and ID extraction without requiring full infrastructure
 */

#include <iostream>
#include <string>
#include <regex>
#include <iomanip>

namespace zoom_test {

// URL pattern from Zoom extractor
const std::regex URL_PATTERN(
    R"((https?://(?:[^.]+\.)?zoom\.us/)rec(?:ording)?/(play|share)/([\w.-]+))",
    std::regex::icase
);

bool suitable(const std::string& url) {
    return std::regex_search(url, URL_PATTERN);
}

struct UrlComponents {
    std::string base_url;
    std::string url_type;
    std::string video_id;
    bool valid = false;
};

UrlComponents parse_url(const std::string& url) {
    UrlComponents components;
    std::smatch match;

    if (std::regex_search(url, match, URL_PATTERN)) {
        if (match.size() > 3) {
            components.base_url = match[1].str();
            components.url_type = match[2].str();
            components.video_id = match[3].str();
            components.valid = true;
        }
    }

    return components;
}

} // namespace zoom_test

void print_test_result(const std::string& test_name, bool passed) {
    std::cout << "  [" << (passed ? "✓" : "✗") << "] " << test_name << "\n";
}

int main() {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ZOOM EXTRACTOR URL PARSING TEST\n";
    std::cout << std::string(80, '=') << "\n\n";

    int passed = 0;
    int total = 0;

    // Test 1: UTEC URL (the one provided by user)
    {
        total++;
        std::string url = "https://utec.zoom.us/rec/play/k-O3Gvpp31NECNZswOUC0bEdaC7GUA6tH4jzeIUSTsF-CIyVouzeKvgYczjOWKfuvRZ6JRQuxeV34Cs.5pPB_1cFFPVdPX2Z";
        auto components = zoom_test::parse_url(url);

        bool valid = components.valid &&
                    components.base_url == "https://utec.zoom.us/" &&
                    components.url_type == "play" &&
                    components.video_id == "k-O3Gvpp31NECNZswOUC0bEdaC7GUA6tH4jzeIUSTsF-CIyVouzeKvgYczjOWKfuvRZ6JRQuxeV34Cs.5pPB_1cFFPVdPX2Z";

        print_test_result("UTEC play URL", valid);
        if (valid) {
            std::cout << "      Base URL:  " << components.base_url << "\n";
            std::cout << "      Type:      " << components.url_type << "\n";
            std::cout << "      Video ID:  " << components.video_id << "\n";
            passed++;
        }
    }

    // Test 2: economist.zoom.us play URL
    {
        total++;
        std::string url = "https://economist.zoom.us/rec/play/dUk_CNBETmZ5VA2BwEl-jjakPpJ3M1pcfVYAPRsoIbEByGsLjUZtaa4yCATQuOL3der8BlTwxQePl_j0.EImBkXzTIaPvdZO5";
        auto components = zoom_test::parse_url(url);

        bool valid = components.valid && components.url_type == "play";
        print_test_result("economist.zoom.us play URL", valid);
        if (valid) passed++;
    }

    // Test 3: Share URL
    {
        total++;
        std::string url = "https://us02web.zoom.us/rec/share/hkUk5Zxcga0nkyNGhVCRfzkA2gX_mzgS3LpTxEEWJz9Y_QpIQ4mZFOUx7KZRZDQA.9LGQBdqmDAYgiZ_8";
        auto components = zoom_test::parse_url(url);

        bool valid = components.valid && components.url_type == "share";
        print_test_result("us02web.zoom.us share URL", valid);
        if (valid) passed++;
    }

    // Test 4: recording URL (with 'recording' instead of 'rec')
    {
        total++;
        std::string url = "https://test.zoom.us/recording/play/test-video-id";
        auto components = zoom_test::parse_url(url);

        bool valid = components.valid && components.video_id == "test-video-id";
        print_test_result("recording/play URL", valid);
        if (valid) passed++;
    }

    // Test 5: Invalid URL (not Zoom)
    {
        total++;
        std::string url = "https://youtube.com/watch?v=test";
        bool valid = !zoom_test::suitable(url);
        print_test_result("Rejects non-Zoom URL", valid);
        if (valid) passed++;
    }

    // Test 6: Invalid URL (zoom.us but not recording)
    {
        total++;
        std::string url = "https://zoom.us/meeting/register";
        bool valid = !zoom_test::suitable(url);
        print_test_result("Rejects non-recording Zoom URL", valid);
        if (valid) passed++;
    }

    std::cout << "\n" << std::string(80, '-') << "\n";
    std::cout << "Results: " << passed << "/" << total << " tests passed";

    if (passed == total) {
        std::cout << " ✓\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << "\nSUCCESS: Zoom URL parsing is working correctly!\n";
        std::cout << "The extractor can properly identify and parse Zoom recording URLs.\n\n";
        return 0;
    } else {
        std::cout << " ✗\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << "\nFAILURE: Some tests failed.\n\n";
        return 1;
    }
}
