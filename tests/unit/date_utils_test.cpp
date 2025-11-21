#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/date_utils.hpp"

using namespace ytdlp::utils;

TEST_CASE("extract_timezone extracts timezone offsets", "[utils][date]") {
    SECTION("extracts positive timezone offset") {
        auto result = extract_timezone("2023-01-15 10:30:00+0100");
        REQUIRE(result.offset_seconds.has_value());
        REQUIRE(result.offset_seconds.value() == 3600);  // +01:00 = 3600 seconds
        REQUIRE(result.date_str == "2023-01-15 10:30:00");
    }

    SECTION("extracts negative timezone offset") {
        auto result = extract_timezone("2023-01-15 10:30:00-0500");
        REQUIRE(result.offset_seconds.has_value());
        REQUIRE(result.offset_seconds.value() == -18000);  // -05:00 = -18000 seconds
        REQUIRE(result.date_str == "2023-01-15 10:30:00");
    }

    SECTION("extracts timezone with colon separator") {
        auto result = extract_timezone("2023-01-15 10:30:00+05:30");
        REQUIRE(result.offset_seconds.has_value());
        REQUIRE(result.offset_seconds.value() == 19800);  // +05:30 = 19800 seconds
        REQUIRE(result.date_str == "2023-01-15 10:30:00");
    }

    SECTION("recognizes Z as UTC") {
        auto result = extract_timezone("2023-01-15T10:30:00Z");
        REQUIRE(result.offset_seconds.has_value());
        REQUIRE(result.offset_seconds.value() == 0);
        REQUIRE(result.date_str == "2023-01-15T10:30:00");
    }

    SECTION("uses default offset when no timezone found") {
        auto result = extract_timezone("2023-01-15 10:30:00", 7200);
        REQUIRE(result.offset_seconds.has_value());
        REQUIRE(result.offset_seconds.value() == 7200);
        REQUIRE(result.date_str == "2023-01-15 10:30:00");
    }
}

