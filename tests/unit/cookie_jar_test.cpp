#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/cookie_jar.hpp"
#include <fstream>
#include <ctime>
#include <filesystem>

using namespace ytdlp::networking;

TEST_CASE("CookieJar construction", "[networking][cookie_jar]") {
    SECTION("default construction") {
        CookieJar jar;

        REQUIRE(jar.empty());
        REQUIRE(jar.size() == 0);
    }
}

TEST_CASE("CookieJar set_cookie and get_all_cookies", "[networking][cookie_jar]") {
    CookieJar jar;

    SECTION("add single cookie") {
        Cookie cookie;
        cookie.domain = "example.com";
        cookie.path = "/";
        cookie.name = "session";
        cookie.value = "abc123";

        jar.set_cookie(cookie);

        REQUIRE(jar.size() == 1);
        REQUIRE_FALSE(jar.empty());

        auto cookies = jar.get_all_cookies();
        REQUIRE(cookies.size() == 1);
        REQUIRE(cookies[0].name == "session");
        REQUIRE(cookies[0].value == "abc123");
    }

    SECTION("add multiple cookies") {
        Cookie c1, c2, c3;

        c1.domain = "example.com";
        c1.path = "/";
        c1.name = "cookie1";
        c1.value = "value1";

        c2.domain = "example.com";
        c2.path = "/path";
        c2.name = "cookie2";
        c2.value = "value2";

        c3.domain = "other.com";
        c3.path = "/";
        c3.name = "cookie3";
        c3.value = "value3";

        jar.set_cookie(c1);
        jar.set_cookie(c2);
        jar.set_cookie(c3);

        REQUIRE(jar.size() == 3);
    }

    SECTION("replace existing cookie") {
        Cookie c1;
        c1.domain = "example.com";
        c1.path = "/";
        c1.name = "session";
        c1.value = "old_value";

        jar.set_cookie(c1);
        REQUIRE(jar.size() == 1);

        // Replace with same domain, path, name
        Cookie c2;
        c2.domain = "example.com";
        c2.path = "/";
        c2.name = "session";
        c2.value = "new_value";

        jar.set_cookie(c2);
        REQUIRE(jar.size() == 1); // Still 1 cookie

        auto cookies = jar.get_all_cookies();
        REQUIRE(cookies[0].value == "new_value");
    }
}

TEST_CASE("CookieJar get_cookies_for_url", "[networking][cookie_jar]") {
    CookieJar jar;

    Cookie c1, c2, c3, c4;

    c1.domain = "example.com";
    c1.path = "/";
    c1.name = "cookie1";
    c1.value = "value1";

    c2.domain = "example.com";
    c2.path = "/admin";
    c2.name = "cookie2";
    c2.value = "value2";

    c3.domain = ".example.com";
    c3.domain_initial_dot = true;
    c3.path = "/";
    c3.name = "cookie3";
    c3.value = "value3";

    c4.domain = "other.com";
    c4.path = "/";
    c4.name = "cookie4";
    c4.value = "value4";

    jar.set_cookie(c1);
    jar.set_cookie(c2);
    jar.set_cookie(c3);
    jar.set_cookie(c4);

    SECTION("match by domain and path") {
        auto cookies = jar.get_cookies_for_url("http://example.com/");

        REQUIRE(cookies.size() == 2); // c1 and c3
        // Check that both cookies are present
        bool found_c1 = false, found_c3 = false;
        for (const auto& cookie : cookies) {
            if (cookie.name == "cookie1") found_c1 = true;
            if (cookie.name == "cookie3") found_c3 = true;
        }
        REQUIRE(found_c1);
        REQUIRE(found_c3);
    }

    SECTION("match specific path") {
        auto cookies = jar.get_cookies_for_url("http://example.com/admin/panel");

        REQUIRE(cookies.size() == 3); // c1, c2, c3
        // c2 should be first (longest path)
        REQUIRE(cookies[0].name == "cookie2");
    }

    SECTION("subdomain matching") {
        auto cookies = jar.get_cookies_for_url("http://www.example.com/");

        REQUIRE(cookies.size() == 1); // only c3 (has domain_initial_dot)
        REQUIRE(cookies[0].name == "cookie3");
    }

    SECTION("no match") {
        auto cookies = jar.get_cookies_for_url("http://unrelated.com/");

        REQUIRE(cookies.empty());
    }

    SECTION("secure cookie on HTTP") {
        Cookie secure_cookie;
        secure_cookie.domain = "example.com";
        secure_cookie.path = "/";
        secure_cookie.name = "secure";
        secure_cookie.value = "value";
        secure_cookie.secure = true;

        jar.set_cookie(secure_cookie);

        auto http_cookies = jar.get_cookies_for_url("http://example.com/");
        auto https_cookies = jar.get_cookies_for_url("https://example.com/");

        // Secure cookie should not appear in HTTP request
        bool found_in_http = false;
        for (const auto& cookie : http_cookies) {
            if (cookie.name == "secure") found_in_http = true;
        }
        REQUIRE_FALSE(found_in_http);

        // But should appear in HTTPS request
        bool found_in_https = false;
        for (const auto& cookie : https_cookies) {
            if (cookie.name == "secure") found_in_https = true;
        }
        REQUIRE(found_in_https);
    }
}

