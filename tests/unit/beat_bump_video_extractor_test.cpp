#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/beat_bump_video.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("BeatBumpVideoIE construction", "[extractor][beat_bump_video]") {
    BeatBumpVideoIE extractor;

    REQUIRE(extractor.ie_key() == "BeatBumpVideo");
    REQUIRE(extractor.ie_name() == "BeatBumpVideo");
}

TEST_CASE("BeatBumpVideoIE URL pattern matching", "[extractor][beat_bump_video]") {
    SECTION("matches test URL 1") {
        REQUIRE(BeatBumpVideoIE::suitable("https://beatbump.ml/listen?id=MgNrAu2pzNs"));
    }

    SECTION("matches test URL 2") {
        REQUIRE(BeatBumpVideoIE::suitable("https://beatbump.io/listen?id=LDGZAprNGWo"));
    }
}

TEST_CASE("BeatBumpVideoIE video ID extraction", "[extractor][beat_bump_video]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("BeatBumpVideoIE with YoutubeDL integration", "[extractor][beat_bump_video]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        BeatBumpVideoIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "BeatBumpVideo");
    }
}
