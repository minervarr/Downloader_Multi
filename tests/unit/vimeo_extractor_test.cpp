#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/vimeo.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("VimeoExtractor construction", "[extractor][vimeo]") {
    VimeoExtractor extractor;

    REQUIRE(extractor.ie_key() == "Vimeo");
    REQUIRE(extractor.ie_name() == "Vimeo");
}

TEST_CASE("VimeoExtractor URL pattern matching", "[extractor][vimeo]") {
    SECTION("matches standard Vimeo URL") {
        REQUIRE(VimeoExtractor::suitable("https://vimeo.com/123456789"));
        REQUIRE(VimeoExtractor::suitable("http://vimeo.com/123456789"));
        REQUIRE(VimeoExtractor::suitable("https://www.vimeo.com/123456789"));
    }

    SECTION("matches player URL") {
        REQUIRE(VimeoExtractor::suitable("https://player.vimeo.com/video/123456789"));
        REQUIRE(VimeoExtractor::suitable("http://player.vimeo.com/video/987654321"));
    }

    SECTION("matches channel URL") {
        REQUIRE(VimeoExtractor::suitable("https://vimeo.com/channels/staffpicks/123456789"));
        REQUIRE(VimeoExtractor::suitable("https://www.vimeo.com/channels/documentary/987654321"));
    }

    SECTION("matches album URL") {
        REQUIRE(VimeoExtractor::suitable("https://vimeo.com/album/12345/video/123456789"));
        REQUIRE(VimeoExtractor::suitable("https://www.vimeo.com/album/67890/video/555555"));
    }

    SECTION("matches groups URL") {
        REQUIRE(VimeoExtractor::suitable("https://vimeo.com/groups/shortfilms/videos/123456789"));
    }

    SECTION("rejects non-Vimeo URLs") {
        REQUIRE_FALSE(VimeoExtractor::suitable("https://youtube.com/watch?v=123"));
        REQUIRE_FALSE(VimeoExtractor::suitable("https://example.com/video"));
        REQUIRE_FALSE(VimeoExtractor::suitable("https://vimeo.com/"));  // No video ID
    }

    SECTION("rejects invalid Vimeo URLs") {
        REQUIRE_FALSE(VimeoExtractor::suitable("https://vimeo.com/notanumber"));
        REQUIRE_FALSE(VimeoExtractor::suitable("https://vimeo.com/about"));
    }
}

TEST_CASE("VimeoExtractor video ID extraction", "[extractor][vimeo]") {
    SECTION("extracts ID from standard URL") {
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/123456789") == "123456789");
        REQUIRE(VimeoExtractor::extract_id("http://www.vimeo.com/987654321") == "987654321");
    }

    SECTION("extracts ID from player URL") {
        REQUIRE(VimeoExtractor::extract_id("https://player.vimeo.com/video/555555") == "555555");
    }

    SECTION("extracts ID from channel URL") {
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/channels/staffpicks/123456789") == "123456789");
    }

    SECTION("extracts ID from album URL") {
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/album/12345/video/777777") == "777777");
    }

    SECTION("extracts ID from groups URL") {
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/groups/shortfilms/videos/888888") == "888888");
    }

    SECTION("throws on invalid URL") {
        REQUIRE_THROWS_AS(
            VimeoExtractor::extract_id("https://youtube.com/watch?v=123"),
            std::runtime_error
        );

        REQUIRE_THROWS_AS(
            VimeoExtractor::extract_id("https://vimeo.com/notanumber"),
            std::runtime_error
        );
    }
}

TEST_CASE("VimeoExtractor with YoutubeDL integration", "[extractor][vimeo]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        VimeoExtractor extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Vimeo");
    }

    SECTION("can be constructed without downloader") {
        VimeoExtractor extractor(nullptr);

        REQUIRE(extractor.downloader() == nullptr);
        REQUIRE(extractor.ie_key() == "Vimeo");
    }
}

TEST_CASE("VimeoExtractor URL validation edge cases", "[extractor][vimeo]") {
    SECTION("handles URLs with query parameters") {
        std::string url = "https://vimeo.com/123456789?quality=hd";
        REQUIRE(VimeoExtractor::suitable(url));
        REQUIRE(VimeoExtractor::extract_id(url) == "123456789");
    }

    SECTION("handles URLs with fragments") {
        std::string url = "https://vimeo.com/123456789#t=30s";
        REQUIRE(VimeoExtractor::suitable(url));
        REQUIRE(VimeoExtractor::extract_id(url) == "123456789");
    }

    SECTION("handles URLs with both query and fragment") {
        std::string url = "https://vimeo.com/123456789?autoplay=1#t=10s";
        REQUIRE(VimeoExtractor::suitable(url));
        REQUIRE(VimeoExtractor::extract_id(url) == "123456789");
    }

    SECTION("handles different video ID lengths") {
        // Short ID
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/123") == "123");

        // Long ID
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/123456789012") == "123456789012");
    }
}

TEST_CASE("VimeoExtractor case sensitivity", "[extractor][vimeo]") {
    SECTION("matches case-insensitive hostnames") {
        REQUIRE(VimeoExtractor::suitable("https://VIMEO.COM/123456789"));
        REQUIRE(VimeoExtractor::suitable("https://Vimeo.Com/123456789"));
        REQUIRE(VimeoExtractor::suitable("https://PLAYER.VIMEO.COM/video/123456789"));
    }

    SECTION("extracts ID from case-insensitive URLs") {
        REQUIRE(VimeoExtractor::extract_id("https://VIMEO.COM/123456789") == "123456789");
        REQUIRE(VimeoExtractor::extract_id("https://PLAYER.VIMEO.COM/video/987654") == "987654");
    }
}

TEST_CASE("VimeoExtractor protocol handling", "[extractor][vimeo]") {
    SECTION("accepts HTTP URLs") {
        REQUIRE(VimeoExtractor::suitable("http://vimeo.com/123456789"));
        REQUIRE(VimeoExtractor::extract_id("http://vimeo.com/123456789") == "123456789");
    }

    SECTION("accepts HTTPS URLs") {
        REQUIRE(VimeoExtractor::suitable("https://vimeo.com/123456789"));
        REQUIRE(VimeoExtractor::extract_id("https://vimeo.com/123456789") == "123456789");
    }

    SECTION("rejects other protocols") {
        REQUIRE_FALSE(VimeoExtractor::suitable("ftp://vimeo.com/123456789"));
        REQUIRE_FALSE(VimeoExtractor::suitable("file://vimeo.com/123456789"));
    }
}

TEST_CASE("VimeoExtractor multiple patterns", "[extractor][vimeo]") {
    SECTION("all patterns extract correct ID") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"https://vimeo.com/123456789", "123456789"},
            {"https://player.vimeo.com/video/123456789", "123456789"},
            {"https://vimeo.com/channels/test/123456789", "123456789"},
            {"https://vimeo.com/album/555/video/123456789", "123456789"},
            {"https://vimeo.com/groups/test/videos/123456789", "123456789"},
        };

        for (const auto& [url, expected_id] : test_cases) {
            REQUIRE(VimeoExtractor::suitable(url));
            REQUIRE(VimeoExtractor::extract_id(url) == expected_id);
        }
    }
}
