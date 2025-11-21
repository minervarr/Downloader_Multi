#include <catch2/catch_test_macros.hpp>
#include "ytdlp/networking/http_header_dict.hpp"

using namespace ytdlp::networking;

TEST_CASE("HTTPHeaderDict handles case-insensitive keys", "[networking][http]") {
    HTTPHeaderDict headers;

    SECTION("set and get with different cases") {
        headers.set("content-type", "text/html");

        REQUIRE(headers.get("content-type").value() == "text/html");
        REQUIRE(headers.get("Content-Type").value() == "text/html");
        REQUIRE(headers.get("CONTENT-TYPE").value() == "text/html");
        REQUIRE(headers.get("CoNtEnT-TyPe").value() == "text/html");
    }

    SECTION("overwrite with different case") {
        headers.set("content-type", "text/html");
        headers.set("Content-Type", "application/json");

        REQUIRE(headers.get("content-type").value() == "application/json");
        REQUIRE(headers.size() == 1);
    }

    SECTION("contains is case-insensitive") {
        headers.set("user-agent", "Mozilla/5.0");

        REQUIRE(headers.contains("user-agent"));
        REQUIRE(headers.contains("User-Agent"));
        REQUIRE(headers.contains("USER-AGENT"));
        REQUIRE_FALSE(headers.contains("content-type"));
    }
}

TEST_CASE("HTTPHeaderDict preserves original casing", "[networking][http]") {
    HTTPHeaderDict headers;

    SECTION("sensitive() returns original casing") {
        headers.set("content-type", "text/html");
        headers.set("User-Agent", "Mozilla/5.0");
        headers.set("X-Custom-HEADER", "value");

        auto sensitive = headers.sensitive();

        REQUIRE(sensitive.size() == 3);
        REQUIRE(sensitive["content-type"] == "text/html");
        REQUIRE(sensitive["User-Agent"] == "Mozilla/5.0");
        REQUIRE(sensitive["X-Custom-HEADER"] == "value");
    }

    SECTION("last set casing is preserved") {
        headers.set("content-type", "text/html");
        headers.set("Content-Type", "application/json");

        auto sensitive = headers.sensitive();
        REQUIRE(sensitive.count("Content-Type") == 1);
        REQUIRE(sensitive.count("content-type") == 0);
    }
}

TEST_CASE("HTTPHeaderDict basic operations", "[networking][http]") {
    HTTPHeaderDict headers;

    SECTION("get with default") {
        headers.set("content-type", "text/html");

        REQUIRE(headers.get("content-type", "default") == "text/html");
        REQUIRE(headers.get("missing", "default") == "default");
        REQUIRE_FALSE(headers.get("missing").has_value());
    }

    SECTION("remove headers") {
        headers.set("content-type", "text/html");
        headers.set("user-agent", "Mozilla/5.0");

        REQUIRE(headers.remove("Content-Type"));
        REQUIRE_FALSE(headers.contains("content-type"));
        REQUIRE(headers.size() == 1);

        REQUIRE_FALSE(headers.remove("non-existent"));
    }

    SECTION("pop headers") {
        headers.set("content-type", "text/html");
        headers.set("user-agent", "Mozilla/5.0");

        auto value = headers.pop("Content-Type");
        REQUIRE(value.has_value());
        REQUIRE(value.value() == "text/html");
        REQUIRE_FALSE(headers.contains("content-type"));
        REQUIRE(headers.size() == 1);

        REQUIRE_FALSE(headers.pop("non-existent").has_value());
        REQUIRE(headers.pop("non-existent", "default") == "default");
    }

    SECTION("setdefault only sets if not exists") {
        headers.set("content-type", "text/html");

        auto result1 = headers.setdefault("content-type", "application/json");
        REQUIRE(result1 == "text/html");
        REQUIRE(headers.get("content-type").value() == "text/html");

        auto result2 = headers.setdefault("user-agent", "Mozilla/5.0");
        REQUIRE(result2 == "Mozilla/5.0");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0");
    }

    SECTION("clear removes all headers") {
        headers.set("content-type", "text/html");
        headers.set("user-agent", "Mozilla/5.0");
        headers.set("accept", "application/json");

        headers.clear();
        REQUIRE(headers.empty());
        REQUIRE(headers.size() == 0);
        REQUIRE(headers.sensitive().empty());
    }
}

TEST_CASE("HTTPHeaderDict construction", "[networking][http]") {
    SECTION("construct from map") {
        std::map<std::string, std::string> headers_map = {
            {"content-type", "text/html"},
            {"User-Agent", "Mozilla/5.0"},
            {"Accept", "application/json"}
        };

        HTTPHeaderDict headers(headers_map);

        REQUIRE(headers.size() == 3);
        REQUIRE(headers.get("content-type").value() == "text/html");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(headers.get("accept").value() == "application/json");
    }

    SECTION("construct from multiple maps (later takes precedence)") {
        std::map<std::string, std::string> map1 = {
            {"content-type", "text/html"},
            {"user-agent", "Mozilla/5.0"}
        };

        std::map<std::string, std::string> map2 = {
            {"content-type", "application/json"},
            {"accept", "text/plain"}
        };

        HTTPHeaderDict headers(std::vector<std::map<std::string, std::string>>{map1, map2});

        REQUIRE(headers.size() == 3);
        REQUIRE(headers.get("content-type").value() == "application/json");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(headers.get("accept").value() == "text/plain");
    }
}

