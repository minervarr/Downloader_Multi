#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/info_extractor.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

// Test extractor implementation
class TestExtractor : public InfoExtractor {
public:
    explicit TestExtractor(YoutubeDL* ydl = nullptr)
        : InfoExtractor(ydl) {}

    std::string ie_key() const override {
        return "TestExtractor";
    }

    std::string ie_name() const override {
        return "Test Extractor";
    }

    // Expose protected methods for testing
    using InfoExtractor::_search_regex;
    using InfoExtractor::_og_search_property;
    using InfoExtractor::_og_search_title;
    using InfoExtractor::_og_search_description;
    using InfoExtractor::_og_search_thumbnail;
    using InfoExtractor::_html_search_meta;
    using InfoExtractor::_html_extract_title;
    using InfoExtractor::_parse_json;
    using InfoExtractor::_search_json;
    using InfoExtractor::_extract_json_ld;
    using InfoExtractor::_search_json_ld;
    using InfoExtractor::report_warning;
    using InfoExtractor::to_screen;
    using InfoExtractor::write_debug;
    using InfoExtractor::report_extraction;

protected:
    InfoDict _real_extract(const std::string& url) override {
        InfoDict info;
        info["id"] = "test-video-123";
        info["title"] = "Test Video";
        info["url"] = url;
        info["extractor"] = ie_key();
        return info;
    }
};

TEST_CASE("InfoExtractor construction", "[extractor][info_extractor]") {
    SECTION("construct without downloader") {
        TestExtractor extractor;

        REQUIRE(extractor.ie_key() == "TestExtractor");
        REQUIRE(extractor.ie_name() == "Test Extractor");
        REQUIRE(extractor.age_limit() == 0);
        REQUIRE(extractor.downloader() == nullptr);
    }

    SECTION("construct with downloader") {
        YoutubeDL ydl;
        TestExtractor extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
    }

    SECTION("set downloader") {
        TestExtractor extractor;
        YoutubeDL ydl;

        extractor.set_downloader(&ydl);
        REQUIRE(extractor.downloader() == &ydl);
    }
}

TEST_CASE("InfoExtractor extract", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("successful extraction") {
        InfoDict info = extractor.extract("https://example.com/video");

        REQUIRE(info::get_string(info, "id") == "test-video-123");
        REQUIRE(info::get_string(info, "title") == "Test Video");
        REQUIRE(info::get_string(info, "url") == "https://example.com/video");
        REQUIRE(info::get_string(info, "extractor") == "TestExtractor");
    }
}

TEST_CASE("InfoExtractor _search_regex", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("simple extraction") {
        std::string html = R"(<title>My Video Title</title>)";
        std::string title = extractor._search_regex(
            R"(<title>([^<]+)</title>)",
            html,
            "title"
        );

        REQUIRE(title == "My Video Title");
    }

    SECTION("extraction with multiple groups") {
        std::string html = R"(<meta name="author" content="John Doe">)";
        std::string author = extractor._search_regex(
            R"regex(<meta name="([^"]+)" content="([^"]+)">)regex",
            html,
            "author",
            std::nullopt,
            true,
            std::regex::ECMAScript,
            2  // Extract second group
        );

        REQUIRE(author == "John Doe");
    }

    SECTION("pattern not found with default") {
        std::string html = "<div>No match here</div>";
        std::string result = extractor._search_regex(
            R"(<title>([^<]+)</title>)",
            html,
            "title",
            "Default Title"
        );

        REQUIRE(result == "Default Title");
    }

    SECTION("pattern not found without default throws") {
        std::string html = "<div>No match here</div>";

        REQUIRE_THROWS_AS(
            extractor._search_regex(
                R"(<title>([^<]+)</title>)",
                html,
                "title"
            ),
            std::runtime_error
        );
    }

    SECTION("pattern not found non-fatal returns empty") {
        std::string html = "<div>No match here</div>";
        std::string result = extractor._search_regex(
            R"(<title>([^<]+)</title>)",
            html,
            "title",
            std::nullopt,
            false  // non-fatal
        );

        REQUIRE(result.empty());
    }

    SECTION("case insensitive search") {
        std::string html = "<TITLE>MY TITLE</TITLE>";
        std::string title = extractor._search_regex(
            R"(<title>([^<]+)</title>)",
            html,
            "title",
            std::nullopt,
            true,
            std::regex::ECMAScript | std::regex::icase
        );

        REQUIRE(title == "MY TITLE");
    }
}

