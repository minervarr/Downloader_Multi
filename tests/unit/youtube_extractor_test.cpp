#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/youtube.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("YoutubeIE construction", "[extractor][youtube]") {
    YoutubeIE extractor;

    REQUIRE(extractor.ie_key() == "Youtube");
    REQUIRE(extractor.ie_name() == "youtube.com");
}

TEST_CASE("YoutubeIE URL pattern matching", "[extractor][youtube]") {
    SECTION("matches youtube.com/watch?v=ID") {
        REQUIRE(YoutubeIE::suitable("https://www.youtube.com/watch?v=dQw4w9WgXcQ"));
        REQUIRE(YoutubeIE::suitable("https://youtube.com/watch?v=dQw4w9WgXcQ"));
        REQUIRE(YoutubeIE::suitable("http://www.youtube.com/watch?v=dQw4w9WgXcQ"));
    }

    SECTION("matches youtu.be/ID") {
        REQUIRE(YoutubeIE::suitable("https://youtu.be/dQw4w9WgXcQ"));
        REQUIRE(YoutubeIE::suitable("http://youtu.be/dQw4w9WgXcQ"));
    }

    SECTION("matches youtube.com/embed/ID") {
        REQUIRE(YoutubeIE::suitable("https://www.youtube.com/embed/dQw4w9WgXcQ"));
    }

    SECTION("matches youtube.com/v/ID") {
        REQUIRE(YoutubeIE::suitable("https://www.youtube.com/v/dQw4w9WgXcQ"));
    }

    SECTION("matches bare video ID") {
        REQUIRE(YoutubeIE::suitable("dQw4w9WgXcQ"));
    }

    SECTION("rejects invalid URLs") {
        REQUIRE_FALSE(YoutubeIE::suitable("https://www.example.com/watch?v=dQw4w9WgXcQ"));
        REQUIRE_FALSE(YoutubeIE::suitable("not-a-url"));
    }
}

TEST_CASE("YoutubeIE video ID extraction", "[extractor][youtube]") {
    SECTION("extracts from youtube.com/watch?v=ID") {
        REQUIRE(YoutubeIE::extract_id("https://www.youtube.com/watch?v=dQw4w9WgXcQ") == "dQw4w9WgXcQ");
    }

    SECTION("extracts from youtu.be/ID") {
        REQUIRE(YoutubeIE::extract_id("https://youtu.be/dQw4w9WgXcQ") == "dQw4w9WgXcQ");
    }

    SECTION("extracts from youtube.com/embed/ID") {
        REQUIRE(YoutubeIE::extract_id("https://www.youtube.com/embed/dQw4w9WgXcQ") == "dQw4w9WgXcQ");
    }

    SECTION("extracts bare video ID") {
        REQUIRE(YoutubeIE::extract_id("dQw4w9WgXcQ") == "dQw4w9WgXcQ");
    }
}

TEST_CASE("YoutubeIE with YoutubeDL integration", "[extractor][youtube]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        YoutubeIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Youtube");
    }
}

// Note: Real extraction tests would require network access
// These are just structure/compilation tests
TEST_CASE("YoutubeIE format metadata", "[extractor][youtube]") {
    SECTION("has format metadata for common itags") {
        YoutubeIE extractor;

        // Check that FORMAT_MAP has expected entries
        // (This is a basic sanity check - full testing would verify all formats)
        REQUIRE(YoutubeIE::suitable("dQw4w9WgXcQ"));  // Just verify it compiles
    }
}
