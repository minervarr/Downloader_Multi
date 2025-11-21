#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/href_li_redirect.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("HrefLiRedirectIE construction", "[extractor][href_li_redirect]") {
    HrefLiRedirectIE extractor;

    REQUIRE(extractor.ie_key() == "href.li");
    REQUIRE(extractor.ie_name() == "href.li");
}

TEST_CASE("HrefLiRedirectIE URL pattern matching", "[extractor][href_li_redirect]") {
    SECTION("matches test URL 1") {
        REQUIRE(HrefLiRedirectIE::suitable("https://href.li/?https://www.reddit.com/r/cats/comments/12bluel/my_cat_helps_me_with_water/?utm_source=share&utm_medium=android_app&utm_name=androidcss&utm_term=1&utm_content=share_button"));
    }
}

TEST_CASE("HrefLiRedirectIE video ID extraction", "[extractor][href_li_redirect]") {
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}

TEST_CASE("HrefLiRedirectIE with YoutubeDL integration", "[extractor][href_li_redirect]") {
    SECTION("works with YoutubeDL instance") {
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        HrefLiRedirectIE extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "href.li");
    }
}
