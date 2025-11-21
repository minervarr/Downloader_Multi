#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/request.hpp"
#include <sstream>

using namespace ytdlp::networking;

TEST_CASE("Request basic construction", "[networking][request]") {
    SECTION("construct with URL only") {
        Request req("https://example.com/path");

        REQUIRE(req.url() == "https://example.com/path");
        REQUIRE(req.method() == "GET");
        REQUIRE_FALSE(req.has_explicit_method());
        REQUIRE_FALSE(req.has_data());
    }

    SECTION("construct with method") {
        Request req("https://example.com", "POST");

        REQUIRE(req.method() == "POST");
        REQUIRE(req.has_explicit_method());
    }

    SECTION("construct with headers") {
        std::map<std::string, std::string> headers = {
            {"User-Agent", "TestAgent/1.0"},
            {"Accept", "application/json"}
        };

        Request req("https://example.com", "", headers);

        REQUIRE(req.headers().get("user-agent").value() == "TestAgent/1.0");
        REQUIRE(req.headers().get("Accept").value() == "application/json");
    }

    SECTION("construct with proxies") {
        std::map<std::string, std::string> proxies = {
            {"http", "http://proxy.example.com:8080"},
            {"https", "http://proxy.example.com:8443"}
        };

        Request req("https://example.com", "", {}, proxies);

        REQUIRE(req.proxies().at("http") == "http://proxy.example.com:8080");
        REQUIRE(req.proxies().at("https") == "http://proxy.example.com:8443");
    }

    SECTION("construct with query parameters") {
        std::map<std::string, std::string> query = {
            {"foo", "bar"},
            {"baz", "qux"}
        };

        Request req("https://example.com/path", "", {}, {}, query);

        // URL should have query parameters
        REQUIRE(req.url().find("foo=bar") != std::string::npos);
        REQUIRE(req.url().find("baz=qux") != std::string::npos);
    }
}

TEST_CASE("Request URL handling", "[networking][request]") {
    SECTION("normalizes URLs") {
        Request req("https://example.com/./path/../other");

        // URL normalization should remove dot segments
        REQUIRE(req.url().find("./") == std::string::npos);
        REQUIRE(req.url().find("..") == std::string::npos);
    }

    SECTION("handles protocol-relative URLs") {
        Request req("//example.com/path");

        REQUIRE(req.url().find("http://example.com") != std::string::npos);
    }

    SECTION("update URL") {
        Request req("https://example.com/path");
        req.set_url("https://newexample.com/newpath");

        REQUIRE(req.url() == "https://newexample.com/newpath");
    }

    SECTION("update URL query") {
        Request req("https://example.com/path");

        std::map<std::string, std::string> query = {
            {"key1", "value1"},
            {"key2", "value2"}
        };

        req.update_url_query(query);

        REQUIRE(req.url().find("key1=value1") != std::string::npos);
        REQUIRE(req.url().find("key2=value2") != std::string::npos);
    }
}

TEST_CASE("Request method handling", "[networking][request]") {
    SECTION("auto-determines GET when no data") {
        Request req("https://example.com");

        REQUIRE(req.method() == "GET");
        REQUIRE_FALSE(req.has_explicit_method());
    }

    SECTION("auto-determines POST when data present") {
        Request req("https://example.com");
        req.set_data(std::vector<uint8_t>{1, 2, 3});

        REQUIRE(req.method() == "POST");
        REQUIRE_FALSE(req.has_explicit_method());
    }

    SECTION("explicit method overrides auto-determination") {
        Request req("https://example.com", "PUT");
        req.set_data(std::vector<uint8_t>{1, 2, 3});

        REQUIRE(req.method() == "PUT");
        REQUIRE(req.has_explicit_method());
    }

    SECTION("method is uppercased") {
        Request req("https://example.com", "post");

        REQUIRE(req.method() == "POST");
    }

    SECTION("set method explicitly") {
        Request req("https://example.com");
        req.set_method("delete");

        REQUIRE(req.method() == "DELETE");
        REQUIRE(req.has_explicit_method());
    }
}

TEST_CASE("Request data handling", "[networking][request]") {
    SECTION("set data from bytes") {
        Request req("https://example.com");
        std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"

        req.set_data(data);

        REQUIRE(req.has_data());
        REQUIRE(req.method() == "POST");
    }

    SECTION("set data from stream") {
        Request req("https://example.com");
        auto stream = std::make_shared<std::istringstream>("test data");

        req.set_data(stream);

        REQUIRE(req.has_data());
        REQUIRE(req.method() == "POST");
    }

    SECTION("setting data sets Content-Type if not present") {
        Request req("https://example.com");
        req.set_data(std::vector<uint8_t>{1, 2, 3});

        REQUIRE(req.headers().contains("Content-Type"));
        REQUIRE(req.headers().get("Content-Type").value() == "application/x-www-form-urlencoded");
    }

    SECTION("setting data does not override existing Content-Type") {
        Request req("https://example.com");
        req.set_header("Content-Type", "application/json");
        req.set_data(std::vector<uint8_t>{1, 2, 3});

        REQUIRE(req.headers().get("Content-Type").value() == "application/json");
    }

    SECTION("clear data") {
        Request req("https://example.com");
        req.set_data(std::vector<uint8_t>{1, 2, 3});

        REQUIRE(req.has_data());

        req.clear_data();

        REQUIRE_FALSE(req.has_data());
        REQUIRE_FALSE(req.headers().contains("Content-Type"));
        REQUIRE(req.method() == "GET");
    }
}

