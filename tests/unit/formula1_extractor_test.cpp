#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/formula1.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("Formula1IE construction", "[extractor][formula1]") {
    Formula1IE extractor;

    REQUIRE(extractor.ie_key() == "Formula1");
    REQUIRE(extractor.ie_name() == "Formula1");
}

TEST_CASE("Formula1IE URL pattern matching", "[extractor][formula1]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("Formula1IE video ID extraction", "[extractor][formula1]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("Formula1IE with YoutubeDL integration", "[extractor][formula1]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        Formula1IE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Formula1");
    }
}
