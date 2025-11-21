#include <catch2/catch_test_macros.hpp>
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"
#include <filesystem>
#include <fstream>

using namespace ytdlp::core;
using namespace ytdlp::core::info;

TEST_CASE("YoutubeDL construction", "[core][youtube_dl]") {
    SECTION("default construction") {
        YoutubeDL ydl;

        REQUIRE_FALSE(ydl.params().quiet);
        REQUIRE_FALSE(ydl.params().verbose);
        REQUIRE(ydl.params().socket_timeout == 60);
    }

    SECTION("construction with custom params") {
        YoutubeDLParams params;
        params.quiet = true;
        params.verbose = false;
        params.socket_timeout = 30;
        params.format = "best";

        YoutubeDL ydl(params);

        REQUIRE(ydl.params().quiet == true);
        REQUIRE(ydl.params().socket_timeout == 30);
        REQUIRE(ydl.params().format.value() == "best");
    }
}

TEST_CASE("YoutubeDL HTTP client access", "[core][youtube_dl]") {
    YoutubeDL ydl;

    SECTION("HTTP client is initialized") {
        // HTTP client should be accessible
        auto& client = ydl.http_client();

        // Verify configuration was applied
        REQUIRE(client.config().timeout == 60);
    }
}

TEST_CASE("YoutubeDL cookie loading", "[core][youtube_dl]") {
    // Create a temporary cookie file
    std::string temp_cookie_file = "/tmp/test_cookies_" + std::to_string(std::time(nullptr)) + ".txt";

    {
        std::ofstream file(temp_cookie_file);
        file << "# Netscape HTTP Cookie File\n";
        file << "example.com\tFALSE\t/\tFALSE\t9999999999\ttest\tvalue\n";
    }

    SECTION("loads cookies from file") {
        YoutubeDLParams params;
        params.cookie_file = temp_cookie_file;

        YoutubeDL ydl(params);

        auto jar = ydl.cookie_jar();
        REQUIRE(jar != nullptr);
        REQUIRE(jar->size() == 1);

        // Verify cookie was loaded
        std::string header = jar->get_cookie_header("http://example.com/");
        REQUIRE(header == "test=value");
    }

    SECTION("handles missing cookie file gracefully") {
        YoutubeDLParams params;
        params.cookie_file = "/nonexistent/cookies.txt";

        // Should not throw, just warn
        REQUIRE_NOTHROW(YoutubeDL(params));
    }

    // Clean up
    std::filesystem::remove(temp_cookie_file);
}

TEST_CASE("YoutubeDL extract_info", "[core][youtube_dl]") {
    YoutubeDL ydl;

    SECTION("extracts info from URL") {
        std::string url = "https://example.com/video";

        InfoDict info = ydl.extract_info(url, false);

        // Verify basic info dict structure
        REQUIRE(info.contains("url"));
        REQUIRE(get_string(info, "url") == url);
        REQUIRE(info.contains("_type"));
    }

    SECTION("flat extraction") {
        YoutubeDLParams params;
        params.extract_flat = true;

        YoutubeDL ydl(params);

        InfoDict info = ydl.extract_info("https://example.com/playlist", false);

        REQUIRE(info.contains("url"));
    }
}

TEST_CASE("YoutubeDL progress hooks", "[core][youtube_dl]") {
    YoutubeDL ydl;

    SECTION("can add progress hooks") {
        bool hook_called = false;
        InfoDict captured_status;

        ydl.add_progress_hook([&](const InfoDict& status) {
            hook_called = true;
            captured_status = status;
        });

        // Hooks are stored
        // (We can't easily test if they're called without a real download)
    }
}

TEST_CASE("YoutubeDL download", "[core][youtube_dl]") {
    SECTION("download with skip_download") {
        YoutubeDLParams params;
        params.skip_download = true;
        params.quiet = true;  // Suppress output during tests

        YoutubeDL ydl(params);

        bool result = ydl.download_single("https://example.com/video");

        REQUIRE(result == true);
    }

    SECTION("download multiple URLs") {
        YoutubeDLParams params;
        params.skip_download = true;
        params.quiet = true;

        YoutubeDL ydl(params);

        std::vector<std::string> urls = {
            "https://example.com/video1",
            "https://example.com/video2",
            "https://example.com/video3"
        };

        int count = ydl.download(urls);

        REQUIRE(count == 3);
    }

    SECTION("handles errors with ignore_errors") {
        YoutubeDLParams params;
        params.ignore_errors = true;
        params.quiet = true;

        YoutubeDL ydl(params);

        std::vector<std::string> urls = {
            "https://example.com/video1",
            "",  // Invalid URL
            "https://example.com/video3"
        };

        // Should not throw even with invalid URL
        REQUIRE_NOTHROW(ydl.download(urls));
    }
}

