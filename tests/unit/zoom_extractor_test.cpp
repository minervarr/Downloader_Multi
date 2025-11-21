#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("ZoomIE construction", "[extractor][zoom]") {
    ZoomIE extractor;

    REQUIRE(extractor.ie_key() == "Zoom");
    REQUIRE(extractor.ie_name() == "zoom");
}

TEST_CASE("ZoomIE URL pattern matching", "[extractor][zoom]") {
    SECTION("matches play URL with economist domain") {
        REQUIRE(ZoomIE::suitable("https://economist.zoom.us/rec/play/dUk_CNBETmZ5VA2BwEl-jjakPpJ3M1pcfVYAPRsoIbEByGsLjUZtaa4yCATQuOL3der8BlTwxQePl_j0.EImBkXzTIaPvdZO5"));
    }

    SECTION("matches play URL with ffgolf domain") {
        REQUIRE(ZoomIE::suitable("https://ffgolf.zoom.us/rec/play/qhEhXbrxq1Zoucx8CMtHzq1Z_2YZRPVCqWK_K-2FkEGRsSLDeOX8Tu4P6jtjZcRry8QhIbvKZdtr4UNo.QcPn2debFskI9whJ"));
    }

    SECTION("matches share URL with us02web domain") {
        REQUIRE(ZoomIE::suitable("https://us02web.zoom.us/rec/share/hkUk5Zxcga0nkyNGhVCRfzkA2gX_mzgS3LpTxEEWJz9Y_QpIQ4mZFOUx7KZRZDQA.9LGQBdqmDAYgiZ_8"));
    }

    SECTION("matches share URL with cityofdetroit domain") {
        REQUIRE(ZoomIE::suitable("https://cityofdetroit.zoom.us/rec/share/VjE-5kW3xmgbEYqR5KzRgZ1OFZvtMtiXk5HyRJo5kK4m5PYE6RF4rF_oiiO_9qaM.UTAg1MI7JSnF3ZjX"));
    }

    SECTION("matches recording URL with 'recording' instead of 'rec'") {
        REQUIRE(ZoomIE::suitable("https://example.zoom.us/recording/play/test-video-id"));
        REQUIRE(ZoomIE::suitable("https://example.zoom.us/recording/share/another-id"));
    }

    SECTION("does not match invalid URLs") {
        REQUIRE_FALSE(ZoomIE::suitable("https://youtube.com/watch?v=test"));
        REQUIRE_FALSE(ZoomIE::suitable("https://zoom.us/meeting/test"));
        REQUIRE_FALSE(ZoomIE::suitable("https://zoom.us/download"));
    }
}

TEST_CASE("ZoomIE video ID extraction", "[extractor][zoom]") {
    SECTION("extracts ID from play URL") {
        std::string video_id = ZoomIE::extract_id(
            "https://economist.zoom.us/rec/play/dUk_CNBETmZ5VA2BwEl-jjakPpJ3M1pcfVYAPRsoIbEByGsLjUZtaa4yCATQuOL3der8BlTwxQePl_j0.EImBkXzTIaPvdZO5"
        );
        REQUIRE(video_id == "dUk_CNBETmZ5VA2BwEl-jjakPpJ3M1pcfVYAPRsoIbEByGsLjUZtaa4yCATQuOL3der8BlTwxQePl_j0.EImBkXzTIaPvdZO5");
    }

    SECTION("extracts ID from share URL") {
        std::string video_id = ZoomIE::extract_id(
            "https://us02web.zoom.us/rec/share/hkUk5Zxcga0nkyNGhVCRfzkA2gX_mzgS3LpTxEEWJz9Y_QpIQ4mZFOUx7KZRZDQA.9LGQBdqmDAYgiZ_8"
        );
        REQUIRE(video_id == "hkUk5Zxcga0nkyNGhVCRfzkA2gX_mzgS3LpTxEEWJz9Y_QpIQ4mZFOUx7KZRZDQA.9LGQBdqmDAYgiZ_8");
    }

    SECTION("extracts ID with dots and hyphens") {
        std::string video_id = ZoomIE::extract_id(
            "https://test.zoom.us/rec/play/test-id_123.456-abc"
        );
        REQUIRE(video_id == "test-id_123.456-abc");
    }
}

TEST_CASE("ZoomIE with YoutubeDL integration", "[extractor][zoom]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        ZoomIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Zoom");
    }
}

TEST_CASE("ZoomIE URL parsing", "[extractor][zoom]") {
    SECTION("parses play URL components") {
        // This tests internal functionality
        // In a real scenario, these would be tested through _real_extract
        std::string url = "https://economist.zoom.us/rec/play/test-video-id";

        REQUIRE(ZoomIE::suitable(url));

        auto video_id = ZoomIE::extract_id(url);
        REQUIRE(video_id == "test-video-id");
    }

    SECTION("parses share URL components") {
        std::string url = "https://cityofdetroit.zoom.us/rec/share/test-share-id";

        REQUIRE(ZoomIE::suitable(url));

        auto video_id = ZoomIE::extract_id(url);
        REQUIRE(video_id == "test-share-id");
    }
}

// Note: Full integration tests that actually download from Zoom would require:
// 1. Valid Zoom recording URLs (which may expire)
// 2. Network connectivity
// 3. Proper HTTP client configuration
// These tests focus on URL parsing and pattern matching instead.
