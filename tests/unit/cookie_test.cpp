#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/cookie.hpp"
#include <ctime>

using namespace ytdlp::networking;

TEST_CASE("Cookie expiration", "[networking][cookie]") {
    Cookie cookie;
    cookie.name = "test";
    cookie.value = "value";

    SECTION("session cookie never expires") {
        cookie.expires = std::nullopt;

        REQUIRE(cookie.is_session_cookie());
        REQUIRE_FALSE(cookie.is_expired());
    }

    SECTION("expires = 0 means session cookie") {
        cookie.expires = 0;

        REQUIRE(cookie.is_session_cookie());
        REQUIRE_FALSE(cookie.is_expired());
    }

    SECTION("future expiration is not expired") {
        int64_t future = static_cast<int64_t>(std::time(nullptr)) + 3600; // 1 hour from now
        cookie.expires = future;

        REQUIRE_FALSE(cookie.is_session_cookie());
        REQUIRE_FALSE(cookie.is_expired());
    }

    SECTION("past expiration is expired") {
        int64_t past = static_cast<int64_t>(std::time(nullptr)) - 3600; // 1 hour ago
        cookie.expires = past;

        REQUIRE_FALSE(cookie.is_session_cookie());
        REQUIRE(cookie.is_expired());
    }

    SECTION("explicit time check") {
        cookie.expires = 1000;

        REQUIRE(cookie.is_expired(2000));
        REQUIRE_FALSE(cookie.is_expired(500));
    }
}

TEST_CASE("Cookie domain matching", "[networking][cookie]") {
    Cookie cookie;

    SECTION("exact domain match") {
        cookie.domain = "example.com";
        cookie.domain_initial_dot = false;

        REQUIRE(cookie.domain_matches("example.com"));
        REQUIRE(cookie.domain_matches("EXAMPLE.COM")); // case insensitive
        REQUIRE_FALSE(cookie.domain_matches("www.example.com"));
        REQUIRE_FALSE(cookie.domain_matches("other.com"));
    }

    SECTION("subdomain matching with leading dot") {
        cookie.domain = ".example.com";
        cookie.domain_initial_dot = true;

        REQUIRE(cookie.domain_matches("example.com"));
        REQUIRE(cookie.domain_matches("www.example.com"));
        REQUIRE(cookie.domain_matches("sub.www.example.com"));
        REQUIRE_FALSE(cookie.domain_matches("notexample.com"));
        REQUIRE_FALSE(cookie.domain_matches("other.com"));
    }

    SECTION("subdomain matching without leading dot but flag set") {
        cookie.domain = "example.com";
        cookie.domain_initial_dot = true;

        REQUIRE(cookie.domain_matches("example.com"));
        REQUIRE(cookie.domain_matches("www.example.com"));
    }

    SECTION("case insensitive matching") {
        cookie.domain = "Example.Com";
        cookie.domain_initial_dot = false;

        REQUIRE(cookie.domain_matches("example.com"));
        REQUIRE(cookie.domain_matches("EXAMPLE.COM"));
        REQUIRE(cookie.domain_matches("eXaMpLe.CoM"));
    }
}

TEST_CASE("Cookie path matching", "[networking][cookie]") {
    Cookie cookie;

    SECTION("exact path match") {
        cookie.path = "/foo";

        REQUIRE(cookie.path_matches("/foo"));
        REQUIRE_FALSE(cookie.path_matches("/bar"));
        REQUIRE_FALSE(cookie.path_matches("/fo"));
    }

    SECTION("path prefix match") {
        cookie.path = "/foo";

        REQUIRE(cookie.path_matches("/foo"));
        REQUIRE(cookie.path_matches("/foo/bar"));
        REQUIRE(cookie.path_matches("/foo/bar/baz"));
        REQUIRE_FALSE(cookie.path_matches("/foobar")); // not a directory prefix
    }

    SECTION("path with trailing slash") {
        cookie.path = "/foo/";

        REQUIRE(cookie.path_matches("/foo/"));
        REQUIRE(cookie.path_matches("/foo/bar"));
        REQUIRE_FALSE(cookie.path_matches("/foo"));
    }

    SECTION("root path") {
        cookie.path = "/";

        REQUIRE(cookie.path_matches("/"));
        REQUIRE(cookie.path_matches("/foo"));
        REQUIRE(cookie.path_matches("/foo/bar"));
    }
}

TEST_CASE("Cookie to_cookie_string", "[networking][cookie]") {
    Cookie cookie;

    SECTION("normal cookie") {
        cookie.name = "session_id";
        cookie.value = "abc123";

        REQUIRE(cookie.to_cookie_string() == "session_id=abc123");
    }

    SECTION("cookie with empty value") {
        cookie.name = "empty";
        cookie.value = "";

        REQUIRE(cookie.to_cookie_string() == "empty=");
    }

    SECTION("cookie with no name (value becomes name)") {
        cookie.name = "";
        cookie.value = "valuename";

        REQUIRE(cookie.to_cookie_string() == "valuename");
    }

    SECTION("cookie with special characters") {
        cookie.name = "data";
        cookie.value = "a=b&c=d";

        REQUIRE(cookie.to_cookie_string() == "data=a=b&c=d");
    }
}

TEST_CASE("Cookie construction and defaults", "[networking][cookie]") {
    Cookie cookie;

    SECTION("default values") {
        REQUIRE(cookie.path == "/");
        REQUIRE(cookie.path_specified == true);
        REQUIRE(cookie.domain_specified == true);
        REQUIRE(cookie.domain_initial_dot == false);
        REQUIRE(cookie.secure == false);
        REQUIRE(cookie.http_only == false);
        REQUIRE(cookie.discard == false);
        REQUIRE_FALSE(cookie.expires.has_value());
    }

    SECTION("set all fields") {
        cookie.domain = ".example.com";
        cookie.domain_initial_dot = true;
        cookie.path = "/path";
        cookie.secure = true;
        cookie.http_only = true;
        cookie.name = "test";
        cookie.value = "value";
        cookie.expires = 12345678;

        REQUIRE(cookie.domain == ".example.com");
        REQUIRE(cookie.domain_initial_dot == true);
        REQUIRE(cookie.path == "/path");
        REQUIRE(cookie.secure == true);
        REQUIRE(cookie.http_only == true);
        REQUIRE(cookie.name == "test");
        REQUIRE(cookie.value == "value");
        REQUIRE(cookie.expires.value() == 12345678);
    }
}
