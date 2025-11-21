#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/bundesliga.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("BundesligaIE construction", "[extractor][bundesliga]") {
    BundesligaIE extractor;

    REQUIRE(extractor.ie_key() == "Bundesliga");
    REQUIRE(extractor.ie_name() == "Bundesliga");
}

TEST_CASE("BundesligaIE URL pattern matching", "[extractor][bundesliga]") {
    SECTION("matches test URL 1") {
        REQUIRE(BundesligaIE::suitable("https://www.bundesliga.com/en/bundesliga/videos?vid=bhhHkKyN"));
    }

    SECTION("matches test URL 2") {
        REQUIRE(BundesligaIE::suitable("https://www.bundesliga.com/en/bundesliga/videos/latest-features/T8IKc8TX?vid=ROHjs06G"));
    }

    SECTION("matches test URL 3") {
        REQUIRE(BundesligaIE::suitable("https://www.bundesliga.com/en/bundesliga/videos/goals?vid=mOG56vWA"));
    }
}

TEST_CASE("BundesligaIE video ID extraction", "[extractor][bundesliga]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("BundesligaIE with YoutubeDL integration", "[extractor][bundesliga]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        BundesligaIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "Bundesliga");
    }
}
