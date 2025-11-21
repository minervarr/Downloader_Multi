/**
 * Unit Tests for JSON Utilities
 */

#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/json_utils.hpp"
#include "ytdlp/utils/filesystem_utils.hpp"

using namespace ytdlp::utils;

// ============================================================================
// JSON Parsing Tests
// ============================================================================

TEST_CASE("parse_json parses valid JSON", "[utils][json]") {
    SECTION("parses simple object") {
        auto j = parse_json(R"({"name": "test", "value": 123})");
        REQUIRE(j.has_value());
        REQUIRE(j->is_object());
        REQUIRE((*j)["name"] == "test");
        REQUIRE((*j)["value"] == 123);
    }

    SECTION("parses array") {
        auto j = parse_json(R"([1, 2, 3, "four"])");
        REQUIRE(j.has_value());
        REQUIRE(j->is_array());
        REQUIRE(j->size() == 4);
    }

    SECTION("returns nullopt for invalid JSON") {
        auto j = parse_json("{invalid json}");
        REQUIRE_FALSE(j.has_value());
    }
}

// ============================================================================
// Safe Access Tests
// ============================================================================

TEST_CASE("get_string returns string or default", "[utils][json]") {
    json j = {
        {"name", "test"},
        {"number", 123},
        {"flag", true}
    };

    SECTION("returns existing string") {
        REQUIRE(get_string(j, "name") == "test");
    }

    SECTION("returns default for missing key") {
        REQUIRE(get_string(j, "missing", "default") == "default");
    }

    SECTION("returns default for wrong type") {
        REQUIRE(get_string(j, "number", "default") == "default");
    }
}

TEST_CASE("get_int returns integer or default", "[utils][json]") {
    json j = {
        {"number", 123},
        {"string", "not a number"},
        {"float", 45.67}
    };

    SECTION("returns existing integer") {
        REQUIRE(get_int(j, "number") == 123);
    }

    SECTION("returns default for missing key") {
        REQUIRE(get_int(j, "missing", 999) == 999);
    }

    SECTION("returns integer from float") {
        REQUIRE(get_int(j, "float") == 45);
    }
}

TEST_CASE("get_bool returns boolean or default", "[utils][json]") {
    json j = {
        {"flag", true},
        {"number", 1}
    };

    REQUIRE(get_bool(j, "flag") == true);
    REQUIRE(get_bool(j, "missing", false) == false);
    REQUIRE(get_bool(j, "number", false) == false); // Wrong type
}

TEST_CASE("get_array returns array or empty", "[utils][json]") {
    json j = {
        {"items", json::array({1, 2, 3})},
        {"notarray", "string"}
    };

    SECTION("returns existing array") {
        auto arr = get_array(j, "items");
        REQUIRE(arr.is_array());
        REQUIRE(arr.size() == 3);
    }

    SECTION("returns empty array for missing key") {
        auto arr = get_array(j, "missing");
        REQUIRE(arr.is_array());
        REQUIRE(arr.empty());
    }

    SECTION("returns empty array for wrong type") {
        auto arr = get_array(j, "notarray");
        REQUIRE(arr.is_array());
        REQUIRE(arr.empty());
    }
}

// ============================================================================
// Optional Access Tests
// ============================================================================

TEST_CASE("optional getters return optional values", "[utils][json]") {
    json j = {
        {"name", "test"},
        {"number", 42}
    };

    SECTION("get_string_opt") {
        REQUIRE(get_string_opt(j, "name") == "test");
        REQUIRE_FALSE(get_string_opt(j, "missing").has_value());
        REQUIRE_FALSE(get_string_opt(j, "number").has_value());
    }

    SECTION("get_int_opt") {
        REQUIRE(get_int_opt(j, "number") == 42);
        REQUIRE_FALSE(get_int_opt(j, "missing").has_value());
        REQUIRE_FALSE(get_int_opt(j, "name").has_value());
    }
}

// ============================================================================
// Type Checking Tests
// ============================================================================

TEST_CASE("type checking functions work correctly", "[utils][json]") {
    json j = {
        {"string", "value"},
        {"number", 123},
        {"bool", true},
        {"array", json::array({1, 2, 3})},
        {"object", json::object({{"key", "value"}})},
        {"null", nullptr}
    };

    REQUIRE(is_string(j, "string"));
    REQUIRE_FALSE(is_string(j, "number"));

    REQUIRE(is_number(j, "number"));
    REQUIRE_FALSE(is_number(j, "string"));

    REQUIRE(is_bool(j, "bool"));
    REQUIRE_FALSE(is_bool(j, "number"));

    REQUIRE(is_array(j, "array"));
    REQUIRE_FALSE(is_array(j, "string"));

    REQUIRE(is_object(j, "object"));
    REQUIRE_FALSE(is_object(j, "string"));

    REQUIRE(is_null(j, "null"));
    REQUIRE_FALSE(is_null(j, "string"));
}

// ============================================================================
// JSON Path Navigation Tests
// ============================================================================

TEST_CASE("JSON path navigation works", "[utils][json][path]") {
    json j = {
        {"user", {
            {"name", "Alice"},
            {"age", 30},
            {"address", {
                {"city", "NYC"}
            }}
        }},
        {"items", json::array({1, 2, 3})}
    };

    SECTION("get_at_path retrieves nested values") {
        REQUIRE(get_at_path(j, "/user/name") == "Alice");
        REQUIRE(get_at_path(j, "/user/age") == 30);
        REQUIRE(get_at_path(j, "/user/address/city") == "NYC");
        REQUIRE(get_at_path(j, "/items/0") == 1);
    }

    SECTION("has_path checks path existence") {
        REQUIRE(has_path(j, "/user/name"));
        REQUIRE(has_path(j, "/user/address/city"));
        REQUIRE_FALSE(has_path(j, "/user/nonexistent"));
    }

    SECTION("set_at_path sets nested values") {
        json j_copy = j;
        REQUIRE(set_at_path(j_copy, "/user/name", "Bob"));
        REQUIRE(get_at_path(j_copy, "/user/name") == "Bob");
    }
}

