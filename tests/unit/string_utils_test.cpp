/**
 * Unit Tests for String Utilities
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ytdlp/utils/string_utils.hpp"

using namespace ytdlp::utils;
using Catch::Matchers::Equals;

// ============================================================================
// URL Encoding/Decoding Tests
// ============================================================================

TEST_CASE("url_encode encodes special characters", "[utils][string][url]") {
    SECTION("spaces are encoded as %20") {
        REQUIRE(url_encode("hello world") == "hello%20world");
    }

    SECTION("special characters are percent-encoded") {
        REQUIRE(url_encode("foo&bar=baz") == "foo%26bar%3Dbaz");
    }

    SECTION("safe characters are not encoded") {
        REQUIRE(url_encode("abc123-_.~") == "abc123-_.~");
    }

    SECTION("unicode characters are encoded") {
        // é (e with acute accent)
        REQUIRE(url_encode("\xC3\xA9") == "%C3%A9");
    }

    SECTION("safe parameter works") {
        REQUIRE(url_encode("a/b", "/") == "a/b");
        REQUIRE(url_encode("a/b", "") == "a%2Fb");
    }
}

TEST_CASE("url_decode decodes percent-encoded strings", "[utils][string][url]") {
    SECTION("decodes %20 to space") {
        REQUIRE(url_decode("hello%20world") == "hello world");
    }

    SECTION("decodes special characters") {
        REQUIRE(url_decode("foo%26bar%3Dbaz") == "foo&bar=baz");
    }

    SECTION("handles plus as space") {
        REQUIRE(url_decode("hello+world") == "hello world");
    }

    SECTION("leaves non-encoded characters alone") {
        REQUIRE(url_decode("abc123") == "abc123");
    }
}

TEST_CASE("urlencode_postdata encodes parameters", "[utils][string][url]") {
    SECTION("single parameter") {
        std::map<std::string, std::string> params = {{"q", "search"}};
        REQUIRE(urlencode_postdata(params) == "q=search");
    }

    SECTION("multiple parameters") {
        std::map<std::string, std::string> params = {
            {"q", "search term"},
            {"page", "1"}
        };
        auto result = urlencode_postdata(params);
        // Map order is sorted by key
        REQUIRE(result == "page=1&q=search%20term");
    }

    SECTION("empty map") {
        std::map<std::string, std::string> params;
        REQUIRE(urlencode_postdata(params) == "");
    }
}

// ============================================================================
// HTML Escaping Tests
// ============================================================================

TEST_CASE("escape_html escapes HTML special characters", "[utils][string][html]") {
    SECTION("escapes angle brackets") {
        REQUIRE(escape_html("<script>") == "&lt;script&gt;");
    }

    SECTION("escapes ampersand") {
        REQUIRE(escape_html("A & B") == "A &amp; B");
    }

    SECTION("escapes quotes") {
        REQUIRE(escape_html("\"hello\"") == "&quot;hello&quot;");
        REQUIRE(escape_html("'world'") == "&#39;world&#39;");
    }

    SECTION("leaves normal text alone") {
        REQUIRE(escape_html("hello world") == "hello world");
    }
}

TEST_CASE("unescape_html unescapes HTML entities", "[utils][string][html]") {
    SECTION("unescapes named entities") {
        REQUIRE(unescape_html("&lt;script&gt;") == "<script>");
        REQUIRE(unescape_html("&amp;") == "&");
        REQUIRE(unescape_html("&quot;") == "\"");
    }

    SECTION("unescapes numeric entities") {
        REQUIRE(unescape_html("&#72;ello") == "Hello");
        REQUIRE(unescape_html("&#x48;ello") == "Hello");
    }

    SECTION("leaves normal text alone") {
        REQUIRE(unescape_html("hello world") == "hello world");
    }
}

// ============================================================================
// Filename Sanitization Tests
// ============================================================================

TEST_CASE("sanitize_filename removes invalid characters", "[utils][string][filename]") {
    SECTION("removes path separators") {
        REQUIRE(sanitize_filename("foo/bar") == "foo_bar");
        REQUIRE(sanitize_filename("foo\\bar") == "foo_bar");
    }

    SECTION("removes special characters") {
        REQUIRE(sanitize_filename("foo<bar>") == "foo_bar");  // Trailing _ stripped
        REQUIRE(sanitize_filename("foo:bar") == "foo -bar");
        REQUIRE(sanitize_filename("foo|bar") == "foo_bar");
        REQUIRE(sanitize_filename("foo*bar") == "foo_bar");
    }

    SECTION("removes control characters") {
        // Control characters are removed (empty string returned for them)
        REQUIRE(sanitize_filename(std::string("foo") + '\x00' + "bar") == "foobar");
        REQUIRE(sanitize_filename(std::string("foo") + '\x1F' + "bar") == "foobar");
    }

    SECTION("strips leading dots") {
        REQUIRE(sanitize_filename(".hidden") == "hidden");
        REQUIRE(sanitize_filename("..secret") == "secret");
    }

    SECTION("strips edge underscores") {
        REQUIRE(sanitize_filename("_foo_") == "foo");
    }

    SECTION("returns underscore for empty input") {
        REQUIRE(sanitize_filename("") == "");
        REQUIRE(sanitize_filename("???") == "_");
    }

    SECTION("restricted mode is stricter") {
        REQUIRE(sanitize_filename("foo bar", true) == "foo_bar");
        REQUIRE(sanitize_filename("foo!bar", true) == "foo_bar");
    }
}

// ============================================================================
// String Manipulation Tests
// ============================================================================

TEST_CASE("strip removes whitespace", "[utils][string]") {
    SECTION("removes leading and trailing spaces") {
        REQUIRE(strip("  hello  ") == "hello");
    }

    SECTION("removes tabs and newlines") {
        REQUIRE(strip("\t\nhello\n\t") == "hello");
    }

    SECTION("leaves internal whitespace") {
        REQUIRE(strip("  hello world  ") == "hello world");
    }

    SECTION("handles empty string") {
        REQUIRE(strip("") == "");
        REQUIRE(strip("   ") == "");
    }
}

TEST_CASE("strip with character removes specific character", "[utils][string]") {
    REQUIRE(strip("__hello__", '_') == "hello");
    REQUIRE(strip("///path///", '/') == "path");
}

TEST_CASE("to_lower converts to lowercase", "[utils][string]") {
    REQUIRE(to_lower("HELLO") == "hello");
    REQUIRE(to_lower("Hello World") == "hello world");
    REQUIRE(to_lower("abc123") == "abc123");
}

TEST_CASE("to_upper converts to uppercase", "[utils][string]") {
    REQUIRE(to_upper("hello") == "HELLO");
    REQUIRE(to_upper("Hello World") == "HELLO WORLD");
    REQUIRE(to_upper("ABC123") == "ABC123");
}

TEST_CASE("split divides string by delimiter", "[utils][string]") {
    SECTION("split by character") {
        auto parts = split("a,b,c", ',');
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "b");
        REQUIRE(parts[2] == "c");
    }

    SECTION("split by string") {
        auto parts = split("a::b::c", "::");
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "b");
        REQUIRE(parts[2] == "c");
    }

    SECTION("max_split limits splits") {
        auto parts = split("a,b,c,d", ',', 2);
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "b");
        REQUIRE(parts[2] == "c,d");
    }

    SECTION("handles empty parts") {
        auto parts = split("a,,c", ',');
        REQUIRE(parts.size() == 3);
        REQUIRE(parts[1] == "");
    }
}

TEST_CASE("join concatenates strings", "[utils][string]") {
    SECTION("joins with delimiter") {
        std::vector<std::string> parts = {"a", "b", "c"};
        REQUIRE(join(parts, ",") == "a,b,c");
        REQUIRE(join(parts, " - ") == "a - b - c");
    }

    SECTION("handles empty vector") {
        std::vector<std::string> parts;
        REQUIRE(join(parts, ",") == "");
    }

    SECTION("handles single element") {
        std::vector<std::string> parts = {"a"};
        REQUIRE(join(parts, ",") == "a");
    }
}

TEST_CASE("starts_with checks prefix", "[utils][string]") {
    REQUIRE(starts_with("hello world", "hello"));
    REQUIRE(starts_with("hello", "hello"));
    REQUIRE_FALSE(starts_with("hello", "world"));
    REQUIRE_FALSE(starts_with("hi", "hello"));
}

TEST_CASE("ends_with checks suffix", "[utils][string]") {
    REQUIRE(ends_with("hello world", "world"));
    REQUIRE(ends_with("world", "world"));
    REQUIRE_FALSE(ends_with("world", "hello"));
    REQUIRE_FALSE(ends_with("hi", "hello"));
}

TEST_CASE("replace_all replaces all occurrences", "[utils][string]") {
    REQUIRE(replace_all("hello world", "l", "L") == "heLLo worLd");
    REQUIRE(replace_all("aaa", "a", "b") == "bbb");
    REQUIRE(replace_all("hello", "x", "y") == "hello");
    REQUIRE(replace_all("a b c", " ", "") == "abc");
}

// ============================================================================
// Format Detection Tests
// ============================================================================

TEST_CASE("determine_ext extracts file extension", "[utils][string]") {
    SECTION("extracts from filename") {
        REQUIRE(determine_ext("video.mp4") == "mp4");
        REQUIRE(determine_ext("audio.m4a") == "m4a");
    }

    SECTION("extracts from URL") {
        REQUIRE(determine_ext("http://example.com/video.mp4") == "mp4");
    }

    SECTION("ignores query string") {
        REQUIRE(determine_ext("video.mp4?quality=hd") == "mp4");
    }

    SECTION("ignores fragment") {
        REQUIRE(determine_ext("video.mp4#start") == "mp4");
    }

    SECTION("returns default for no extension") {
        REQUIRE(determine_ext("video") == "unknown_video");
        REQUIRE(determine_ext("video", "mkv") == "mkv");
    }

    SECTION("converts to lowercase") {
        REQUIRE(determine_ext("VIDEO.MP4") == "mp4");
    }
}

// ============================================================================
// Number Parsing Tests
// ============================================================================

TEST_CASE("parse_int parses integers", "[utils][string][parse]") {
    SECTION("parses positive integers") {
        REQUIRE(parse_int("123") == 123);
        REQUIRE(parse_int("0") == 0);
    }

    SECTION("parses negative integers") {
        REQUIRE(parse_int("-456") == -456);
    }

    SECTION("handles whitespace") {
        REQUIRE(parse_int("  123  ") == 123);
    }

    SECTION("returns nullopt for invalid input") {
        REQUIRE_FALSE(parse_int("abc").has_value());
        REQUIRE_FALSE(parse_int("12.34").has_value());
        REQUIRE_FALSE(parse_int("").has_value());
    }
}

TEST_CASE("parse_float parses floating point", "[utils][string][parse]") {
    SECTION("parses floats") {
        REQUIRE(*parse_float("123.45") == 123.45);
        REQUIRE(*parse_float("0.5") == 0.5);
    }

    SECTION("parses integers as floats") {
        REQUIRE(*parse_float("123") == 123.0);
    }

    SECTION("parses negative floats") {
        REQUIRE(*parse_float("-456.78") == -456.78);
    }

    SECTION("returns nullopt for invalid input") {
        REQUIRE_FALSE(parse_float("abc").has_value());
        REQUIRE_FALSE(parse_float("").has_value());
    }
}


// ============================================================================
// URL Sanitization Tests
// ============================================================================

TEST_CASE("sanitize_url adds missing scheme", "[utils][string][url]") {
    REQUIRE(sanitize_url("example.com") == "http://example.com");
    REQUIRE(sanitize_url("example.com", "https") == "https://example.com");
    REQUIRE(sanitize_url("http://example.com") == "http://example.com");
}

TEST_CASE("extract_basic_auth extracts credentials", "[utils][string][url]") {
    SECTION("extracts username and password") {
        auto [url, user, pass] = extract_basic_auth("http://user:pass@example.com/path");
        REQUIRE(url == "http://example.com/path");
        REQUIRE(user == "user");
        REQUIRE(pass == "pass");
    }

    SECTION("extracts username only") {
        auto [url, user, pass] = extract_basic_auth("http://user@example.com/path");
        REQUIRE(url == "http://example.com/path");
        REQUIRE(user == "user");
        REQUIRE(pass == "");
    }

    SECTION("handles no auth") {
        auto [url, user, pass] = extract_basic_auth("http://example.com/path");
        REQUIRE(url == "http://example.com/path");
        REQUIRE(user == "");
        REQUIRE(pass == "");
    }
}
