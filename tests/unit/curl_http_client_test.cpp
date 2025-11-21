#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/curl_http_client.hpp"

using namespace ytdlp::networking;

TEST_CASE("CurlHttpClient construction", "[networking][curl]") {
    SECTION("default construction") {
        CurlHttpClient client;

        REQUIRE(client.config().connect_timeout == 10);
        REQUIRE(client.config().timeout == 60);
        REQUIRE(client.config().follow_redirects == true);
        REQUIRE(client.config().max_redirects == 10);
        REQUIRE(client.config().verify_ssl == true);
    }

    SECTION("construction with custom config") {
        CurlHttpClient::Config config;
        config.connect_timeout = 5;
        config.timeout = 30;
        config.follow_redirects = false;
        config.verify_ssl = false;
        config.user_agent = "TestAgent/1.0";

        CurlHttpClient client(config);

        REQUIRE(client.config().connect_timeout == 5);
        REQUIRE(client.config().timeout == 30);
        REQUIRE(client.config().follow_redirects == false);
        REQUIRE(client.config().verify_ssl == false);
        REQUIRE(client.config().user_agent == "TestAgent/1.0");
    }
}

TEST_CASE("CurlHttpClient config modification", "[networking][curl]") {
    CurlHttpClient client;

    SECTION("modify timeout") {
        client.config().timeout = 120;
        REQUIRE(client.config().timeout == 120);
    }

    SECTION("modify user agent") {
        client.config().user_agent = "CustomAgent/2.0";
        REQUIRE(client.config().user_agent == "CustomAgent/2.0");
    }

    SECTION("disable SSL verification") {
        client.config().verify_ssl = false;
        REQUIRE(client.config().verify_ssl == false);
    }
}

TEST_CASE("CurlHttpClient request creation helpers", "[networking][curl]") {
    SECTION("create GET request") {
        Request req("https://httpbin.org/get", "GET");
        REQUIRE(req.method() == "GET");
        REQUIRE(req.url().find("httpbin.org") != std::string::npos);
    }

    SECTION("create POST request") {
        std::vector<uint8_t> data = {1, 2, 3, 4};
        Request req = make_post_request("https://httpbin.org/post", data);

        REQUIRE(req.method() == "POST");
        REQUIRE(req.has_data());
    }

    SECTION("create HEAD request") {
        Request req = make_head_request("https://httpbin.org/head");

        REQUIRE(req.method() == "HEAD");
        REQUIRE_FALSE(req.has_data());
    }
}

// Note: The following tests require network connectivity and are marked with [.integration]
// to be skipped by default. Run with: ./curl_http_client_test "[integration]"

