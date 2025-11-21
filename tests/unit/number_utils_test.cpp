#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ytdlp/utils/number_utils.hpp"
#include <cmath>

using namespace ytdlp::utils;

TEST_CASE("parse_filesize parses file sizes", "[utils][number]") {
    SECTION("parses plain bytes") {
        auto result = parse_filesize("1024");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1024);
    }

    SECTION("parses kilobytes (decimal)") {
        auto result = parse_filesize("1KB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1000);
    }

    SECTION("parses kilobytes (binary)") {
        auto result = parse_filesize("1KiB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1024);
    }

    SECTION("parses megabytes with decimal") {
        auto result = parse_filesize("1.5MB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1500000);
    }

    SECTION("parses gigabytes") {
        auto result = parse_filesize("2GB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 2000000000);
    }

    SECTION("parses with spaces") {
        auto result = parse_filesize("2.5 MB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 2500000);
    }

    SECTION("parses bytes suffix") {
        auto result = parse_filesize("500 bytes");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 500);
    }

    SECTION("returns nullopt for empty string") {
        auto result = parse_filesize("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for invalid format") {
        auto result = parse_filesize("invalid");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("parses lowercase units") {
        auto result = parse_filesize("500kb");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 500000);
    }

    SECTION("parses terabytes") {
        auto result = parse_filesize("1TB");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1000000000000);
    }
}

TEST_CASE("parse_count parses count strings", "[utils][number]") {
    SECTION("parses plain numbers") {
        auto result = parse_count("1234");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("parses numbers with commas") {
        auto result = parse_count("1,234,567");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234567);
    }

    SECTION("parses K suffix") {
        auto result = parse_count("12.5K");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 12500);
    }

    SECTION("parses M suffix") {
        auto result = parse_count("1.5M");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1500000);
    }

    SECTION("parses B suffix") {
        auto result = parse_count("2B");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 2000000000);
    }

    SECTION("parses KK suffix") {
        auto result = parse_count("1.5KK");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1500000);
    }

    SECTION("removes leading non-digit + space") {
        auto result = parse_count("約 1234");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("extracts leading number with trailing text") {
        auto result = parse_count("1234 views");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("returns nullopt for empty string") {
        auto result = parse_count("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for no digits") {
        auto result = parse_count("no numbers");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("parse_duration parses duration strings", "[utils][number]") {
    SECTION("parses seconds only") {
        auto result = parse_duration("90");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 90.0);
    }

    SECTION("parses MM:SS format") {
        auto result = parse_duration("1:30");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 90.0);
    }

    SECTION("parses HH:MM:SS format") {
        auto result = parse_duration("1:30:45");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5445.0);
    }

    SECTION("parses with milliseconds") {
        auto result = parse_duration("1:30.5");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == Catch::Approx(90.5));
    }

    SECTION("parses text format with hours") {
        auto result = parse_duration("1h30m45s");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5445.0);
    }

    SECTION("parses ISO 8601 format") {
        auto result = parse_duration("PT1H30M45S");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5445.0);
    }

    SECTION("parses days") {
        auto result = parse_duration("2d 3h");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 183600.0);
    }

    SECTION("parses simple hours") {
        auto result = parse_duration("2.5 hours");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 9000.0);
    }

    SECTION("parses simple minutes") {
        auto result = parse_duration("90 minutes");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5400.0);
    }

    SECTION("parses with Z suffix") {
        auto result = parse_duration("1:30Z");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 90.0);
    }

    SECTION("returns nullopt for empty string") {
        auto result = parse_duration("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for invalid format") {
        auto result = parse_duration("invalid");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("parses mixed format") {
        auto result = parse_duration("1h 30min 45sec");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5445.0);
    }
}

TEST_CASE("format_bytes formats byte counts", "[utils][number]") {
    SECTION("formats bytes") {
        auto result = format_bytes(500);
        REQUIRE(result == "500.00B");
    }

    SECTION("formats kilobytes") {
        auto result = format_bytes(1024);
        REQUIRE(result == "1.00KiB");
    }

    SECTION("formats megabytes") {
        auto result = format_bytes(1048576);
        REQUIRE(result == "1.00MiB");
    }

    SECTION("formats gigabytes") {
        auto result = format_bytes(1073741824);
        REQUIRE(result == "1.00GiB");
    }

    SECTION("formats with decimal places") {
        auto result = format_bytes(1536);
        REQUIRE(result == "1.50KiB");
    }

    SECTION("returns N/A for zero") {
        auto result = format_bytes(0);
        REQUIRE(result == "N/A");
    }

    SECTION("returns N/A for negative") {
        auto result = format_bytes(-100);
        REQUIRE(result == "N/A");
    }

    SECTION("formats large sizes") {
        auto result = format_bytes(1099511627776LL);  // 1 TiB
        REQUIRE(result == "1.00TiB");
    }
}