TEST_CASE("InfoExtractor Open Graph extraction", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("og:title extraction") {
        std::string html = R"(
            <html>
            <head>
                <meta property="og:title" content="Amazing Video">
            </head>
            </html>
        )";

        std::string title = extractor._og_search_title(html);
        REQUIRE(title == "Amazing Video");
    }

    SECTION("og:description extraction") {
        std::string html = R"(
            <meta property="og:description" content="This is a great video about cats">
        )";

        std::string desc = extractor._og_search_description(html);
        REQUIRE(desc == "This is a great video about cats");
    }

    SECTION("og:image extraction") {
        std::string html = R"(
            <meta property="og:image" content="https://example.com/thumb.jpg">
        )";

        std::string thumb = extractor._og_search_thumbnail(html);
        REQUIRE(thumb == "https://example.com/thumb.jpg");
    }

    SECTION("content before property") {
        std::string html = R"(
            <meta content="Reversed Order" property="og:title">
        )";

        std::string title = extractor._og_search_title(html);
        REQUIRE(title == "Reversed Order");
    }

    SECTION("HTML entities are unescaped") {
        std::string html = R"(
            <meta property="og:title" content="Tom &amp; Jerry">
        )";

        std::string title = extractor._og_search_title(html);
        REQUIRE(title == "Tom & Jerry");
    }

    SECTION("not found non-fatal") {
        std::string html = "<html><body>No OG tags</body></html>";

        std::string title = extractor._og_search_title(html, false);
        REQUIRE(title.empty());
    }
}

TEST_CASE("InfoExtractor HTML meta tag extraction", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("single name search") {
        std::string html = R"(
            <meta name="description" content="Video description here">
        )";

        std::string desc = extractor._html_search_meta(
            {"description"},
            html
        );

        REQUIRE(desc == "Video description here");
    }

    SECTION("multiple name fallback") {
        std::string html = R"(
            <meta name="twitter:description" content="Twitter desc">
        )";

        std::string desc = extractor._html_search_meta(
            {"description", "twitter:description"},
            html
        );

        REQUIRE(desc == "Twitter desc");
    }

    SECTION("property attribute") {
        std::string html = R"(
            <meta property="video:duration" content="120">
        )";

        std::string duration = extractor._html_search_meta(
            {"video:duration"},
            html
        );

        REQUIRE(duration == "120");
    }

    SECTION("content before name") {
        std::string html = R"(
            <meta content="Author Name" name="author">
        )";

        std::string author = extractor._html_search_meta(
            {"author"},
            html
        );

        REQUIRE(author == "Author Name");
    }

    SECTION("not found with default") {
        std::string html = "<html>No meta tags</html>";

        std::string result = extractor._html_search_meta(
            {"description"},
            html,
            std::nullopt,
            "Default description"
        );

        REQUIRE(result == "Default description");
    }

    SECTION("HTML entities unescaped") {
        std::string html = R"(
            <meta name="title" content="A &lt;tag&gt; test">
        )";

        std::string title = extractor._html_search_meta(
            {"title"},
            html
        );

        REQUIRE(title == "A <tag> test");
    }
}

TEST_CASE("InfoExtractor title extraction", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("basic title extraction") {
        std::string html = R"(
            <html>
            <head>
                <title>Page Title</title>
            </head>
            </html>
        )";

        std::string title = extractor._html_extract_title(html);
        REQUIRE(title == "Page Title");
    }

    SECTION("title with whitespace") {
        std::string html = "<title>  Trimmed Title  </title>";

        std::string title = extractor._html_extract_title(html);
        REQUIRE(title == "Trimmed Title");
    }

    SECTION("title with HTML entities") {
        std::string html = "<title>A&amp;B &lt;test&gt;</title>";

        std::string title = extractor._html_extract_title(html);
        REQUIRE(title == "A&B <test>");
    }

    SECTION("title with attributes") {
        std::string html = R"(<title lang="en">Attributed Title</title>)";

        std::string title = extractor._html_extract_title(html);
        REQUIRE(title == "Attributed Title");
    }

    SECTION("no title non-fatal") {
        std::string html = "<html><body>No title tag</body></html>";

        std::string title = extractor._html_extract_title(html, "title", false);
        REQUIRE(title.empty());
    }
}

TEST_CASE("InfoExtractor logging methods", "[extractor][info_extractor]") {
    TestExtractor extractor;

    SECTION("methods don't crash without downloader") {
        // These should work even without a downloader
        REQUIRE_NOTHROW(extractor.report_warning("Test warning"));
        REQUIRE_NOTHROW(extractor.to_screen("Test message"));
        REQUIRE_NOTHROW(extractor.write_debug("Test debug"));
        REQUIRE_NOTHROW(extractor.report_extraction("test-id"));
    }

    SECTION("methods work with downloader") {
        YoutubeDL ydl;
        extractor.set_downloader(&ydl);

        REQUIRE_NOTHROW(extractor.report_warning("Test warning", "video-123"));
        REQUIRE_NOTHROW(extractor.to_screen("Test message"));
        REQUIRE_NOTHROW(extractor.write_debug("Test debug"));
        REQUIRE_NOTHROW(extractor.report_extraction("test-id"));
    }
}