TEST_CASE("parse_iso8601 parses ISO 8601 dates", "[utils][date]") {
    SECTION("parses basic ISO 8601 date with T delimiter") {
        auto result = parse_iso8601("2023-01-15T10:30:45");
        REQUIRE(result.has_value());
        // 2023-01-15 10:30:45 UTC = 1673778645
        REQUIRE(result.value() == 1673778645);
    }

    SECTION("parses ISO 8601 with space delimiter") {
        auto result = parse_iso8601("2023-01-15 10:30:45", ' ');
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673778645);
    }

    SECTION("parses ISO 8601 with timezone") {
        auto result = parse_iso8601("2023-01-15T10:30:45+0100");
        REQUIRE(result.has_value());
        // Adjusted for +01:00 timezone
        REQUIRE(result.value() == 1673778645 - 3600);
    }

    SECTION("parses ISO 8601 with Z timezone") {
        auto result = parse_iso8601("2023-01-15T10:30:45Z");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673778645);
    }

    SECTION("handles fractional seconds") {
        auto result = parse_iso8601("2023-01-15T10:30:45.123");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673778645);  // Fractional part ignored
    }

    SECTION("returns nullopt for invalid dates") {
        auto result = parse_iso8601("invalid");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for empty string") {
        auto result = parse_iso8601("");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("unified_strdate normalizes dates to YYYYMMDD", "[utils][date]") {
    SECTION("handles YYYY-MM-DD format") {
        auto result = unified_strdate("2023-01-15");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("handles DD/MM/YYYY with day_first=true") {
        auto result = unified_strdate("15/01/2023", true);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("handles MM/DD/YYYY with day_first=false") {
        auto result = unified_strdate("01/15/2023", false);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("handles dates with commas") {
        auto result = unified_strdate("2023-01-15, 10:30:00");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("removes AM/PM") {
        auto result = unified_strdate("2023-01-15 10:30 AM");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("returns nullopt for empty string") {
        auto result = unified_strdate("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for invalid date") {
        auto result = unified_strdate("not a date");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("unified_timestamp parses various date formats", "[utils][date]") {
    SECTION("parses YYYY-MM-DD format") {
        auto result = unified_timestamp("2023-01-15");
        REQUIRE(result.has_value());
        // 2023-01-15 00:00:00 UTC = 1673740800
        REQUIRE(result.value() == 1673740800);
    }

    SECTION("parses YYYY-MM-DD with time") {
        auto result = unified_timestamp("2023-01-15 10:30:00");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673778600);
    }

    SECTION("parses DD/MM/YYYY with day_first=true") {
        auto result = unified_timestamp("15/01/2023", true);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673740800);
    }

    SECTION("parses MM/DD/YYYY with day_first=false") {
        auto result = unified_timestamp("01/15/2023", false);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673740800);
    }

    SECTION("returns nullopt for empty string") {
        auto result = unified_timestamp("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for invalid date") {
        auto result = unified_timestamp("invalid date");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("datetime_round rounds timestamps to precision", "[utils][date]") {
    // Timestamp: 2023-01-15 10:35:47 UTC = 1673778947

    SECTION("rounds to day precision") {
        int64_t timestamp = 1673778947;  // 2023-01-15 10:35:47
        auto result = datetime_round(timestamp, DatePrecision::Day);
        // Should round to 2023-01-15 00:00:00 = 1673740800
        REQUIRE(result == 1673740800);
    }

    SECTION("rounds to hour precision") {
        int64_t timestamp = 1673778947;  // 2023-01-15 10:35:47
        auto result = datetime_round(timestamp, DatePrecision::Hour);
        // Should round to 2023-01-15 11:00:00 = 1673780400
        REQUIRE(result == 1673780400);
    }

    SECTION("rounds to minute precision") {
        int64_t timestamp = 1673778947;  // 2023-01-15 10:35:47
        auto result = datetime_round(timestamp, DatePrecision::Minute);
        // Should round to 2023-01-15 10:36:00 = 1673778960
        REQUIRE(result == 1673778960);
    }

    SECTION("rounds to second precision") {
        int64_t timestamp = 1673778947;
        auto result = datetime_round(timestamp, DatePrecision::Second);
        REQUIRE(result == 1673778947);  // No change at second precision
    }

    SECTION("microsecond precision returns unchanged") {
        int64_t timestamp = 1673778947;
        auto result = datetime_round(timestamp, DatePrecision::Microsecond);
        REQUIRE(result == 1673778947);
    }
}

TEST_CASE("strftime_or_none formats timestamps", "[utils][date]") {
    SECTION("formats UNIX timestamp with default format") {
        int64_t timestamp = 1673740800;  // 2023-01-15 00:00:00 UTC
        auto result = strftime_or_none(timestamp);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("formats UNIX timestamp with custom format") {
        int64_t timestamp = 1673740800;  // 2023-01-15 00:00:00 UTC
        auto result = strftime_or_none(timestamp, "%Y-%m-%d");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2023-01-15");
    }

    SECTION("formats YYYYMMDD string with default format") {
        std::string date_str = "20230115";
        auto result = strftime_or_none(date_str);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "20230115");
    }

    SECTION("formats YYYYMMDD string with custom format") {
        std::string date_str = "20230115";
        auto result = strftime_or_none(date_str, "%Y-%m-%d");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2023-01-15");
    }

    SECTION("returns nullopt for invalid YYYYMMDD string") {
        std::string invalid = "invalid";
        auto result = strftime_or_none(invalid);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("timeconvert parses RFC 2822 dates", "[utils][date]") {
    SECTION("parses RFC 2822 format") {
        auto result = timeconvert("Sun, 15 Jan 2023 10:30:00 +0000");
        REQUIRE(result.has_value());
        // 2023-01-15 10:30:00 UTC = 1673778600
        REQUIRE(result.value() == 1673778600);
    }

    SECTION("returns nullopt for empty string") {
        auto result = timeconvert("");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for invalid format") {
        auto result = timeconvert("not a date");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("datetime_from_str handles relative dates", "[utils][date]") {
    SECTION("handles 'now' keyword") {
        auto result = datetime_from_str("now");
        REQUIRE(result.has_value());
        // Should return current time (can't check exact value)
        REQUIRE(result.value() > 0);
    }

    SECTION("handles 'today' keyword") {
        auto result = datetime_from_str("today");
        REQUIRE(result.has_value());
        REQUIRE(result.value() > 0);
    }

    SECTION("handles 'yesterday' keyword") {
        auto result = datetime_from_str("yesterday");
        REQUIRE(result.has_value());
        REQUIRE(result.value() > 0);
    }

    SECTION("handles relative offset with days") {
        auto result = datetime_from_str("today+1day");
        REQUIRE(result.has_value());
    }

    SECTION("handles relative offset with hours") {
        auto result = datetime_from_str("now-2hours");
        REQUIRE(result.has_value());
    }

    SECTION("parses absolute date in YYYYMMDD format") {
        auto result = datetime_from_str("20230115");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 1673740800);  // 2023-01-15 00:00:00 UTC
    }

    SECTION("returns nullopt for empty string") {
        auto result = datetime_from_str("");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("hyphenate_date formats dates with hyphens", "[utils][date]") {
    SECTION("converts YYYYMMDD to YYYY-MM-DD") {
        auto result = hyphenate_date("20230115");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "2023-01-15");
    }

    SECTION("returns nullopt for wrong length") {
        auto result = hyphenate_date("2023");
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("returns nullopt for non-numeric string") {
        auto result = hyphenate_date("abcd efgh");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("time_seconds calculates total seconds", "[utils][date]") {
    SECTION("calculates seconds from hours and minutes") {
        double result = time_seconds(0, 1, 30);  // 1 hour 30 minutes
        REQUIRE(result == 5400.0);
    }

    SECTION("calculates seconds from days") {
        double result = time_seconds(1);  // 1 day
        REQUIRE(result == 86400.0);
    }

    SECTION("calculates seconds with fractional seconds") {
        double result = time_seconds(0, 0, 0, 1.5);  // 1.5 seconds
        REQUIRE(result == 1.5);
    }

    SECTION("calculates seconds with milliseconds") {
        double result = time_seconds(0, 0, 0, 0.0, 500);  // 500 milliseconds
        REQUIRE(result == 0.5);
    }

    SECTION("calculates seconds with microseconds") {
        double result = time_seconds(0, 0, 0, 0.0, 0, 500000);  // 500000 microseconds
        REQUIRE(result == 0.5);
    }

    SECTION("calculates seconds with mixed units") {
        double result = time_seconds(1, 2, 30, 45.5, 500, 250000);
        // 1 day + 2 hours + 30 minutes + 45.5 seconds + 500 ms + 250000 us
        // = 86400 + 7200 + 1800 + 45.5 + 0.5 + 0.25 = 95446.25
        REQUIRE(result == 95446.25);
    }
}

TEST_CASE("date_formats returns format strings", "[utils][date]") {
    SECTION("returns day-first formats when day_first=true") {
        auto formats = date_formats(true);
        REQUIRE_FALSE(formats.empty());
        // Should contain DD/MM/YYYY format
        bool has_day_first = false;
        for (const auto& fmt : formats) {
            if (fmt.find("%d/%m/%Y") != std::string::npos) {
                has_day_first = true;
                break;
            }
        }
        REQUIRE(has_day_first);
    }

    SECTION("returns month-first formats when day_first=false") {
        auto formats = date_formats(false);
        REQUIRE_FALSE(formats.empty());
        // Should contain MM/DD/YYYY format
        bool has_month_first = false;
        for (const auto& fmt : formats) {
            if (fmt.find("%m/%d/%Y") != std::string::npos) {
                has_month_first = true;
                break;
            }
        }
        REQUIRE(has_month_first);
    }
}
