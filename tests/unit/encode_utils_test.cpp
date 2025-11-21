#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/encode_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"

using namespace ytdlp::utils;

TEST_CASE("base64_encode encodes binary data", "[utils][encode]") {
    SECTION("encodes simple string") {
        auto result = base64_encode("Hello, World!");
        REQUIRE(result == "SGVsbG8sIFdvcmxkIQ==");
    }

    SECTION("encodes empty string") {
        auto result = base64_encode("");
        REQUIRE(result == "");
    }

    SECTION("encodes binary data") {
        std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0x03};
        auto result = base64_encode(data);
        REQUIRE(result == "AAECAw==");
    }

    SECTION("encodes text without padding") {
        auto result = base64_encode("abc");
        REQUIRE(result == "YWJj");
    }

    SECTION("encodes text with padding") {
        auto result = base64_encode("ab");
        REQUIRE(result == "YWI=");
    }
}

TEST_CASE("base64_decode decodes base64 strings", "[utils][encode]") {
    SECTION("decodes simple string") {
        auto result = base64_decode("SGVsbG8sIFdvcmxkIQ==");
        REQUIRE(result.has_value());
        std::string decoded(result->begin(), result->end());
        REQUIRE(decoded == "Hello, World!");
    }

    SECTION("decodes without padding") {
        auto result = base64_decode("YWJj");
        REQUIRE(result.has_value());
        std::string decoded(result->begin(), result->end());
        REQUIRE(decoded == "abc");
    }

    SECTION("decodes with padding") {
        auto result = base64_decode("YWI=");
        REQUIRE(result.has_value());
        std::string decoded(result->begin(), result->end());
        REQUIRE(decoded == "ab");
    }

    SECTION("handles URL-safe base64") {
        auto result = base64_decode("SGVsbG8tV29ybGQh"); // URL-safe variant
        REQUIRE(result.has_value());
    }

    SECTION("returns nullopt for invalid base64") {
        auto result = base64_decode("Invalid!@#$%");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("encode_base_n encodes integers to base-N", "[utils][encode]") {
    SECTION("encodes to base62") {
        auto result = encode_base_n(12345, 62);
        REQUIRE(result == "3d7");
    }

    SECTION("encodes zero") {
        auto result = encode_base_n(0, 62);
        REQUIRE(result == "0");
    }

    SECTION("encodes to base16 (hex)") {
        auto result = encode_base_n(255, 16);
        REQUIRE(result == "FF");
    }

    SECTION("encodes to base10") {
        auto result = encode_base_n(12345, 10);
        REQUIRE(result == "12345");  // Base 10 uses digits 0-9
    }

    SECTION("encodes large number") {
        auto result = encode_base_n(1000000, 62);
        REQUIRE_FALSE(result.empty());
    }
}

TEST_CASE("decode_base_n decodes base-N strings", "[utils][encode]") {
    SECTION("decodes base62 string") {
        auto result = decode_base_n("3d7", 62);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 12345);
    }

    SECTION("decodes zero") {
        auto result = decode_base_n("0", 62);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 0);
    }

    SECTION("decodes base16 (hex)") {
        auto result = decode_base_n("FF", 16);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 255);
    }

    SECTION("returns nullopt for invalid character") {
        auto result = decode_base_n("xyz@", 62);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for empty string") {
        auto result = decode_base_n("", 62);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("encode_data_uri creates data URIs", "[utils][encode]") {
    SECTION("creates simple data URI") {
        std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
        auto result = encode_data_uri(data, "text/plain");
        REQUIRE(result == "data:text/plain;base64,SGVsbG8=");
    }

    SECTION("creates image data URI") {
        std::vector<uint8_t> data = {0x89, 0x50, 0x4E, 0x47};  // PNG header
        auto result = encode_data_uri(data, "image/png");
        REQUIRE(result.find("data:image/png;base64,") == 0);
    }
}

TEST_CASE("jwt_encode creates JSON Web Tokens", "[utils][encode]") {
    SECTION("creates basic JWT") {
        nlohmann::json payload = {{"user_id", 123}, {"name", "John"}};
        auto jwt = jwt_encode(payload, "secret");

        // JWT should have 3 parts separated by dots
        int dot_count = std::count(jwt.begin(), jwt.end(), '.');
        REQUIRE(dot_count == 2);

        // Check it starts with expected header (eyJ = base64url of '{"')
        REQUIRE(jwt.substr(0, 3) == "eyJ");
    }

    SECTION("creates JWT with custom headers") {
        nlohmann::json payload = {{"data", "test"}};
        nlohmann::json headers = {{"kid", "key123"}};
        auto jwt = jwt_encode(payload, "secret", headers);

        REQUIRE_FALSE(jwt.empty());
        int dot_count = std::count(jwt.begin(), jwt.end(), '.');
        REQUIRE(dot_count == 2);
    }
}

TEST_CASE("jwt_decode_hs256 decodes JWTs", "[utils][encode]") {
    SECTION("decodes valid JWT") {
        // Create a JWT first
        nlohmann::json payload = {{"user_id", 123}, {"name", "John"}};
        auto jwt = jwt_encode(payload, "secret");

        // Decode it
        auto decoded = jwt_decode_hs256(jwt);
        REQUIRE(decoded.has_value());
        REQUIRE(decoded.value()["user_id"] == 123);
        REQUIRE(decoded.value()["name"] == "John");
    }

    SECTION("returns nullopt for invalid JWT format") {
        auto decoded = jwt_decode_hs256("not.a.valid.jwt");
        REQUIRE_FALSE(decoded.has_value());
    }

    SECTION("returns nullopt for malformed base64") {
        auto decoded = jwt_decode_hs256("invalid.base64!.data");
        REQUIRE_FALSE(decoded.has_value());
    }
}

TEST_CASE("multipart_encode creates multipart form data", "[utils][encode]") {
    SECTION("encodes simple form data") {
        std::map<std::string, std::string> data = {
            {"field1", "value1"},
            {"field2", "value2"}
        };
        auto [body, content_type] = multipart_encode(data);

        REQUIRE(content_type.find("multipart/form-data; boundary=") == 0);
        REQUIRE(body.find("Content-Disposition: form-data; name=\"field1\"") != std::string::npos);
        REQUIRE(body.find("value1") != std::string::npos);
        REQUIRE(body.find("field2") != std::string::npos);
    }

    SECTION("uses custom boundary") {
        std::map<std::string, std::string> data = {{"key", "value"}};
        auto [body, content_type] = multipart_encode(data, "myboundary");

        REQUIRE(content_type == "multipart/form-data; boundary=myboundary");
        REQUIRE(body.find("--myboundary") != std::string::npos);
    }

    SECTION("handles empty data") {
        std::map<std::string, std::string> data;
        auto [body, content_type] = multipart_encode(data);

        REQUIRE_FALSE(content_type.empty());
        REQUIRE_FALSE(body.empty());
    }
}

TEST_CASE("urlencode_postdata creates URL-encoded form data", "[utils][encode]") {
    SECTION("encodes simple key-value pairs") {
        std::map<std::string, std::string> data = {
            {"key1", "value1"},
            {"key2", "value2"}
        };
        auto result = urlencode_postdata(data);

        // Result should contain both key=value pairs with & separator
        REQUIRE(result.find("key1=value1") != std::string::npos);
        REQUIRE(result.find("key2=value2") != std::string::npos);
        REQUIRE(result.find("&") != std::string::npos);
    }

    SECTION("URL-encodes special characters") {
        std::map<std::string, std::string> data = {
            {"name", "John Doe"},
            {"email", "test@example.com"}
        };
        auto result = urlencode_postdata(data);

        // Space should be encoded as %20
        REQUIRE(result.find("%20") != std::string::npos);
    }

    SECTION("handles empty data") {
        std::map<std::string, std::string> data;
        auto result = urlencode_postdata(data);

        REQUIRE(result.empty());
    }
}

TEST_CASE("caesar shifts characters in alphabet", "[utils][encode]") {
    SECTION("shifts alphabetic characters") {
        auto result = caesar("ABC", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 3);
        REQUIRE(result == "DEF");
    }

    SECTION("wraps around alphabet") {
        auto result = caesar("XYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 3);
        REQUIRE(result == "ABC");
    }

    SECTION("handles negative shift") {
        auto result = caesar("DEF", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", -3);
        REQUIRE(result == "ABC");
    }

    SECTION("preserves characters not in alphabet") {
        auto result = caesar("A1B2C3", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 3);
        REQUIRE(result == "D1E2F3");
    }

    SECTION("handles zero shift") {
        auto result = caesar("ABC", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0);
        REQUIRE(result == "ABC");
    }
}

TEST_CASE("rot47 performs ROT47 cipher", "[utils][encode]") {
    SECTION("encodes printable ASCII") {
        auto result = rot47("Hello");
        // ROT47 shifts all printable ASCII by 47
        REQUIRE_FALSE(result.empty());
        REQUIRE(result.length() == 5);
    }

    SECTION("is reversible") {
        std::string original = "Test123!";
        auto encoded = rot47(original);
        auto decoded = rot47(encoded);
        REQUIRE(decoded == original);
    }

    SECTION("handles empty string") {
        auto result = rot47("");
        REQUIRE(result.empty());
    }
}

TEST_CASE("urshift performs unsigned right shift", "[utils][encode]") {
    SECTION("shifts positive number") {
        auto result = urshift(8, 2);
        REQUIRE(result == 2);
    }

    SECTION("shifts negative number (zero-fill)") {
        auto result = urshift(-1, 1);
        // -1 as uint32_t is 0xFFFFFFFF, shifted right by 1 is 0x7FFFFFFF
        REQUIRE(result == 0x7FFFFFFF);
    }

    SECTION("shifts by zero") {
        auto result = urshift(100, 0);
        REQUIRE(result == 100);
    }

    SECTION("shifts to zero") {
        auto result = urshift(1, 32);
        REQUIRE(result == 0);
    }
}

TEST_CASE("detect_bom detects byte order marks", "[utils][encode]") {
    SECTION("detects UTF-8 BOM") {
        std::vector<uint8_t> data = {0xEF, 0xBB, 0xBF, 'H', 'e', 'l', 'l', 'o'};
        auto result = detect_bom(data);
        REQUIRE(result.has_value());
        REQUIRE(result->first == 3);
        REQUIRE(result->second == "utf-8");
    }

    SECTION("detects UTF-16 LE BOM") {
        std::vector<uint8_t> data = {0xFF, 0xFE, 'H', 'e'};
        auto result = detect_bom(data);
        REQUIRE(result.has_value());
        REQUIRE(result->first == 2);
        REQUIRE(result->second == "utf-16-le");
    }

    SECTION("detects UTF-16 BE BOM") {
        std::vector<uint8_t> data = {0xFE, 0xFF, 'H', 'e'};
        auto result = detect_bom(data);
        REQUIRE(result.has_value());
        REQUIRE(result->first == 2);
        REQUIRE(result->second == "utf-16-be");
    }

    SECTION("returns nullopt when no BOM") {
        std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
        auto result = detect_bom(data);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("remove_bom removes byte order marks", "[utils][encode]") {
    SECTION("removes UTF-8 BOM") {
        std::string data = "\xEF\xBB\xBFHello";
        auto result = remove_bom(data);
        REQUIRE(result == "Hello");
    }

    SECTION("removes UTF-16 LE BOM") {
        std::string data = "\xFF\xFEHello";
        auto result = remove_bom(data);
        REQUIRE(result == "Hello");
    }

    SECTION("returns unchanged string without BOM") {
        std::string data = "Hello";
        auto result = remove_bom(data);
        REQUIRE(result == "Hello");
    }

    SECTION("handles empty string") {
        auto result = remove_bom("");
        REQUIRE(result.empty());
    }
}

TEST_CASE("hmac_sha256 computes HMAC-SHA256", "[utils][encode]") {
    SECTION("computes valid HMAC") {
        auto result = hmac_sha256("key", "message");
        REQUIRE(result.size() == 32);  // SHA256 produces 32 bytes
    }

    SECTION("produces different hashes for different messages") {
        auto hash1 = hmac_sha256("key", "message1");
        auto hash2 = hmac_sha256("key", "message2");
        REQUIRE(hash1 != hash2);
    }

    SECTION("produces different hashes for different keys") {
        auto hash1 = hmac_sha256("key1", "message");
        auto hash2 = hmac_sha256("key2", "message");
        REQUIRE(hash1 != hash2);
    }

    SECTION("same key and message produce same hash") {
        auto hash1 = hmac_sha256("key", "message");
        auto hash2 = hmac_sha256("key", "message");
        REQUIRE(hash1 == hash2);
    }
}

TEST_CASE("base_n_table returns correct character tables", "[utils][encode]") {
    SECTION("returns base62 table") {
        auto table = base_n_table(62);
        REQUIRE(table.length() == 62);
        REQUIRE(table[0] == '0');
        REQUIRE(table[61] == 'Z');
    }

    SECTION("returns base16 table") {
        auto table = base_n_table(16);
        REQUIRE(table.length() == 16);
        REQUIRE(table == "0123456789ABCDEF");
    }

    SECTION("returns custom table") {
        auto table = base_n_table(5, "ABCDE");
        REQUIRE(table == "ABCDE");
    }
}