// ============================================================================
// Array Helpers Tests
// ============================================================================

TEST_CASE("to_string_vector converts JSON array to string vector", "[utils][json]") {
    json j = json::array({"one", "two", "three", 123, true});

    auto vec = to_string_vector(j);
    REQUIRE(vec.size() == 3);  // Only strings
    REQUIRE(vec[0] == "one");
    REQUIRE(vec[1] == "two");
    REQUIRE(vec[2] == "three");
}

TEST_CASE("to_int_vector converts JSON array to int vector", "[utils][json]") {
    json j = json::array({1, 2, 3, "four", 5});

    auto vec = to_int_vector(j);
    REQUIRE(vec.size() == 4);  // Only numbers
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 5);
}

TEST_CASE("get_array_size returns array size", "[utils][json]") {
    json j = {
        {"items", json::array({1, 2, 3, 4, 5})},
        {"notarray", "string"}
    };

    REQUIRE(get_array_size(j, "items") == 5);
    REQUIRE(get_array_size(j, "notarray") == 0);
    REQUIRE(get_array_size(j, "missing") == 0);
}

// ============================================================================
// JSON Merging Tests
// ============================================================================

TEST_CASE("merge combines JSON objects", "[utils][json]") {
    json base = {
        {"a", 1},
        {"b", {
            {"c", 2},
            {"d", 3}
        }}
    };

    json overlay = {
        {"b", {
            {"d", 4},
            {"e", 5}
        }},
        {"f", 6}
    };

    json merged = merge(base, overlay);

    REQUIRE(merged["a"] == 1);
    REQUIRE(merged["b"]["c"] == 2);  // From base
    REQUIRE(merged["b"]["d"] == 4);  // Overridden
    REQUIRE(merged["b"]["e"] == 5);  // New
    REQUIRE(merged["f"] == 6);       // New
}

TEST_CASE("update performs shallow merge", "[utils][json]") {
    json target = {
        {"a", 1},
        {"b", 2}
    };

    json source = {
        {"b", 20},
        {"c", 30}
    };

    update(target, source);

    REQUIRE(target["a"] == 1);
    REQUIRE(target["b"] == 20);
    REQUIRE(target["c"] == 30);
}

// ============================================================================
// JSON Formatting Tests
// ============================================================================

TEST_CASE("JSON formatting functions", "[utils][json][format]") {
    json j = {
        {"name", "test"},
        {"value", 123}
    };

    SECTION("to_string with indentation") {
        std::string formatted = to_string(j, 2);
        REQUIRE(formatted.find('\n') != std::string::npos);  // Has newlines
    }

    SECTION("to_compact_string") {
        std::string compact = to_compact_string(j);
        REQUIRE(compact.find('\n') == std::string::npos);  // No newlines
    }
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_CASE("has_required_keys validates keys", "[utils][json][validation]") {
    json j = {
        {"name", "test"},
        {"age", 30},
        {"email", "test@example.com"}
    };

    SECTION("returns true when all keys present") {
        REQUIRE(has_required_keys(j, {"name", "age"}));
    }

    SECTION("returns false when key missing") {
        REQUIRE_FALSE(has_required_keys(j, {"name", "missing"}));
    }
}

TEST_CASE("get_missing_keys returns missing keys", "[utils][json][validation]") {
    json j = {
        {"name", "test"},
        {"age", 30}
    };

    auto missing = get_missing_keys(j, {"name", "age", "email", "phone"});
    REQUIRE(missing.size() == 2);
    REQUIRE(std::find(missing.begin(), missing.end(), "email") != missing.end());
    REQUIRE(std::find(missing.begin(), missing.end(), "phone") != missing.end());
}

TEST_CASE("validate_structure checks types", "[utils][json][validation]") {
    json j = {
        {"name", "test"},
        {"count", 123},
        {"enabled", true}
    };

    std::map<std::string, json::value_t> schema = {
        {"name", json::value_t::string},
        {"count", json::value_t::number_integer},
        {"enabled", json::value_t::boolean}
    };

    REQUIRE(validate_structure(j, schema));

    // Change a type
    j["count"] = "not a number";
    REQUIRE_FALSE(validate_structure(j, schema));
}

// ============================================================================
// File I/O Tests
// ============================================================================

TEST_CASE("JSON file I/O works", "[utils][json][io]") {
    std::string test_dir = get_temp_directory() + "/ytdlp_test";
    std::string test_file = test_dir + "/test.json";

    // Clean up
    if (path_exists(test_dir)) {
        remove_directory(test_dir);
    }

    make_directory(test_dir);

    SECTION("write and read JSON file") {
        json original = {
            {"name", "test"},
            {"values", json::array({1, 2, 3})},
            {"nested", {
                {"key", "value"}
            }}
        };

        REQUIRE(write_json_file(original, test_file));
        REQUIRE(path_exists(test_file));

        auto loaded = read_json_file(test_file);
        REQUIRE(loaded.has_value());
        REQUIRE(*loaded == original);
    }

    SECTION("read_json_file returns nullopt for non-existent file") {
        auto result = read_json_file("/nonexistent/file.json");
        REQUIRE_FALSE(result.has_value());
    }

    // Cleanup
    remove_directory(test_dir);
}
