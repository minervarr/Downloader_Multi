#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/dailymotion_base_info.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("DailymotionBaseInfoExtractor construction", "[extractor][dailymotion_base_info]") {
    DailymotionBaseInfoExtractor extractor;

    REQUIRE(extractor.ie_key() == "DailymotionBaseInfo");
    REQUIRE(extractor.ie_name() == "DailymotionBaseInfo");
}

TEST_CASE("DailymotionBaseInfoExtractor URL pattern matching", "[extractor][dailymotion_base_info]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("DailymotionBaseInfoExtractor video ID extraction", "[extractor][dailymotion_base_info]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("DailymotionBaseInfoExtractor with YoutubeDL integration", "[extractor][dailymotion_base_info]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        DailymotionBaseInfoExtractor extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "DailymotionBaseInfo");
    }
}
