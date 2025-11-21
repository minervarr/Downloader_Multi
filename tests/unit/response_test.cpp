#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/response.hpp"
#include <sstream>

using namespace ytdlp::networking;

TEST_CASE("Response basic construction", "[networking][response]") {
    SECTION("construct with minimal parameters") {
        auto body = std::make_shared<std::istringstream>("test body");
        std::map<std::string, std::string> headers = {
            {"Content-Type", "text/plain"}
        };

        Response resp(body, "https://example.com", headers);

        REQUIRE(resp.status() == 200);
        REQUIRE(resp.reason() == "OK");
        REQUIRE(resp.url() == "https://example.com");
        REQUIRE(resp.headers().contains("Content-Type"));
    }

    SECTION("construct with custom status") {
        auto body = std::make_shared<std::istringstream>("");
        Response resp(body, "https://example.com", {}, 404);

        REQUIRE(resp.status() == 404);
        REQUIRE(resp.reason() == "Not Found");
    }

    SECTION("construct with custom reason") {
        auto body = std::make_shared<std::istringstream>("");
        Response resp(body, "https://example.com", {}, 200, "Custom Reason");

        REQUIRE(resp.status() == 200);
        REQUIRE(resp.reason() == "Custom Reason");
    }

    SECTION("construct with extensions") {
        auto body = std::make_shared<std::istringstream>("");
        std::map<std::string, std::any> extensions = {
            {"custom_key", std::string("custom_value")}
        };

        Response resp(body, "https://example.com", {}, 200, "", extensions);

        auto ext = resp.get_extension("custom_key");
        REQUIRE(ext != nullptr);
        REQUIRE(std::any_cast<std::string>(*ext) == "custom_value");
    }
}

TEST_CASE("Response status code helpers", "[networking][response]") {
    auto body = std::make_shared<std::istringstream>("");

    SECTION("is_success for 2xx status") {
        Response resp200(body, "https://example.com", {}, 200);
        REQUIRE(resp200.is_success());

        Response resp201(body, "https://example.com", {}, 201);
        REQUIRE(resp201.is_success());

        Response resp204(body, "https://example.com", {}, 204);
        REQUIRE(resp204.is_success());
    }

    SECTION("is_redirect for 3xx status") {
        Response resp301(body, "https://example.com", {}, 301);
        REQUIRE(resp301.is_redirect());

        Response resp302(body, "https://example.com", {}, 302);
        REQUIRE(resp302.is_redirect());

        Response resp307(body, "https://example.com", {}, 307);
        REQUIRE(resp307.is_redirect());
    }

    SECTION("is_client_error for 4xx status") {
        Response resp400(body, "https://example.com", {}, 400);
        REQUIRE(resp400.is_client_error());

        Response resp404(body, "https://example.com", {}, 404);
        REQUIRE(resp404.is_client_error());

        Response resp429(body, "https://example.com", {}, 429);
        REQUIRE(resp429.is_client_error());
    }

    SECTION("is_server_error for 5xx status") {
        Response resp500(body, "https://example.com", {}, 500);
        REQUIRE(resp500.is_server_error());

        Response resp502(body, "https://example.com", {}, 502);
        REQUIRE(resp502.is_server_error());

        Response resp503(body, "https://example.com", {}, 503);
        REQUIRE(resp503.is_server_error());
    }

    SECTION("non-overlapping status categories") {
        Response resp(body, "https://example.com", {}, 200);
        REQUIRE(resp.is_success());
        REQUIRE_FALSE(resp.is_redirect());
        REQUIRE_FALSE(resp.is_client_error());
        REQUIRE_FALSE(resp.is_server_error());
    }
}

