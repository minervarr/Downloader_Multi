/**
 * Unit Tests for Parse Utilities
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ytdlp/utils/parse_utils.hpp"

using namespace ytdlp::utils;
using Catch::Approx;

// ============================================================================
// Duration Parsing Tests
// ============================================================================

TEST_CASE("parse_duration parses time format", "[utils][parse][duration]") {
    SECTION("HH:MM:SS format") {
        REQUIRE(parse_duration("1:30:45") == Approx(5445.0));
        REQUIRE(parse_duration("0:05:30") == Approx(330.0));
        REQUIRE(parse_duration("2:00:00") == Approx(7200.0));
    }

    SECTION("MM:SS format") {
        REQUIRE(parse_duration("5:30") == Approx(330.0));
        REQUIRE(parse_duration("90:30") == Approx(5430.0));
        REQUIRE(parse_duration("0:45") == Approx(45.0));
    }

    SECTION("SS format") {
        REQUIRE(parse_duration("45") == Approx(45.0));
        REQUIRE(parse_duration("120") == Approx(120.0));
        REQUIRE(parse_duration("3661") == Approx(3661.0));
    }

    SECTION("with milliseconds") {
        REQUIRE(parse_duration("1:30:45.500") == Approx(5445.5));
        REQUIRE(parse_duration("0:00:01.250") == Approx(1.25));
    }
}

TEST_CASE("parse_duration parses ISO 8601 format", "[utils][parse][duration]") {
    SECTION("PT format") {
        REQUIRE(parse_duration("PT1H30M45S") == Approx(5445.0));
        REQUIRE(parse_duration("PT1H30M") == Approx(5400.0));
        REQUIRE(parse_duration("PT45S") == Approx(45.0));
        REQUIRE(parse_duration("PT2H") == Approx(7200.0));
    }

    SECTION("with days") {
        REQUIRE(parse_duration("P1DT2H") == Approx(93600.0));
        REQUIRE(parse_duration("P2D") == Approx(172800.0));
    }
}

TEST_CASE("parse_duration parses descriptive format", "[utils][parse][duration]") {
    SECTION("hours") {
        REQUIRE(parse_duration("1.5 hours") == Approx(5400.0));
        REQUIRE(parse_duration("2 hours") == Approx(7200.0));
    }

    SECTION("minutes") {
        REQUIRE(parse_duration("90 minutes") == Approx(5400.0));
        REQUIRE(parse_duration("5 mins") == Approx(300.0));
    }

    SECTION("seconds") {
        REQUIRE(parse_duration("45 seconds") == Approx(45.0));
        REQUIRE(parse_duration("120 secs") == Approx(120.0));
    }
}

TEST_CASE("parse_duration handles invalid input", "[utils][parse][duration]") {
    REQUIRE_FALSE(parse_duration("").has_value());
    REQUIRE_FALSE(parse_duration("   ").has_value());
    REQUIRE_FALSE(parse_duration("invalid").has_value());
    REQUIRE_FALSE(parse_duration("abc:def:ghi").has_value());
}

// ============================================================================
// File Size Parsing Tests
// ============================================================================

TEST_CASE("parse_filesize parses decimal units", "[utils][parse][filesize]") {
    SECTION("bytes") {
        REQUIRE(parse_filesize("500B") == 500);
        REQUIRE(parse_filesize("1024 bytes") == 1024);
    }

    SECTION("kilobytes") {
        REQUIRE(parse_filesize("1KB") == 1000);
        REQUIRE(parse_filesize("5 KB") == 5000);
        REQUIRE(parse_filesize("1.5KB") == 1500);
    }

    SECTION("megabytes") {
        REQUIRE(parse_filesize("1MB") == 1000000);
        REQUIRE(parse_filesize("500 MB") == 500000000);
        REQUIRE(parse_filesize("1.5MB") == 1500000);
    }

    SECTION("gigabytes") {
        REQUIRE(parse_filesize("1GB") == 1000000000);
        REQUIRE(parse_filesize("2.5 GB") == 2500000000);
    }

    SECTION("terabytes") {
        REQUIRE(parse_filesize("1TB") == 1000000000000);
        REQUIRE(parse_filesize("1.5TB") == 1500000000000);
    }
}

TEST_CASE("parse_filesize parses binary units", "[utils][parse][filesize]") {
    SECTION("kibibytes") {
        REQUIRE(parse_filesize("1KiB") == 1024);
        REQUIRE(parse_filesize("10 KiB") == 10240);
    }

    SECTION("mebibytes") {
        REQUIRE(parse_filesize("1MiB") == 1048576);
        REQUIRE(parse_filesize("100 MiB") == 104857600);
    }

    SECTION("gibibytes") {
        REQUIRE(parse_filesize("1GiB") == 1073741824);
        REQUIRE(parse_filesize("2 GiB") == 2147483648);
    }

    SECTION("tebibytes") {
        REQUIRE(parse_filesize("1TiB") == 1099511627776);
    }
}

TEST_CASE("parse_filesize handles case variations", "[utils][parse][filesize]") {
    REQUIRE(parse_filesize("1mb") == 1000000);
    REQUIRE(parse_filesize("1Mb") == 1000000);
    REQUIRE(parse_filesize("1MB") == 1000000);
    REQUIRE(parse_filesize("1gb") == 1000000000);
}

TEST_CASE("parse_filesize handles invalid input", "[utils][parse][filesize]") {
    REQUIRE_FALSE(parse_filesize("").has_value());
    REQUIRE_FALSE(parse_filesize("   ").has_value());
    REQUIRE_FALSE(parse_filesize("invalid").has_value());
    REQUIRE_FALSE(parse_filesize("123XYZ").has_value());
}

TEST_CASE("format_filesize formats sizes correctly", "[utils][parse][filesize]") {
    SECTION("decimal units") {
        REQUIRE(format_filesize(1000, true) == "1.00KB");
        REQUIRE(format_filesize(1500, true) == "1.50KB");
        REQUIRE(format_filesize(1000000, true) == "1.00MB");
        REQUIRE(format_filesize(1500000000, true) == "1.50GB");
    }

    SECTION("binary units") {
        REQUIRE(format_filesize(1024, false) == "1.00KiB");
        REQUIRE(format_filesize(1048576, false) == "1.00MiB");
        REQUIRE(format_filesize(1073741824, false) == "1.00GiB");
    }
}

// ============================================================================
// Number Parsing Tests
// ============================================================================

TEST_CASE("str_to_int handles thousand separators", "[utils][parse][number]") {
    SECTION("comma separator") {
        REQUIRE(str_to_int("1,234,567") == 1234567);
        REQUIRE(str_to_int("1,000") == 1000);
    }

    SECTION("space separator") {
        REQUIRE(str_to_int("1 234 567") == 1234567);
    }

    SECTION("no separator") {
        REQUIRE(str_to_int("1234567") == 1234567);
        REQUIRE(str_to_int("42") == 42);
    }
}

TEST_CASE("str_to_int handles invalid input", "[utils][parse][number]") {
    REQUIRE_FALSE(str_to_int("").has_value());
    REQUIRE_FALSE(str_to_int("abc").has_value());
    REQUIRE_FALSE(str_to_int("12.34").has_value());
}

TEST_CASE("parse_count handles suffixes", "[utils][parse][count]") {
    SECTION("K suffix") {
        REQUIRE(parse_count("1.2K") == 1200);
        REQUIRE(parse_count("5K") == 5000);
    }

    SECTION("M suffix") {
        REQUIRE(parse_count("1.5M") == 1500000);
        REQUIRE(parse_count("10M") == 10000000);
    }

    SECTION("B suffix") {
        REQUIRE(parse_count("2.3B") == 2300000000);
    }

    SECTION("no suffix") {
        REQUIRE(parse_count("1,234,567") == 1234567);
    }
}

TEST_CASE("int_or_none handles optional values", "[utils][parse][number]") {
    SECTION("valid integer") {
        REQUIRE(int_or_none("42") == 42);
        REQUIRE(int_or_none("100", 2) == 200);
    }

    SECTION("invalid input") {
        REQUIRE_FALSE(int_or_none("").has_value());
        REQUIRE_FALSE(int_or_none("abc").has_value());
    }

    SECTION("with default") {
        REQUIRE(int_or_none("", 1, 999) == 999);
        REQUIRE(int_or_none("abc", 1, 999) == 999);
    }
}

TEST_CASE("float_or_none handles optional values", "[utils][parse][number]") {
    SECTION("valid float") {
        REQUIRE(float_or_none("3.14") == Approx(3.14));
        REQUIRE(float_or_none("2.5", 2.0) == Approx(5.0));
    }

    SECTION("invalid input") {
        REQUIRE_FALSE(float_or_none("").has_value());
        REQUIRE_FALSE(float_or_none("abc").has_value());
    }

    SECTION("with default") {
        REQUIRE(float_or_none("", 1.0, 99.9) == Approx(99.9));
    }
}

// ============================================================================
// Resolution Parsing Tests
// ============================================================================

TEST_CASE("parse_resolution handles WxH format", "[utils][parse][resolution]") {
    auto [width, height] = ytdlp::utils::parse_resolution("1920x1080");
    REQUIRE(width == 1920);
    REQUIRE(height == 1080);

    auto [w2, h2] = ytdlp::utils::parse_resolution("1280x720");
    REQUIRE(w2 == 1280);
    REQUIRE(h2 == 720);
}

TEST_CASE("parse_resolution handles Hp format", "[utils][parse][resolution]") {
    auto [width, height] = ytdlp::utils::parse_resolution("1080p");
    REQUIRE_FALSE(width.has_value());
    REQUIRE(height == 1080);

    auto [w2, h2] = ytdlp::utils::parse_resolution("720p60");
    REQUIRE_FALSE(w2.has_value());
    REQUIRE(h2 == 720);
}

TEST_CASE("parse_resolution handles invalid input", "[utils][parse][resolution]") {
    auto [width, height] = ytdlp::utils::parse_resolution("");
    REQUIRE_FALSE(width.has_value());
    REQUIRE_FALSE(height.has_value());

    auto [w2, h2] = ytdlp::utils::parse_resolution("invalid");
    REQUIRE_FALSE(w2.has_value());
    REQUIRE_FALSE(h2.has_value());
}

// ============================================================================
// Bitrate Parsing Tests
// ============================================================================

TEST_CASE("parse_bitrate handles various formats", "[utils][parse][bitrate]") {
    SECTION("with K suffix") {
        REQUIRE(parse_bitrate("128k") == 128000);
        REQUIRE(parse_bitrate("320K") == 320000);
    }

    SECTION("with M suffix") {
        REQUIRE(parse_bitrate("1.5M") == 1500000);
        REQUIRE(parse_bitrate("2M") == 2000000);
    }

    SECTION("with bps suffix") {
        REQUIRE(parse_bitrate("128kbps") == 128000);
        REQUIRE(parse_bitrate("1.5Mbps") == 1500000);
    }

    SECTION("plain number") {
        REQUIRE(parse_bitrate("128000") == 128000);
    }
}

TEST_CASE("parse_bitrate handles invalid input", "[utils][parse][bitrate]") {
    REQUIRE_FALSE(parse_bitrate("").has_value());
    REQUIRE_FALSE(parse_bitrate("invalid").has_value());
}

// ============================================================================
// Age Limit Parsing Tests
// ============================================================================

TEST_CASE("parse_age_limit handles various formats", "[utils][parse][age]") {
    SECTION("simple number") {
        REQUIRE(parse_age_limit("18") == 18);
        REQUIRE(parse_age_limit("16") == 16);
    }

    SECTION("with + suffix") {
        REQUIRE(parse_age_limit("18+") == 18);
        REQUIRE(parse_age_limit("21+") == 21);
    }

    SECTION("PG format") {
        REQUIRE(parse_age_limit("PG-13") == 13);
        REQUIRE(parse_age_limit("PG13") == 13);
    }
}

TEST_CASE("parse_age_limit handles invalid input", "[utils][parse][age]") {
    REQUIRE_FALSE(parse_age_limit("").has_value());
    REQUIRE_FALSE(parse_age_limit("invalid").has_value());
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST_CASE("is_number validates numbers", "[utils][parse][helper]") {
    REQUIRE(is_number("123"));
    REQUIRE(is_number("3.14"));
    REQUIRE(is_number("-42"));
    REQUIRE(is_number("+3.14"));
    REQUIRE(is_number("1.23e10"));

    REQUIRE_FALSE(is_number(""));
    REQUIRE_FALSE(is_number("abc"));
    REQUIRE_FALSE(is_number("12abc"));
}

TEST_CASE("is_integer validates integers", "[utils][parse][helper]") {
    REQUIRE(is_integer("123"));
    REQUIRE(is_integer("-42"));
    REQUIRE(is_integer("+100"));

    REQUIRE_FALSE(is_integer(""));
    REQUIRE_FALSE(is_integer("3.14"));
    REQUIRE_FALSE(is_integer("abc"));
}
