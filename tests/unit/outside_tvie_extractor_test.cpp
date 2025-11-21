#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/outside_tvie.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("OutsideTVIE construction", "[extractor][outside_tvie]") {
    OutsideTVIE extractor;

    REQUIRE(extractor.ie_key() == "OutsideTV");
    REQUIRE(extractor.ie_name() == "OutsideTV");
}

TEST_CASE("OutsideTVIE URL pattern matching", "[extractor][outside_tvie]") {
    SECTION("matches test URL 1") {
        REQUIRE(OutsideTVIE::suitable("http://www.outsidetv.com/category/snow/play/ZjQYboH6/1/10/Hdg0jukV/4"));
    }

    SECTION("matches test URL 2") {
        REQUIRE(OutsideTVIE::suitable("http://www.outsidetv.com/home/play/ZjQYboH6/1/10/Hdg0jukV/4"));
    }
}

TEST_CASE("OutsideTVIE video ID extraction", "[extractor][outside_tvie]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("OutsideTVIE with YoutubeDL integration", "[extractor][outside_tvie]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        OutsideTVIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "OutsideTV");
    }
}