TEST_CASE("HTTPHeaderDict update operations", "[networking][http]") {
    SECTION("update from map") {
        HTTPHeaderDict headers;
        headers.set("content-type", "text/html");

        std::map<std::string, std::string> updates = {
            {"user-agent", "Mozilla/5.0"},
            {"accept", "application/json"}
        };

        headers.update(updates);

        REQUIRE(headers.size() == 3);
        REQUIRE(headers.get("content-type").value() == "text/html");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(headers.get("accept").value() == "application/json");
    }

    SECTION("update from another HTTPHeaderDict") {
        HTTPHeaderDict headers1;
        headers1.set("content-type", "text/html");
        headers1.set("user-agent", "Mozilla/5.0");

        HTTPHeaderDict headers2;
        headers2.set("Content-Type", "application/json");
        headers2.set("Accept", "text/plain");

        headers1.update(headers2);

        REQUIRE(headers1.size() == 3);
        REQUIRE(headers1.get("content-type").value() == "application/json");
        REQUIRE(headers1.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(headers1.get("accept").value() == "text/plain");
    }
}

TEST_CASE("HTTPHeaderDict operators", "[networking][http]") {
    SECTION("bracket operator") {
        HTTPHeaderDict headers;
        headers.set("content-type", "text/html");

        REQUIRE(headers["content-type"] == "text/html");
        REQUIRE(headers["Content-Type"] == "text/html");
        REQUIRE(headers["missing"] == "");
    }

    SECTION("merge operator |") {
        HTTPHeaderDict headers1;
        headers1.set("content-type", "text/html");
        headers1.set("user-agent", "Mozilla/5.0");

        HTTPHeaderDict headers2;
        headers2.set("Content-Type", "application/json");
        headers2.set("accept", "text/plain");

        auto merged = headers1 | headers2;

        REQUIRE(merged.size() == 3);
        REQUIRE(merged.get("content-type").value() == "application/json");
        REQUIRE(merged.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(merged.get("accept").value() == "text/plain");

        // Original should not be modified
        REQUIRE(headers1.get("content-type").value() == "text/html");
        REQUIRE_FALSE(headers1.contains("accept"));
    }

    SECTION("merge operator | with map") {
        HTTPHeaderDict headers;
        headers.set("content-type", "text/html");

        std::map<std::string, std::string> updates = {
            {"user-agent", "Mozilla/5.0"},
            {"accept", "application/json"}
        };

        auto merged = headers | updates;

        REQUIRE(merged.size() == 3);
        REQUIRE(merged.get("content-type").value() == "text/html");
        REQUIRE(merged.get("user-agent").value() == "Mozilla/5.0");
        REQUIRE(merged.get("accept").value() == "application/json");
    }
}

TEST_CASE("HTTPHeaderDict value trimming", "[networking][http]") {
    HTTPHeaderDict headers;

    SECTION("trims leading and trailing whitespace") {
        headers.set("content-type", "  text/html  ");
        REQUIRE(headers.get("content-type").value() == "text/html");

        headers.set("user-agent", "\t Mozilla/5.0 \n");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0");
    }

    SECTION("preserves internal whitespace") {
        headers.set("user-agent", "Mozilla/5.0 (Windows NT 10.0)");
        REQUIRE(headers.get("user-agent").value() == "Mozilla/5.0 (Windows NT 10.0)");
    }
}

TEST_CASE("HTTPHeaderDict title casing", "[networking][http]") {
    HTTPHeaderDict headers;

    SECTION("correctly title-cases common headers") {
        headers.set("content-type", "text/html");
        headers.set("user-agent", "Mozilla/5.0");
        headers.set("accept-encoding", "gzip, deflate");
        headers.set("x-custom-header", "value");

        auto map = headers.to_map();

        REQUIRE(map.count("Content-Type") == 1);
        REQUIRE(map.count("User-Agent") == 1);
        REQUIRE(map.count("Accept-Encoding") == 1);
        REQUIRE(map.count("X-Custom-Header") == 1);
    }
}

TEST_CASE("HTTPHeaderDict copy and move", "[networking][http]") {
    SECTION("copy constructor") {
        HTTPHeaderDict headers1;
        headers1.set("content-type", "text/html");
        headers1.set("user-agent", "Mozilla/5.0");

        HTTPHeaderDict headers2(headers1);

        REQUIRE(headers2.size() == 2);
        REQUIRE(headers2.get("content-type").value() == "text/html");
        REQUIRE(headers2.get("user-agent").value() == "Mozilla/5.0");

        // Modify copy should not affect original
        headers2.set("accept", "application/json");
        REQUIRE_FALSE(headers1.contains("accept"));
    }

    SECTION("copy assignment") {
        HTTPHeaderDict headers1;
        headers1.set("content-type", "text/html");

        HTTPHeaderDict headers2;
        headers2 = headers1;

        REQUIRE(headers2.get("content-type").value() == "text/html");
    }
}