TEST_CASE("parse_resolution parses resolution strings", "[utils][number]") {
    SECTION("parses WIDTHxHEIGHT format") {
        auto result = parse_resolution("1920x1080");
        REQUIRE(result.size() == 2);
        REQUIRE(result["width"] == 1920);
        REQUIRE(result["height"] == 1080);
    }

    SECTION("parses with × symbol") {
        auto result = parse_resolution("1280×720");
        REQUIRE(result.size() == 2);
        REQUIRE(result["width"] == 1280);
        REQUIRE(result["height"] == 720);
    }

    SECTION("parses with spaces") {
        auto result = parse_resolution("1920 x 1080");
        REQUIRE(result.size() == 2);
        REQUIRE(result["width"] == 1920);
        REQUIRE(result["height"] == 1080);
    }

    SECTION("parses NNNp format") {
        auto result = parse_resolution("1080p");
        REQUIRE(result.size() == 1);
        REQUIRE(result["height"] == 1080);
    }

    SECTION("parses uppercase P") {
        auto result = parse_resolution("720P");
        REQUIRE(result.size() == 1);
        REQUIRE(result["height"] == 720);
    }

    SECTION("parses interlaced format") {
        auto result = parse_resolution("1080i");
        REQUIRE(result.size() == 1);
        REQUIRE(result["height"] == 1080);
    }

    SECTION("parses 4k format") {
        auto result = parse_resolution("4k");
        REQUIRE(result.size() == 1);
        REQUIRE(result["height"] == 2160);
    }

    SECTION("parses 8K format") {
        auto result = parse_resolution("8K");
        REQUIRE(result.size() == 1);
        REQUIRE(result["height"] == 4320);
    }

    SECTION("returns empty for invalid format") {
        auto result = parse_resolution("invalid");
        REQUIRE(result.empty());
    }

    SECTION("returns empty for empty string") {
        auto result = parse_resolution("");
        REQUIRE(result.empty());
    }
}

TEST_CASE("format_duration formats durations", "[utils][number]") {
    SECTION("formats seconds only") {
        auto result = format_duration(45);
        REQUIRE(result == "0:45");
    }

    SECTION("formats minutes and seconds") {
        auto result = format_duration(90);
        REQUIRE(result == "1:30");
    }

    SECTION("formats hours minutes and seconds") {
        auto result = format_duration(3665);
        REQUIRE(result == "1:01:05");
    }

    SECTION("pads seconds with zero") {
        auto result = format_duration(65);
        REQUIRE(result == "1:05");
    }

    SECTION("pads minutes with zero when hours present") {
        auto result = format_duration(3605);
        REQUIRE(result == "1:00:05");
    }

    SECTION("handles zero duration") {
        auto result = format_duration(0);
        REQUIRE(result == "0:00");
    }

    SECTION("handles negative duration as zero") {
        auto result = format_duration(-10);
        REQUIRE(result == "0:00");
    }

    SECTION("handles large durations") {
        auto result = format_duration(86400);  // 24 hours
        REQUIRE(result == "24:00:00");
    }
}

TEST_CASE("str_to_int parses integers", "[utils][number]") {
    SECTION("parses simple integer") {
        auto result = str_to_int("1234");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("parses integer with commas") {
        auto result = str_to_int("1,234,567");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234567);
    }

    SECTION("parses negative integer") {
        auto result = str_to_int("-123");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == -123);
    }

    SECTION("handles leading whitespace") {
        auto result = str_to_int("  1234");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("handles trailing whitespace") {
        auto result = str_to_int("1234  ");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1234);
    }

    SECTION("returns nullopt for empty string") {
        auto result = str_to_int("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for non-numeric string") {
        auto result = str_to_int("abc");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("parses large integer") {
        auto result = str_to_int("9223372036854775807");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 9223372036854775807LL);
    }
}