TEST_CASE("Response default reason phrases", "[networking][response]") {
    auto body = std::make_shared<std::istringstream>("");

    SECTION("common 2xx statuses") {
        Response resp200(body, "https://example.com", {}, 200);
        REQUIRE(resp200.reason() == "OK");

        Response resp201(body, "https://example.com", {}, 201);
        REQUIRE(resp201.reason() == "Created");

        Response resp204(body, "https://example.com", {}, 204);
        REQUIRE(resp204.reason() == "No Content");
    }

    SECTION("common 3xx statuses") {
        Response resp301(body, "https://example.com", {}, 301);
        REQUIRE(resp301.reason() == "Moved Permanently");

        Response resp302(body, "https://example.com", {}, 302);
        REQUIRE(resp302.reason() == "Found");

        Response resp304(body, "https://example.com", {}, 304);
        REQUIRE(resp304.reason() == "Not Modified");
    }

    SECTION("common 4xx statuses") {
        Response resp400(body, "https://example.com", {}, 400);
        REQUIRE(resp400.reason() == "Bad Request");

        Response resp401(body, "https://example.com", {}, 401);
        REQUIRE(resp401.reason() == "Unauthorized");

        Response resp403(body, "https://example.com", {}, 403);
        REQUIRE(resp403.reason() == "Forbidden");

        Response resp404(body, "https://example.com", {}, 404);
        REQUIRE(resp404.reason() == "Not Found");

        Response resp418(body, "https://example.com", {}, 418);
        REQUIRE(resp418.reason() == "I'm a teapot");
    }

    SECTION("common 5xx statuses") {
        Response resp500(body, "https://example.com", {}, 500);
        REQUIRE(resp500.reason() == "Internal Server Error");

        Response resp502(body, "https://example.com", {}, 502);
        REQUIRE(resp502.reason() == "Bad Gateway");

        Response resp503(body, "https://example.com", {}, 503);
        REQUIRE(resp503.reason() == "Service Unavailable");
    }

    SECTION("unknown status codes get generic reasons") {
        Response resp299(body, "https://example.com", {}, 299);
        REQUIRE(resp299.reason() == "Success");

        Response resp399(body, "https://example.com", {}, 399);
        REQUIRE(resp399.reason() == "Redirection");

        Response resp499(body, "https://example.com", {}, 499);
        REQUIRE(resp499.reason() == "Client Error");

        Response resp599(body, "https://example.com", {}, 599);
        REQUIRE(resp599.reason() == "Server Error");
    }
}

TEST_CASE("Response header handling", "[networking][response]") {
    auto body = std::make_shared<std::istringstream>("");

    SECTION("get header") {
        std::map<std::string, std::string> headers = {
            {"Content-Type", "text/html"},
            {"Content-Length", "1234"}
        };

        Response resp(body, "https://example.com", headers);

        REQUIRE(resp.get_header("Content-Type") == "text/html");
        REQUIRE(resp.get_header("Content-Length") == "1234");
    }

    SECTION("get header with default") {
        Response resp(body, "https://example.com", {});

        REQUIRE(resp.get_header("NonExistent", "default") == "default");
    }

    SECTION("headers are case-insensitive") {
        std::map<std::string, std::string> headers = {
            {"Content-Type", "text/html"}
        };

        Response resp(body, "https://example.com", headers);

        REQUIRE(resp.get_header("content-type") == "text/html");
        REQUIRE(resp.get_header("CONTENT-TYPE") == "text/html");
    }
}

TEST_CASE("Response body reading", "[networking][response]") {
    SECTION("read_all reads entire body") {
        auto body = std::make_shared<std::istringstream>("Hello, World!");
        Response resp(body, "https://example.com", {});

        std::string content = resp.read_all();
        REQUIRE(content == "Hello, World!");
    }

    SECTION("read specific number of bytes") {
        auto body = std::make_shared<std::istringstream>("Hello, World!");
        Response resp(body, "https://example.com", {});

        char buffer[5];
        size_t bytes_read = resp.read(buffer, 5);

        REQUIRE(bytes_read == 5);
        REQUIRE(std::string(buffer, 5) == "Hello");
    }

    SECTION("read_bytes with size") {
        auto body = std::make_shared<std::istringstream>("Hello, World!");
        Response resp(body, "https://example.com", {});

        auto bytes = resp.read_bytes(5);

        REQUIRE(bytes.size() == 5);
        REQUIRE(std::string(bytes.begin(), bytes.end()) == "Hello");
    }

    SECTION("readable returns true for valid stream") {
        auto body = std::make_shared<std::istringstream>("test");
        Response resp(body, "https://example.com", {});

        REQUIRE(resp.readable());
    }

    SECTION("close resets the stream") {
        auto body = std::make_shared<std::istringstream>("test");
        Response resp(body, "https://example.com", {});

        resp.close();

        // After close, stream should not be readable
        REQUIRE_FALSE(resp.readable());
    }
}

TEST_CASE("Response extensions", "[networking][response]") {
    auto body = std::make_shared<std::istringstream>("");

    SECTION("get extension") {
        std::map<std::string, std::any> extensions = {
            {"redirect_count", 3},
            {"cached", true}
        };

        Response resp(body, "https://example.com", {}, 200, "", extensions);

        auto redirect_count = resp.get_extension("redirect_count");
        REQUIRE(redirect_count != nullptr);
        REQUIRE(std::any_cast<int>(*redirect_count) == 3);

        auto cached = resp.get_extension("cached");
        REQUIRE(cached != nullptr);
        REQUIRE(std::any_cast<bool>(*cached) == true);
    }

    SECTION("get non-existent extension") {
        Response resp(body, "https://example.com", {});

        auto ext = resp.get_extension("nonexistent");
        REQUIRE(ext == nullptr);
    }
}

TEST_CASE("Response URL", "[networking][response]") {
    auto body = std::make_shared<std::istringstream>("");

    SECTION("stores final URL after redirects") {
        Response resp(body, "https://final-url.com/path", {});

        REQUIRE(resp.url() == "https://final-url.com/path");
    }
}
