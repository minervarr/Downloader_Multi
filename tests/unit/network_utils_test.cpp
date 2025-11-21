#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/network_utils.hpp"

using namespace ytdlp::utils;

TEST_CASE("random_user_agent generates valid user-agent", "[utils][network]") {
    SECTION("generates Chrome user-agent") {
        auto ua = random_user_agent();
        REQUIRE_FALSE(ua.empty());
        REQUIRE(ua.find("Mozilla/5.0") != std::string::npos);
        REQUIRE(ua.find("Chrome/") != std::string::npos);
        REQUIRE(ua.find("Safari/537.36") != std::string::npos);
    }

    SECTION("generates different versions") {
        auto ua1 = random_user_agent();
        auto ua2 = random_user_agent();
        // May be same or different - just check validity
        REQUIRE_FALSE(ua1.empty());
        REQUIRE_FALSE(ua2.empty());
    }
}

TEST_CASE("std_headers returns standard HTTP headers", "[utils][network]") {
    SECTION("includes required headers") {
        auto headers = std_headers();
        REQUIRE(headers.count("User-Agent") > 0);
        REQUIRE(headers.count("Accept") > 0);
        REQUIRE(headers.count("Accept-Language") > 0);
        REQUIRE(headers.count("Sec-Fetch-Mode") > 0);
    }

    SECTION("User-Agent is Chrome-like") {
        auto headers = std_headers();
        auto ua = headers["User-Agent"];
        REQUIRE(ua.find("Chrome/") != std::string::npos);
    }
}

TEST_CASE("parse_url extracts URL components", "[utils][network]") {
    SECTION("parses complete URL") {
        auto parts = parse_url("https://example.com:8080/path/to/resource?key=value#section");
        REQUIRE(parts["scheme"] == "https");
        REQUIRE(parts["netloc"] == "example.com:8080");
        REQUIRE(parts["path"] == "/path/to/resource");
        REQUIRE(parts["query"] == "key=value");
        REQUIRE(parts["fragment"] == "section");
    }

    SECTION("parses URL without port") {
        auto parts = parse_url("http://example.com/path");
        REQUIRE(parts["scheme"] == "http");
        REQUIRE(parts["netloc"] == "example.com");
        REQUIRE(parts["path"] == "/path");
    }

    SECTION("parses URL without scheme") {
        auto parts = parse_url("example.com/path");
        REQUIRE(parts["scheme"] == "");
        REQUIRE(parts["netloc"] == "example.com");
        REQUIRE(parts["path"] == "/path");
    }
}

TEST_CASE("build_url constructs URL from components", "[utils][network]") {
    SECTION("builds complete URL") {
        std::map<std::string, std::string> parts = {
            {"scheme", "https"},
            {"netloc", "example.com"},
            {"path", "/path"},
            {"query", "key=value"},
            {"fragment", "section"}
        };
        auto url = build_url(parts);
        REQUIRE(url == "https://example.com/path?key=value#section");
    }

    SECTION("builds URL without optional parts") {
        std::map<std::string, std::string> parts = {
            {"scheme", "http"},
            {"netloc", "example.com"},
            {"path", "/path"}
        };
        auto url = build_url(parts);
        REQUIRE(url == "http://example.com/path");
    }
}

TEST_CASE("remove_dot_segments normalizes paths", "[utils][network]") {
    SECTION("removes single dot segments") {
        auto result = remove_dot_segments("/a/./b");
        REQUIRE(result == "/a/b");
    }

    SECTION("removes double dot segments") {
        auto result = remove_dot_segments("/a/b/../c");
        REQUIRE(result == "/a/c");
    }

    SECTION("handles complex path") {
        auto result = remove_dot_segments("/a/./b/../c/./d");
        REQUIRE(result == "/a/c/d");
    }

    SECTION("handles path starting with dots") {
        auto result = remove_dot_segments("./a/b");
        REQUIRE(result == "a/b");
    }

    SECTION("preserves absolute path") {
        auto result = remove_dot_segments("/a/b");
        REQUIRE(result == "/a/b");
    }
}