TEST_CASE("Request headers handling", "[networking][request]") {
    SECTION("set individual header") {
        Request req("https://example.com");
        req.set_header("X-Custom-Header", "custom value");

        REQUIRE(req.headers().get("X-Custom-Header").value() == "custom value");
    }

    SECTION("headers are case-insensitive") {
        Request req("https://example.com");
        req.set_header("content-type", "application/json");

        REQUIRE(req.headers().get("Content-Type").value() == "application/json");
    }
}

TEST_CASE("Request proxies handling", "[networking][request]") {
    SECTION("set proxies map") {
        Request req("https://example.com");

        std::map<std::string, std::string> proxies = {
            {"http", "http://proxy1.example.com"},
            {"https", "http://proxy2.example.com"}
        };

        req.set_proxies(proxies);

        REQUIRE(req.proxies().size() == 2);
        REQUIRE(req.proxies().at("http") == "http://proxy1.example.com");
        REQUIRE(req.proxies().at("https") == "http://proxy2.example.com");
    }

    SECTION("set individual proxy") {
        Request req("https://example.com");
        req.set_proxy("http", "http://proxy.example.com");
        req.set_proxy("all", "http://fallback.example.com");

        REQUIRE(req.proxies().size() == 2);
        REQUIRE(req.proxies().at("http") == "http://proxy.example.com");
        REQUIRE(req.proxies().at("all") == "http://fallback.example.com");
    }
}

TEST_CASE("Request extensions handling", "[networking][request]") {
    SECTION("set extension") {
        Request req("https://example.com");
        req.set_extension("timeout", 30);
        req.set_extension("verify_ssl", false);

        auto timeout = req.get_extension("timeout");
        REQUIRE(timeout != nullptr);
        REQUIRE(std::any_cast<int>(*timeout) == 30);

        auto verify = req.get_extension("verify_ssl");
        REQUIRE(verify != nullptr);
        REQUIRE(std::any_cast<bool>(*verify) == false);
    }

    SECTION("get non-existent extension") {
        Request req("https://example.com");

        auto ext = req.get_extension("nonexistent");
        REQUIRE(ext == nullptr);
    }
}

TEST_CASE("Request update method", "[networking][request]") {
    SECTION("update with new URL") {
        Request req("https://example.com/path");

        req.update("https://newexample.com/newpath");

        REQUIRE(req.url() == "https://newexample.com/newpath");
    }

    SECTION("update with headers") {
        Request req("https://example.com");

        std::map<std::string, std::string> headers = {
            {"X-New-Header", "value"}
        };

        req.update("", nullptr, headers);

        REQUIRE(req.headers().contains("X-New-Header"));
    }

    SECTION("update with query") {
        Request req("https://example.com/path");

        std::map<std::string, std::string> query = {
            {"param", "value"}
        };

        req.update("", nullptr, {}, query);

        REQUIRE(req.url().find("param=value") != std::string::npos);
    }

    SECTION("update with data") {
        Request req("https://example.com");

        RequestData data = std::vector<uint8_t>{1, 2, 3};
        req.update("", &data);

        REQUIRE(req.has_data());
    }
}

TEST_CASE("Request copy method", "[networking][request]") {
    SECTION("creates independent copy") {
        Request req1("https://example.com", "POST");
        req1.set_header("X-Custom", "value");
        req1.set_data(std::vector<uint8_t>{1, 2, 3});
        req1.set_proxy("http", "http://proxy.example.com");
        req1.set_extension("timeout", 30);

        Request req2 = req1.copy();

        REQUIRE(req2.url() == req1.url());
        REQUIRE(req2.method() == req1.method());
        REQUIRE(req2.headers().get("X-Custom").value() == "value");
        REQUIRE(req2.has_data());
        REQUIRE(req2.proxies().at("http") == "http://proxy.example.com");

        // Modify copy should not affect original
        req2.set_url("https://different.com");
        REQUIRE(req1.url() == "https://example.com");
    }
}

TEST_CASE("Request factory functions", "[networking][request]") {
    SECTION("make_head_request") {
        auto req = make_head_request("https://example.com");

        REQUIRE(req.method() == "HEAD");
        REQUIRE(req.has_explicit_method());
    }

    SECTION("make_post_request") {
        std::vector<uint8_t> data = {1, 2, 3};
        auto req = make_post_request("https://example.com", data);

        REQUIRE(req.method() == "POST");
        REQUIRE(req.has_data());
    }

    SECTION("make_put_request") {
        std::vector<uint8_t> data = {1, 2, 3};
        auto req = make_put_request("https://example.com", data);

        REQUIRE(req.method() == "PUT");
        REQUIRE(req.has_data());
    }

    SECTION("make_patch_request") {
        std::vector<uint8_t> data = {1, 2, 3};
        auto req = make_patch_request("https://example.com", data);

        REQUIRE(req.method() == "PATCH");
        REQUIRE(req.has_data());
    }
}
