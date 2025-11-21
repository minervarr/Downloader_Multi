#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/unity.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("UnityIE construction", "[extractor][unity]") {
    UnityIE extractor;

    REQUIRE(extractor.ie_key() == "Unity");
    REQUIRE(extractor.ie_name() == "Unity");
}

TEST_CASE("UnityIE URL pattern matching", "[extractor][unity]") {
    SECTION("matches test URL 1") {
        REQUIRE(UnityIE::suitable("https://unity3d.com/learn/tutorials/topics/animation/animate-anything-mecanim"));
    }

    SECTION("matches test URL 2") {
        REQUIRE(UnityIE::suitable("https://unity3d.com/learn/tutorials/projects/2d-ufo-tutorial/following-player-camera?playlist=25844"));
    }
}

TEST_CASE("UnityIE video ID extraction", "[extractor][unity]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("UnityIE with YoutubeDL integration", "[extractor][unity]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        UnityIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Unity");
    }
}