TEST_CASE("CookieJar get_cookie_header", "[networking][cookie_jar]") {
    CookieJar jar;

    SECTION("no cookies") {
        std::string header = jar.get_cookie_header("http://example.com/");
        REQUIRE(header.empty());
    }

    SECTION("single cookie") {
        Cookie cookie;
        cookie.domain = "example.com";
        cookie.path = "/";
        cookie.name = "session";
        cookie.value = "abc123";

        jar.set_cookie(cookie);

        std::string header = jar.get_cookie_header("http://example.com/");
        REQUIRE(header == "session=abc123");
    }

    SECTION("multiple cookies") {
        Cookie c1, c2;

        c1.domain = "example.com";
        c1.path = "/";
        c1.name = "cookie1";
        c1.value = "value1";

        c2.domain = "example.com";
        c2.path = "/";
        c2.name = "cookie2";
        c2.value = "value2";

        jar.set_cookie(c1);
        jar.set_cookie(c2);

        std::string header = jar.get_cookie_header("http://example.com/");

        // Header should contain both cookies separated by "; "
        REQUIRE(header.find("cookie1=value1") != std::string::npos);
        REQUIRE(header.find("cookie2=value2") != std::string::npos);
        REQUIRE(header.find("; ") != std::string::npos);
    }

    SECTION("cookie ordering by path length") {
        Cookie c1, c2;

        c1.domain = "example.com";
        c1.path = "/";
        c1.name = "root";
        c1.value = "value1";

        c2.domain = "example.com";
        c2.path = "/admin";
        c2.name = "admin";
        c2.value = "value2";

        jar.set_cookie(c1);
        jar.set_cookie(c2);

        std::string header = jar.get_cookie_header("http://example.com/admin/panel");

        // Longer path should come first
        REQUIRE(header.find("admin=value2") < header.find("root=value1"));
    }
}

