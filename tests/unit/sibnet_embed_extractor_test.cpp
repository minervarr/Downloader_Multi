#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/sibnet_embed.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("SibnetEmbedIE construction", "[extractor][sibnet_embed]") {
    SibnetEmbedIE extractor;

    REQUIRE(extractor.ie_key() == "SibnetEmbed");
    REQUIRE(extractor.ie_name() == "SibnetEmbed");
}

TEST_CASE("SibnetEmbedIE URL pattern matching", "[extractor][sibnet_embed]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("SibnetEmbedIE video ID extraction", "[extractor][sibnet_embed]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("SibnetEmbedIE with YoutubeDL integration", "[extractor][sibnet_embed]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        SibnetEmbedIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "SibnetEmbed");
    }
}