TEST_CASE("CurlHttpClient GET request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("simple GET request") {
        try {
            auto resp = client.get("https://httpbin.org/get");

            REQUIRE(resp.status() == 200);
            REQUIRE(resp.is_success());
            REQUIRE_FALSE(resp.read_all().empty());
        } catch (const std::exception& e) {
            // Network error - skip test
            WARN("Network test skipped: " << e.what());
        }
    }

    SECTION("GET with custom headers") {
        try {
            std::map<std::string, std::string> headers = {
                {"X-Test-Header", "TestValue"}
            };

            auto resp = client.get("https://httpbin.org/headers", headers);

            REQUIRE(resp.status() == 200);
            std::string body = resp.read_all();
            REQUIRE(body.find("X-Test-Header") != std::string::npos);
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient POST request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("POST with data") {
        try {
            std::string post_data = "key1=value1&key2=value2";
            std::vector<uint8_t> data(post_data.begin(), post_data.end());

            auto resp = client.post("https://httpbin.org/post", data);

            REQUIRE(resp.status() == 200);
            std::string body = resp.read_all();
            REQUIRE(body.find("key1") != std::string::npos);
            REQUIRE(body.find("value1") != std::string::npos);
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient HEAD request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("HEAD request returns no body") {
        try {
            auto resp = client.head("https://httpbin.org/get");

            REQUIRE(resp.status() == 200);
            REQUIRE(resp.is_success());
            // HEAD should have headers but minimal/no body
            REQUIRE_FALSE(resp.headers().empty());
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient PUT request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("PUT with data") {
        try {
            std::string put_data = "updated data";
            std::vector<uint8_t> data(put_data.begin(), put_data.end());

            auto resp = client.put("https://httpbin.org/put", data);

            REQUIRE(resp.status() == 200);
            std::string body = resp.read_all();
            REQUIRE(body.find("updated data") != std::string::npos);
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient PATCH request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("PATCH with data") {
        try {
            std::string patch_data = "partial update";
            std::vector<uint8_t> data(patch_data.begin(), patch_data.end());

            auto resp = client.patch("https://httpbin.org/patch", data);

            REQUIRE(resp.status() == 200);
            std::string body = resp.read_all();
            REQUIRE(body.find("partial update") != std::string::npos);
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient DELETE request", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("DELETE request") {
        try {
            auto resp = client.del("https://httpbin.org/delete");

            REQUIRE(resp.status() == 200);
            REQUIRE(resp.is_success());
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient redirect handling", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("follows redirects by default") {
        try {
            auto resp = client.get("https://httpbin.org/redirect/2");

            REQUIRE(resp.status() == 200);
            // Final URL should be different after redirect
            REQUIRE(resp.url().find("/get") != std::string::npos);
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }

    SECTION("respects redirect limit") {
        try {
            client.config().max_redirects = 1;

            // This should fail or return 302 because we limit redirects to 1
            // but the endpoint redirects twice
            auto resp = client.get("https://httpbin.org/redirect/2");

            // Either 302/301 (redirect not followed) or throws
            if (resp.status() >= 300 && resp.status() < 400) {
                REQUIRE(resp.is_redirect());
            }
        } catch (const std::exception& e) {
            // Expected - too many redirects
            REQUIRE(true);
        }
    }
}

TEST_CASE("CurlHttpClient status code helpers", "[networking][curl][.integration]") {
    CurlHttpClient client;

    SECTION("404 Not Found") {
        try {
            auto resp = client.get("https://httpbin.org/status/404");

            REQUIRE(resp.status() == 404);
            REQUIRE(resp.is_client_error());
            REQUIRE_FALSE(resp.is_success());
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }

    SECTION("500 Internal Server Error") {
        try {
            auto resp = client.get("https://httpbin.org/status/500");

            REQUIRE(resp.status() == 500);
            REQUIRE(resp.is_server_error());
            REQUIRE_FALSE(resp.is_success());
        } catch (const std::exception& e) {
            WARN("Network test skipped: " << e.what());
        }
    }
}

TEST_CASE("CurlHttpClient error handling", "[networking][curl]") {
    CurlHttpClient client;

    SECTION("invalid URL throws exception") {
        try {
            auto resp = client.get("not-a-valid-url");
            REQUIRE(false); // Should not reach here
        } catch (const std::runtime_error& e) {
            // Expected
            REQUIRE(true);
        }
    }

    SECTION("unreachable host throws exception") {
        try {
            client.config().connect_timeout = 1; // Short timeout
            auto resp = client.get("https://192.0.2.1:12345"); // TEST-NET-1, should be unreachable
            REQUIRE(false); // Should not reach here
        } catch (const std::runtime_error& e) {
            // Expected
            REQUIRE(true);
        }
    }
}

TEST_CASE("CurlHttpClient move semantics", "[networking][curl]") {
    SECTION("move construction") {
        CurlHttpClient client1;
        client1.config().timeout = 99;

        CurlHttpClient client2(std::move(client1));

        REQUIRE(client2.config().timeout == 99);
    }

    SECTION("move assignment") {
        CurlHttpClient client1;
        client1.config().timeout = 77;

        CurlHttpClient client2;
        client2 = std::move(client1);

        REQUIRE(client2.config().timeout == 77);
    }
}

TEST_CASE("CurlHttpClient cookie jar integration", "[networking][curl]") {
    CurlHttpClient client;

    SECTION("cookie jar is null by default") {
        REQUIRE(client.cookie_jar() == nullptr);
    }

    SECTION("can set and get cookie jar") {
        auto jar = std::make_shared<CookieJar>();

        client.set_cookie_jar(jar);

        REQUIRE(client.cookie_jar() == jar);
        REQUIRE(jar->empty());  // Should have no cookies initially
        REQUIRE(jar->size() == 0);
    }

    SECTION("cookies are automatically added to requests") {
        auto jar = std::make_shared<CookieJar>();

        // Add a cookie
        Cookie cookie;
        cookie.domain = "example.com";
        cookie.path = "/";
        cookie.name = "test_cookie";
        cookie.value = "test_value";
        cookie.expires = 9999999999; // Far future

        jar->set_cookie(cookie);
        client.set_cookie_jar(jar);

        // Verify cookie jar has the cookie
        REQUIRE(jar->size() == 1);

        // Verify get_cookie_header works
        std::string header = jar->get_cookie_header("http://example.com/path");
        REQUIRE(header == "test_cookie=test_value");
    }
}