TEST_CASE("CookieJar save and load Netscape format", "[networking][cookie_jar]") {
    std::string test_file = "/tmp/test_cookies_" + std::to_string(std::time(nullptr)) + ".txt";

    SECTION("save and load cookies") {
        CookieJar jar1;

        Cookie c1, c2, c3;

        c1.domain = "example.com";
        c1.domain_initial_dot = false;
        c1.path = "/";
        c1.secure = false;
        c1.name = "session";
        c1.value = "abc123";
        c1.expires = 9999999999; // Far future

        c2.domain = ".subdomain.com";
        c2.domain_initial_dot = true;
        c2.path = "/path";
        c2.secure = true;
        c2.name = "secure_cookie";
        c2.value = "xyz789";
        c2.expires = 9999999999;

        c3.domain = "example.com";
        c3.path = "/";
        c3.name = "session_cookie";
        c3.value = "temporary";
        c3.expires = std::nullopt; // Session cookie
        c3.discard = true;

        jar1.set_cookie(c1);
        jar1.set_cookie(c2);
        jar1.set_cookie(c3);

        // Save (ignore_discard=true saves session cookies)
        jar1.save(test_file, true, false);

        // Load into new jar
        CookieJar jar2;
        jar2.load(test_file);

        REQUIRE(jar2.size() == 3);

        auto cookies = jar2.get_all_cookies();

        bool found_session = false;
        bool found_secure = false;
        bool found_session_cookie = false;

        for (const auto& cookie : cookies) {
            if (cookie.name == "session") {
                found_session = true;
                REQUIRE(cookie.domain == "example.com");
                REQUIRE(cookie.path == "/");
                REQUIRE(cookie.value == "abc123");
                REQUIRE(cookie.secure == false);
            } else if (cookie.name == "secure_cookie") {
                found_secure = true;
                REQUIRE(cookie.domain == ".subdomain.com");
                REQUIRE(cookie.domain_initial_dot == true);
                REQUIRE(cookie.path == "/path");
                REQUIRE(cookie.secure == true);
            } else if (cookie.name == "session_cookie") {
                found_session_cookie = true;
            }
        }

        REQUIRE(found_session);
        REQUIRE(found_secure);
        REQUIRE(found_session_cookie);

        // Clean up
        std::filesystem::remove(test_file);
    }

    SECTION("load HttpOnly cookies") {
        // Manually create a cookie file with HttpOnly prefix
        std::ofstream file(test_file);
        file << "# Netscape HTTP Cookie File\n";
        file << "#HttpOnly_example.com\tFALSE\t/\tFALSE\t9999999999\thttponly_cookie\tvalue\n";
        file.close();

        CookieJar jar;
        jar.load(test_file);

        REQUIRE(jar.size() == 1);

        auto cookies = jar.get_all_cookies();
        REQUIRE(cookies[0].name == "httponly_cookie");
        REQUIRE(cookies[0].http_only == true);

        std::filesystem::remove(test_file);
    }

    SECTION("save HttpOnly cookies") {
        CookieJar jar;

        Cookie cookie;
        cookie.domain = "example.com";
        cookie.path = "/";
        cookie.name = "httponly";
        cookie.value = "value";
        cookie.http_only = true;
        cookie.expires = 9999999999;

        jar.set_cookie(cookie);
        jar.save(test_file);

        // Read file and check for HttpOnly prefix
        std::ifstream file(test_file);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        REQUIRE(content.find("#HttpOnly_") != std::string::npos);

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("CookieJar clear and empty", "[networking][cookie_jar]") {
    CookieJar jar;

    Cookie cookie;
    cookie.domain = "example.com";
    cookie.path = "/";
    cookie.name = "test";
    cookie.value = "value";

    jar.set_cookie(cookie);
    REQUIRE_FALSE(jar.empty());
    REQUIRE(jar.size() == 1);

    jar.clear();
    REQUIRE(jar.empty());
    REQUIRE(jar.size() == 0);
}

TEST_CASE("CookieJar remove_expired", "[networking][cookie_jar]") {
    CookieJar jar;

    Cookie c1, c2, c3;

    c1.domain = "example.com";
    c1.path = "/";
    c1.name = "valid";
    c1.value = "value1";
    c1.expires = 9999999999; // Far future

    c2.domain = "example.com";
    c2.path = "/";
    c2.name = "expired";
    c2.value = "value2";
    c2.expires = 1000; // Past

    c3.domain = "example.com";
    c3.path = "/";
    c3.name = "session";
    c3.value = "value3";
    c3.expires = std::nullopt; // Session cookie

    jar.set_cookie(c1);
    jar.set_cookie(c2);
    jar.set_cookie(c3);

    REQUIRE(jar.size() == 3);

    size_t removed = jar.remove_expired();

    REQUIRE(removed == 1); // Only c2 should be removed
    REQUIRE(jar.size() == 2);

    auto cookies = jar.get_all_cookies();
    for (const auto& cookie : cookies) {
        REQUIRE(cookie.name != "expired");
    }
}

TEST_CASE("CookieJar merge", "[networking][cookie_jar]") {
    CookieJar jar1, jar2;

    Cookie c1, c2, c3;

    c1.domain = "example.com";
    c1.path = "/";
    c1.name = "cookie1";
    c1.value = "value1";

    c2.domain = "example.com";
    c2.path = "/";
    c2.name = "cookie2";
    c2.value = "value2";

    c3.domain = "other.com";
    c3.path = "/";
    c3.name = "cookie3";
    c3.value = "value3";

    jar1.set_cookie(c1);
    jar1.set_cookie(c2);

    jar2.set_cookie(c3);

    SECTION("merge different cookies") {
        jar1.merge(jar2);

        REQUIRE(jar1.size() == 3);

        auto cookies = jar1.get_all_cookies();
        bool found_c3 = false;
        for (const auto& cookie : cookies) {
            if (cookie.name == "cookie3") {
                found_c3 = true;
            }
        }
        REQUIRE(found_c3);
    }

    SECTION("merge overwrites existing") {
        Cookie c1_new;
        c1_new.domain = "example.com";
        c1_new.path = "/";
        c1_new.name = "cookie1";
        c1_new.value = "new_value";

        jar2.set_cookie(c1_new);
        jar1.merge(jar2);

        auto cookies = jar1.get_all_cookies();
        for (const auto& cookie : cookies) {
            if (cookie.name == "cookie1") {
                REQUIRE(cookie.value == "new_value");
            }
        }
    }
}

TEST_CASE("CookieJar URL parsing edge cases", "[networking][cookie_jar]") {
    CookieJar jar;

    Cookie cookie;
    cookie.domain = "example.com";
    cookie.path = "/path";
    cookie.name = "test";
    cookie.value = "value";

    jar.set_cookie(cookie);

    SECTION("URL with port") {
        auto cookies = jar.get_cookies_for_url("http://example.com:8080/path");
        REQUIRE(cookies.size() == 1);
    }

    SECTION("URL with query string") {
        auto cookies = jar.get_cookies_for_url("http://example.com/path?query=value");
        REQUIRE(cookies.size() == 1);
    }

    SECTION("URL with fragment") {
        auto cookies = jar.get_cookies_for_url("http://example.com/path#fragment");
        REQUIRE(cookies.size() == 1);
    }

    SECTION("HTTPS scheme") {
        auto cookies = jar.get_cookies_for_url("https://example.com/path");
        REQUIRE(cookies.size() == 1);
    }
}