TEST_CASE("InfoExtractor _parse_json", "[extractor][info_extractor][json]") {
    TestExtractor extractor;

    SECTION("parses valid JSON object") {
        std::string json_str = R"({"title": "Test Video", "id": "123"})";
        auto result = extractor._parse_json(json_str, "test-id");

        REQUIRE(result.is_object());
        REQUIRE((info::get_string(result, "title") == "Test Video"));
        REQUIRE((info::get_string(result, "id") == "123"));
    }

    SECTION("parses valid JSON array") {
        std::string json_str = R"([1, 2, 3, 4, 5])";
        auto result = extractor._parse_json(json_str, "test-id");

        REQUIRE(result.is_array());
        REQUIRE(result.size() == 5);
    }

    SECTION("handles invalid JSON with fatal=false") {
        std::string json_str = "{invalid json}";
        auto result = extractor._parse_json(json_str, "test-id", std::nullopt, false);

        REQUIRE(result.is_null());
    }

    SECTION("throws on invalid JSON with fatal=true") {
        std::string json_str = "{invalid json}";

        REQUIRE_THROWS_AS(
            extractor._parse_json(json_str, "test-id", std::nullopt, true),
            std::runtime_error
        );
    }

    SECTION("parses nested JSON") {
        std::string json_str = R"({
            "video": {
                "title": "Nested",
                "formats": [
                    {"url": "http://example.com/1.mp4"},
                    {"url": "http://example.com/2.mp4"}
                ]
            }
        })";
        auto result = extractor._parse_json(json_str, "test-id");

        REQUIRE(result.is_object());
        REQUIRE(result.contains("video"));
        REQUIRE(result["video"].is_object());
    }
}

TEST_CASE("InfoExtractor _search_json", "[extractor][info_extractor][json]") {
    TestExtractor extractor;

    SECTION("extracts JSON object from HTML") {
        std::string html = R"(
            <script>
            var config = {"videoId": "abc123", "title": "Test"};
            </script>
        )";

        auto result = extractor._search_json(
            R"(var config\s*=\s*)",
            html,
            "config",
            "test-id"
        );

        REQUIRE(result.is_object());
        REQUIRE((info::get_string(result, "videoId") == "abc123"));
        REQUIRE((info::get_string(result, "title") == "Test"));
    }

    SECTION("extracts JSON array from HTML") {
        std::string html = R"(
            <script>
            var items = [{"id": 1}, {"id": 2}, {"id": 3}];
            </script>
        )";

        auto result = extractor._search_json(
            R"(var items\s*=\s*)",
            html,
            "items",
            "test-id"
        );

        REQUIRE(result.is_array());
        REQUIRE(result.size() == 3);
    }

    SECTION("handles nested braces correctly") {
        std::string html = R"(
            var data = {"outer": {"inner": {"deep": "value"}}, "count": 42};
        )";

        auto result = extractor._search_json(
            R"(var data\s*=\s*)",
            html,
            "data",
            "test-id"
        );

        REQUIRE(result.is_object());
        REQUIRE(result.contains("outer"));
        REQUIRE((info::get_int(result, "count") == 42));
    }

    SECTION("handles JSON with escaped quotes") {
        std::string html = R"(
            var text = {"message": "He said \"hello\""};
        )";

        auto result = extractor._search_json(
            R"(var text\s*=\s*)",
            html,
            "text",
            "test-id"
        );

        REQUIRE(result.is_object());
        REQUIRE(result.contains("message"));
    }

    SECTION("returns null for missing pattern with fatal=false") {
        std::string html = "<div>No JSON here</div>";

        auto result = extractor._search_json(
            R"(var config\s*=\s*)",
            html,
            "config",
            "test-id",
            "",
            false
        );

        REQUIRE(result.is_null());
    }

    SECTION("throws for missing pattern with fatal=true") {
        std::string html = "<div>No JSON here</div>";

        REQUIRE_THROWS_AS(
            extractor._search_json(
                R"(var config\s*=\s*)",
                html,
                "config",
                "test-id",
                "",
                true
            ),
            std::runtime_error
        );
    }
}