TEST_CASE("extract_query_params parses query string", "[utils][network]") {
    SECTION("extracts single parameter") {
        auto params = extract_query_params("http://example.com?key=value");
        REQUIRE(params.size() == 1);
        REQUIRE(params["key"] == "value");
    }

    SECTION("extracts multiple parameters") {
        auto params = extract_query_params("http://example.com?a=1&b=2&c=3");
        REQUIRE(params.size() == 3);
        REQUIRE(params["a"] == "1");
        REQUIRE(params["b"] == "2");
        REQUIRE(params["c"] == "3");
    }

    SECTION("handles URL-encoded values") {
        auto params = extract_query_params("http://example.com?name=John%20Doe");
        REQUIRE(params["name"] == "John Doe");
    }

    SECTION("returns empty map for URL without query") {
        auto params = extract_query_params("http://example.com/path");
        REQUIRE(params.empty());
    }
}

TEST_CASE("update_url_query updates query parameters", "[utils][network]") {
    SECTION("adds new parameter") {
        auto url = update_url_query("http://example.com", {{"key", "value"}});
        REQUIRE(url.find("key=value") != std::string::npos);
    }

    SECTION("updates existing parameter") {
        auto url = update_url_query("http://example.com?key=old", {{"key", "new"}});
        REQUIRE(url.find("key=new") != std::string::npos);
        REQUIRE(url.find("key=old") == std::string::npos);
    }

    SECTION("preserves existing parameters") {
        auto url = update_url_query("http://example.com?a=1", {{"b", "2"}});
        REQUIRE(url.find("a=1") != std::string::npos);
        REQUIRE(url.find("b=2") != std::string::npos);
    }
}

TEST_CASE("get_hostname extracts hostname", "[utils][network]") {
    SECTION("extracts hostname without port") {
        auto hostname = get_hostname("http://example.com/path");
        REQUIRE(hostname == "example.com");
    }

    SECTION("extracts hostname with port") {
        auto hostname = get_hostname("http://example.com:8080/path");
        REQUIRE(hostname == "example.com");
    }

    SECTION("handles URL without scheme") {
        auto hostname = get_hostname("example.com/path");
        REQUIRE(hostname == "example.com");
    }
}

TEST_CASE("get_port extracts port number", "[utils][network]") {
    SECTION("extracts explicit port") {
        auto port = get_port("http://example.com:8080");
        REQUIRE(port == 8080);
    }

    SECTION("returns default port for HTTP") {
        auto port = get_port("http://example.com");
        REQUIRE(port == 80);
    }

    SECTION("returns default port for HTTPS") {
        auto port = get_port("https://example.com");
        REQUIRE(port == 443);
    }
}

TEST_CASE("get_scheme extracts scheme", "[utils][network]") {
    SECTION("extracts HTTP scheme") {
        auto scheme = get_scheme("http://example.com");
        REQUIRE(scheme == "http");
    }

    SECTION("extracts HTTPS scheme") {
        auto scheme = get_scheme("https://example.com");
        REQUIRE(scheme == "https");
    }

    SECTION("returns empty for URL without scheme") {
        auto scheme = get_scheme("example.com");
        REQUIRE(scheme.empty());
    }
}

TEST_CASE("is_https checks if URL is HTTPS", "[utils][network]") {
    SECTION("returns true for HTTPS URL") {
        REQUIRE(is_https("https://example.com"));
    }

    SECTION("returns false for HTTP URL") {
        REQUIRE_FALSE(is_https("http://example.com"));
    }
}

TEST_CASE("to_https converts HTTP to HTTPS", "[utils][network]") {
    SECTION("converts HTTP to HTTPS") {
        auto url = to_https("http://example.com/path");
        REQUIRE(url.find("https://") == 0);
    }

    SECTION("preserves HTTPS URL") {
        auto url = to_https("https://example.com/path");
        REQUIRE(url == "https://example.com/path");
    }
}

