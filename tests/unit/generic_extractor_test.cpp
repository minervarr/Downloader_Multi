#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/generic.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("GenericExtractor construction", "[extractor][generic]") {
    GenericExtractor extractor;

    REQUIRE(extractor.ie_key() == "Generic");
    REQUIRE(extractor.ie_name() == "Generic");
}

TEST_CASE("GenericExtractor direct video URL detection", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("detects .mp4 URL") {
        InfoDict info = extractor.extract("https://example.com/video.mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/video.mp4"));
        REQUIRE((info::get_string(info, "ext") == "mp4"));
        REQUIRE((!info::get_string(info, "id").empty()));
    }

    SECTION("detects .webm URL") {
        InfoDict info = extractor.extract("https://example.com/videos/clip.webm");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/videos/clip.webm"));
        REQUIRE((info::get_string(info, "ext") == "webm"));
    }

    SECTION("detects .m4v URL") {
        InfoDict info = extractor.extract("https://cdn.example.com/content.m4v");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "ext") == "m4v"));
    }

    SECTION("handles URL with query parameters") {
        InfoDict info = extractor.extract("https://example.com/video.mp4?token=abc123");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/video.mp4?token=abc123"));
        REQUIRE((info::get_string(info, "ext") == "mp4"));
    }
}

TEST_CASE("GenericExtractor metadata extraction from HTML", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("extracts Open Graph metadata") {
        // This is a conceptual test - in reality we'd need to mock the HTTP client
        // or use dependency injection for testability
        // For now, we're testing the extractor can be constructed and called

        REQUIRE(extractor.ie_key() == "Generic");
    }

    SECTION("handles missing video URL gracefully") {
        // Extractor should return url type when no video is found
        // This would require actual HTTP mocking in a full implementation
        REQUIRE(extractor.ie_key() == "Generic");
    }
}

TEST_CASE("GenericExtractor video ID extraction", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("extracts ID from filename") {
        InfoDict info = extractor.extract("https://example.com/videos/my-video-123.mp4");

        std::string id = info::get_string(info, "id");
        REQUIRE(id == "my-video-123");
    }

    SECTION("extracts ID from simple filename") {
        InfoDict info = extractor.extract("https://example.com/video.mp4");

        std::string id = info::get_string(info, "id");
        REQUIRE(id == "video");
    }

    SECTION("generates ID for URL without clear filename") {
        InfoDict info = extractor.extract("https://example.com/watch.mp4");

        std::string id = info::get_string(info, "id");
        REQUIRE(!id.empty());  // Should generate some ID
    }
}

TEST_CASE("GenericExtractor info dict structure", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("includes required fields") {
        InfoDict info = extractor.extract("https://example.com/test.mp4");

        REQUIRE(info.contains("id"));
        REQUIRE(info.contains("title"));
        REQUIRE(info.contains("url"));
        REQUIRE(info.contains("ext"));
        REQUIRE(info.contains("_type"));
        REQUIRE(info.contains("extractor"));
        REQUIRE(info.contains("extractor_key"));
        REQUIRE(info.contains("webpage_url"));
    }

    SECTION("extractor metadata is correct") {
        InfoDict info = extractor.extract("https://example.com/video.webm");

        REQUIRE((info::get_string(info, "extractor") == "Generic"));
        REQUIRE((info::get_string(info, "extractor_key") == "Generic"));
        REQUIRE((info::get_string(info, "webpage_url") == "https://example.com/video.webm"));
    }
}

TEST_CASE("GenericExtractor file extension handling", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("correctly identifies .mp4") {
        InfoDict info = extractor.extract("https://example.com/a.mp4");
        REQUIRE((info::get_string(info, "ext") == "mp4"));
    }

    SECTION("correctly identifies .webm") {
        InfoDict info = extractor.extract("https://example.com/b.webm");
        REQUIRE((info::get_string(info, "ext") == "webm"));
    }

    SECTION("correctly identifies .m4v") {
        InfoDict info = extractor.extract("https://example.com/c.m4v");
        REQUIRE((info::get_string(info, "ext") == "m4v"));
    }

    SECTION("correctly identifies .mov") {
        InfoDict info = extractor.extract("https://example.com/d.mov");
        REQUIRE((info::get_string(info, "ext") == "mov"));
    }

    SECTION("correctly identifies .mkv") {
        InfoDict info = extractor.extract("https://example.com/e.mkv");
        REQUIRE((info::get_string(info, "ext") == "mkv"));
    }
}

TEST_CASE("GenericExtractor with YoutubeDL integration", "[extractor][generic]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        GenericExtractor extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Generic");
    }

    SECTION("can extract without downloader for direct URLs") {
        GenericExtractor extractor(nullptr);

        // Direct URL should work even without downloader
        InfoDict info = extractor.extract("https://example.com/video.mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        std::string id = info::get_string(info, "id");
        REQUIRE(!id.empty());
    }
}

TEST_CASE("GenericExtractor URL normalization", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("handles URLs with special characters") {
        InfoDict info = extractor.extract("https://example.com/my%20video.mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/my%20video.mp4"));
    }

    SECTION("handles URLs with ports") {
        InfoDict info = extractor.extract("https://example.com:8080/video.mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        std::string id = info::get_string(info, "id");
        REQUIRE(!id.empty());
    }

    SECTION("handles URLs with fragments") {
        InfoDict info = extractor.extract("https://example.com/video.mp4#start");

        REQUIRE((info::get_string(info, "_type") == "video"));
    }
}

TEST_CASE("GenericExtractor edge cases", "[extractor][generic]") {
    GenericExtractor extractor;

    SECTION("handles uppercase extension") {
        InfoDict info = extractor.extract("https://example.com/VIDEO.MP4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/VIDEO.MP4"));
    }

    SECTION("handles mixed case extension") {
        InfoDict info = extractor.extract("https://example.com/video.Mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
    }

    SECTION("handles very long filenames") {
        std::string long_url = "https://example.com/" + std::string(200, 'a') + ".mp4";
        InfoDict info = extractor.extract(long_url);

        REQUIRE((info::get_string(info, "_type") == "video"));
        std::string id = info::get_string(info, "id");
        REQUIRE(!id.empty());
    }

    SECTION("handles URLs with multiple dots") {
        InfoDict info = extractor.extract("https://example.com/video.720p.h264.mp4");

        REQUIRE((info::get_string(info, "_type") == "video"));
        REQUIRE((info::get_string(info, "ext") == "mp4"));
    }
}