TEST_CASE("InfoExtractor _extract_json_ld", "[extractor][info_extractor][json]") {
    TestExtractor extractor;

    SECTION("extracts single JSON-LD") {
        std::string html = R"(
            <html>
            <head>
                <script type="application/ld+json">
                {
                    "@context": "https://schema.org",
                    "@type": "VideoObject",
                    "name": "Test Video",
                    "description": "A test video"
                }
                </script>
            </head>
            </html>
        )";

        auto results = extractor._extract_json_ld(html, "test-id", false);

        REQUIRE(results.size() == 1);
        REQUIRE((info::get_string(results[0], "@type") == "VideoObject"));
        REQUIRE((info::get_string(results[0], "name") == "Test Video"));
    }

    SECTION("extracts multiple JSON-LD blocks") {
        std::string html = R"(
            <html>
            <head>
                <script type="application/ld+json">
                {"@type": "VideoObject", "name": "Video 1"}
                </script>
                <script type="application/ld+json">
                {"@type": "WebPage", "name": "Page"}
                </script>
            </head>
            </html>
        )";

        auto results = extractor._extract_json_ld(html, "test-id", false);

        REQUIRE(results.size() == 2);
    }

    SECTION("handles JSON-LD array") {
        std::string html = R"(
            <script type="application/ld+json">
            [
                {"@type": "VideoObject", "name": "Video 1"},
                {"@type": "VideoObject", "name": "Video 2"}
            ]
            </script>
        )";

        auto results = extractor._extract_json_ld(html, "test-id", false);

        REQUIRE(results.size() == 2);
    }

    SECTION("returns empty vector when not found with fatal=false") {
        std::string html = "<html><body>No JSON-LD</body></html>";

        auto results = extractor._extract_json_ld(html, "test-id", false);

        REQUIRE(results.empty());
    }

    SECTION("throws when not found with fatal=true") {
        std::string html = "<html><body>No JSON-LD</body></html>";

        REQUIRE_THROWS_AS(
            extractor._extract_json_ld(html, "test-id", true),
            std::runtime_error
        );
    }
}

TEST_CASE("InfoExtractor _search_json_ld", "[extractor][info_extractor][json]") {
    TestExtractor extractor;

    SECTION("extracts video properties from JSON-LD") {
        std::string html = R"(
            <script type="application/ld+json">
            {
                "@context": "https://schema.org",
                "@type": "VideoObject",
                "name": "Amazing Video",
                "description": "This is amazing",
                "thumbnailUrl": "https://example.com/thumb.jpg",
                "uploadDate": "2024-01-15",
                "duration": "PT5M30S",
                "contentUrl": "https://example.com/video.mp4"
            }
            </script>
        )";

        auto info = extractor._search_json_ld(html, "test-id", std::nullopt, false);

        REQUIRE(!info.empty());
        REQUIRE((info::get_string(info, "title") == "Amazing Video"));
        REQUIRE((info::get_string(info, "description") == "This is amazing"));
        REQUIRE((info::get_string(info, "thumbnail") == "https://example.com/thumb.jpg"));
        REQUIRE((info::get_string(info, "upload_date") == "2024-01-15"));
        REQUIRE((info::get_string(info, "url") == "https://example.com/video.mp4"));
    }

    SECTION("filters by expected type") {
        std::string html = R"(
            <script type="application/ld+json">
            {"@type": "WebPage", "name": "Page"}
            </script>
            <script type="application/ld+json">
            {"@type": "VideoObject", "name": "Video", "description": "A video"}
            </script>
        )";

        auto info = extractor._search_json_ld(html, "test-id", "VideoObject", false);

        REQUIRE(!info.empty());
        REQUIRE((info::get_string(info, "title") == "Video"));
        REQUIRE((info::get_string(info, "description") == "A video"));
    }

    SECTION("handles author object") {
        std::string html = R"(
            <script type="application/ld+json">
            {
                "@type": "VideoObject",
                "name": "Video",
                "author": {
                    "@type": "Person",
                    "name": "John Doe"
                }
            }
            </script>
        )";

        auto info = extractor._search_json_ld(html, "test-id", std::nullopt, false);

        REQUIRE(!info.empty());
        REQUIRE((info::get_string(info, "uploader") == "John Doe"));
    }

    SECTION("handles author string") {
        std::string html = R"(
            <script type="application/ld+json">
            {
                "@type": "VideoObject",
                "name": "Video",
                "author": "Jane Smith"
            }
            </script>
        )";

        auto info = extractor._search_json_ld(html, "test-id", std::nullopt, false);

        REQUIRE(!info.empty());
        REQUIRE((info::get_string(info, "uploader") == "Jane Smith"));
    }

    SECTION("handles thumbnail array") {
        std::string html = R"(
            <script type="application/ld+json">
            {
                "@type": "VideoObject",
                "name": "Video",
                "thumbnailUrl": [
                    "https://example.com/thumb1.jpg",
                    "https://example.com/thumb2.jpg"
                ]
            }
            </script>
        )";

        auto info = extractor._search_json_ld(html, "test-id", std::nullopt, false);

        REQUIRE(!info.empty());
        // Should use first thumbnail
        REQUIRE((info::get_string(info, "thumbnail") == "https://example.com/thumb1.jpg"));
    }
}
