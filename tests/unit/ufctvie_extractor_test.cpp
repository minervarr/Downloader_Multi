#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/ufctvie.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("UFCTVIE construction", "[extractor][ufctvie]") {
    UFCTVIE extractor;

    REQUIRE(extractor.ie_key() == "UFCTV");
    REQUIRE(extractor.ie_name() == "UFCTV");
}

TEST_CASE("UFCTVIE URL pattern matching", "[extractor][ufctvie]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("UFCTVIE video ID extraction", "[extractor][ufctvie]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("UFCTVIE with YoutubeDL integration", "[extractor][ufctvie]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        UFCTVIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "UFCTV");
    }
}
