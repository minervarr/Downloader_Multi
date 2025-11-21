#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/my_vidster.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("MyVidsterIE construction", "[extractor][my_vidster]") {
    MyVidsterIE extractor;

    REQUIRE(extractor.ie_key() == "MyVidster");
    REQUIRE(extractor.ie_name() == "MyVidster");
}

TEST_CASE("MyVidsterIE URL pattern matching", "[extractor][my_vidster]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("MyVidsterIE video ID extraction", "[extractor][my_vidster]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("MyVidsterIE with YoutubeDL integration", "[extractor][my_vidster]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        MyVidsterIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "MyVidster");
    }
}