TEST_CASE("url_join joins URLs correctly", "[utils][network]") {
    SECTION("joins relative path") {
        auto url = url_join("http://example.com/a/b", "c/d");
        REQUIRE(url == "http://example.com/a/c/d");
    }

    SECTION("handles absolute path") {
        auto url = url_join("http://example.com/a/b", "/c/d");
        REQUIRE(url == "http://example.com/c/d");
    }

    SECTION("handles absolute URL") {
        auto url = url_join("http://example.com/a", "https://other.com/b");
        REQUIRE(url == "https://other.com/b");
    }

    SECTION("handles parent directory") {
        auto url = url_join("http://example.com/a/b/c", "../d");
        REQUIRE(url == "http://example.com/a/d");
    }
}

TEST_CASE("clean_headers removes yt-dlp specific headers", "[utils][network]") {
    SECTION("removes Youtubedl-No-Compression") {
        std::map<std::string, std::string> headers = {
            {"User-Agent", "test"},
            {"Youtubedl-No-Compression", "1"}
        };
        clean_headers(headers);
        REQUIRE(headers.count("Youtubedl-No-Compression") == 0);
        REQUIRE(headers["Accept-Encoding"] == "identity");
    }

    SECTION("removes Ytdl-socks-proxy") {
        std::map<std::string, std::string> headers = {
            {"User-Agent", "test"},
            {"Ytdl-socks-proxy", "proxy"}
        };
        clean_headers(headers);
        REQUIRE(headers.count("Ytdl-socks-proxy") == 0);
    }
}

TEST_CASE("clean_proxies normalizes proxy settings", "[utils][network]") {
    SECTION("adds http:// scheme to proxies without scheme") {
        std::map<std::string, std::string> proxies = {{"http", "proxy.example.com:8080"}};
        std::map<std::string, std::string> headers;
        clean_proxies(proxies, headers);
        REQUIRE(proxies["http"].find("http://") == 0);
    }

    SECTION("normalizes socks5 to socks5h") {
        std::map<std::string, std::string> proxies = {{"http", "socks5://proxy.example.com"}};
        std::map<std::string, std::string> headers;
        clean_proxies(proxies, headers);
        REQUIRE(proxies["http"].find("socks5h://") == 0);
    }

    SECTION("handles __noproxy__") {
        std::map<std::string, std::string> proxies = {{"http", "__noproxy__"}};
        std::map<std::string, std::string> headers;
        clean_proxies(proxies, headers);
        REQUIRE(proxies["http"].empty());
    }
}

TEST_CASE("select_proxy selects appropriate proxy", "[utils][network]") {
    SECTION("selects scheme-specific proxy") {
        std::map<std::string, std::string> proxies = {
            {"http", "http://proxy1"},
            {"https", "http://proxy2"}
        };
        auto proxy = select_proxy("https://example.com", proxies);
        REQUIRE(proxy == "http://proxy2");
    }

    SECTION("falls back to 'all' proxy") {
        std::map<std::string, std::string> proxies = {{"all", "http://proxy"}};
        auto proxy = select_proxy("http://example.com", proxies);
        REQUIRE(proxy == "http://proxy");
    }

    SECTION("returns empty when no proxy found") {
        std::map<std::string, std::string> proxies;
        auto proxy = select_proxy("http://example.com", proxies);
        REQUIRE(proxy.empty());
    }
}

TEST_CASE("normalize_url normalizes URLs", "[utils][network]") {
    SECTION("removes dot segments") {
        auto url = normalize_url("http://example.com/a/./b/../c");
        REQUIRE(url.find("/a/c") != std::string::npos);
    }

    SECTION("lowercases domain") {
        auto url = normalize_url("http://EXAMPLE.COM/path");
        REQUIRE(url.find("example.com") != std::string::npos);
    }
}
