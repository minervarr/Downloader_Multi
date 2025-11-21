#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/share_videos_embed.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("ShareVideosEmbedIE construction", "[extractor][share_videos_embed]") {
    ShareVideosEmbedIE extractor;

    REQUIRE(extractor.ie_key() == "ShareVideosEmbed");
    REQUIRE(extractor.ie_name() == "ShareVideosEmbed");
}

TEST_CASE("ShareVideosEmbedIE URL pattern matching", "[extractor][share_videos_embed]") {
    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor
}

TEST_CASE("ShareVideosEmbedIE video ID extraction", "[extractor][share_videos_embed]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("ShareVideosEmbedIE with YoutubeDL integration", "[extractor][share_videos_embed]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        ShareVideosEmbedIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "ShareVideosEmbed");
    }
}