TEST_CASE("YoutubeDL reporting methods", "[core][youtube_dl]") {
    SECTION("quiet mode suppresses output") {
        YoutubeDLParams params;
        params.quiet = true;

        YoutubeDL ydl(params);

        // These should not throw and should not produce output
        REQUIRE_NOTHROW(ydl.report_error("test error"));
        REQUIRE_NOTHROW(ydl.report_warning("test warning"));
        REQUIRE_NOTHROW(ydl.to_stdout("test message"));
    }

    SECTION("no_warnings suppresses warnings") {
        YoutubeDLParams params;
        params.no_warnings = true;

        YoutubeDL ydl(params);

        REQUIRE_NOTHROW(ydl.report_warning("test warning"));
    }
}

TEST_CASE("InfoDict helper functions", "[core][info_dict]") {
    InfoDict info;
    info["title"] = "Test Video";
    info["id"] = "abc123";
    info["duration"] = 120;
    info["view_count"] = 1000000;
    info["average_rating"] = 4.5;
    info["is_live"] = true;

    SECTION("get_string") {
        REQUIRE(get_string(info, "title") == "Test Video");
        REQUIRE(get_string(info, "id") == "abc123");
        REQUIRE(get_string(info, "missing", "default") == "default");
    }

    SECTION("get_int") {
        REQUIRE(get_int(info, "duration") == 120);
        REQUIRE(get_int(info, "view_count") == 1000000);
        REQUIRE(get_int(info, "missing", 42) == 42);
    }

    SECTION("get_float") {
        REQUIRE(get_float(info, "average_rating") == 4.5);
        REQUIRE(get_float(info, "missing", 3.14) == 3.14);
    }

    SECTION("get_bool") {
        REQUIRE(get_bool(info, "is_live") == true);
        REQUIRE(get_bool(info, "missing", false) == false);
    }

    SECTION("get_string_opt") {
        auto title_opt = get_string_opt(info, "title");
        REQUIRE(title_opt.has_value());
        REQUIRE(title_opt.value() == "Test Video");

        auto missing_opt = get_string_opt(info, "missing");
        REQUIRE_FALSE(missing_opt.has_value());
    }
}

TEST_CASE("InfoDict type checking", "[core][info_dict]") {
    SECTION("is_video") {
        InfoDict video;
        video["_type"] = "video";
        REQUIRE(is_video(video));

        InfoDict default_video;
        // No _type field defaults to video
        REQUIRE(is_video(default_video));
    }

    SECTION("is_playlist") {
        InfoDict playlist;
        playlist["_type"] = "playlist";
        REQUIRE(is_playlist(playlist));

        InfoDict video;
        video["_type"] = "video";
        REQUIRE_FALSE(is_playlist(video));
    }

    SECTION("get_entries") {
        InfoDict playlist;
        playlist["_type"] = "playlist";

        InfoDict entry1, entry2;
        entry1["id"] = "video1";
        entry2["id"] = "video2";

        playlist["entries"] = nlohmann::json::array({entry1, entry2});

        auto entries = get_entries(playlist);
        REQUIRE(entries.size() == 2);
        REQUIRE(get_string(entries[0], "id") == "video1");
        REQUIRE(get_string(entries[1], "id") == "video2");
    }
}

TEST_CASE("YoutubeDL move semantics", "[core][youtube_dl]") {
    SECTION("move construction") {
        YoutubeDLParams params;
        params.quiet = true;

        YoutubeDL ydl1(params);
        YoutubeDL ydl2(std::move(ydl1));

        REQUIRE(ydl2.params().quiet == true);
    }

    SECTION("move assignment") {
        YoutubeDLParams params;
        params.verbose = true;

        YoutubeDL ydl1(params);
        YoutubeDL ydl2;

        ydl2 = std::move(ydl1);

        REQUIRE(ydl2.params().verbose == true);
    }
}
