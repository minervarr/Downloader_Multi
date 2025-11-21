/**
 * Unit Tests for Filesystem Utilities
 */

#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/filesystem_utils.hpp"

using namespace ytdlp::utils;

// ============================================================================
// Path Manipulation Tests
// ============================================================================

TEST_CASE("join_path combines path components", "[utils][filesystem]") {
    SECTION("joins two paths") {
        REQUIRE(join_path("dir", "file.txt") == "dir/file.txt");
    }

    SECTION("joins multiple paths") {
        std::vector<std::string> parts = {"root", "sub", "file.txt"};
        REQUIRE(join_path(parts) == "root/sub/file.txt");
    }

    SECTION("handles empty parts") {
        std::vector<std::string> parts;
        REQUIRE(join_path(parts) == "");
    }
}

TEST_CASE("basename extracts filename", "[utils][filesystem]") {
    std::string result1 = ytdlp::utils::basename("/path/to/file.txt");
    std::string result2 = ytdlp::utils::basename("file.txt");
    std::string result3 = ytdlp::utils::basename("/path/to/dir/");

    REQUIRE(result1 == "file.txt");
    REQUIRE(result2 == "file.txt");
    REQUIRE(result3 == "");
}

TEST_CASE("dirname extracts directory path", "[utils][filesystem]") {
    REQUIRE(dirname("/path/to/file.txt") == "/path/to");
    REQUIRE(dirname("file.txt") == "");
}

TEST_CASE("get_extension extracts file extension", "[utils][filesystem]") {
    REQUIRE(get_extension("file.txt") == ".txt");
    REQUIRE(get_extension("file.tar.gz") == ".gz");
    REQUIRE(get_extension("noext") == "");
}

TEST_CASE("get_stem extracts filename without extension", "[utils][filesystem]") {
    REQUIRE(get_stem("file.txt") == "file");
    REQUIRE(get_stem("file.tar.gz") == "file.tar");
    REQUIRE(get_stem("/path/to/file.txt") == "file");
}

TEST_CASE("replace_extension changes extension", "[utils][filesystem]") {
    REQUIRE(replace_extension("video.flv", ".mp4") == "video.mp4");
    REQUIRE(replace_extension("video.flv", "mp4") == "video.mp4");
    REQUIRE(replace_extension("noext", ".txt") == "noext.txt");
}

TEST_CASE("prepend_extension adds extension before existing one", "[utils][filesystem]") {
    REQUIRE(prepend_extension("video.mp4", ".temp") == "video.temp.mp4");
    REQUIRE(prepend_extension("video.mp4", "temp") == "video.temp.mp4");
}

// ============================================================================
// File Operations Tests
// ============================================================================

TEST_CASE("file operations work correctly", "[utils][filesystem][io]") {
    std::string test_dir = get_temp_directory() + "/ytdlp_test";
    std::string test_file = test_dir + "/test.txt";

    // Clean up from previous runs
    if (path_exists(test_dir)) {
        remove_directory(test_dir);
    }

    SECTION("directory creation") {
        REQUIRE(make_directory(test_dir));
        REQUIRE(path_exists(test_dir));
        REQUIRE(is_directory(test_dir));
        REQUIRE_FALSE(is_file(test_dir));
    }

    SECTION("file writing and reading") {
        make_directory(test_dir);

        std::string content = "Hello, yt-dlp-cpp!";
        REQUIRE(write_file(test_file, content));
        REQUIRE(path_exists(test_file));
        REQUIRE(is_file(test_file));

        auto read_content = read_file(test_file);
        REQUIRE(read_content.has_value());
        REQUIRE(*read_content == content);
    }

    SECTION("file size") {
        make_directory(test_dir);
        write_file(test_file, "test");

        auto size = file_size(test_file);
        REQUIRE(size.has_value());
        REQUIRE(*size == 4);
    }

    SECTION("file copying") {
        make_directory(test_dir);
        write_file(test_file, "original");

        std::string copy_path = test_dir + "/copy.txt";
        REQUIRE(copy_file(test_file, copy_path));
        REQUIRE(path_exists(copy_path));

        auto content = read_file(copy_path);
        REQUIRE(content.has_value());
        REQUIRE(*content == "original");
    }

    SECTION("file moving") {
        make_directory(test_dir);
        write_file(test_file, "move me");

        std::string new_path = test_dir + "/moved.txt";
        REQUIRE(move_file(test_file, new_path));
        REQUIRE(path_exists(new_path));
        REQUIRE_FALSE(path_exists(test_file));
    }

    SECTION("directory listing") {
        make_directory(test_dir);
        write_file(test_dir + "/file1.txt", "1");
        write_file(test_dir + "/file2.txt", "2");
        make_directory(test_dir + "/subdir");

        auto files = list_directory(test_dir, true);  // files only
        REQUIRE(files.size() == 2);

        auto all = list_directory(test_dir, false);  // files and dirs
        REQUIRE(all.size() == 3);
    }

    // Cleanup
    if (path_exists(test_dir)) {
        remove_directory(test_dir);
    }
}

// ============================================================================
// Binary File Operations Tests
// ============================================================================

TEST_CASE("binary file operations", "[utils][filesystem][binary]") {
    std::string test_dir = get_temp_directory() + "/ytdlp_test";
    std::string test_file = test_dir + "/binary.dat";

    // Clean up
    if (path_exists(test_dir)) {
        remove_directory(test_dir);
    }

    make_directory(test_dir);

    SECTION("write and read binary data") {
        std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};

        REQUIRE(write_binary_file(test_file, data));

        auto read_data = read_binary_file(test_file);
        REQUIRE(read_data.has_value());
        REQUIRE(*read_data == data);
    }

    // Cleanup
    remove_directory(test_dir);
}

// ============================================================================
// Filename Generation Tests
// ============================================================================

TEST_CASE("subtitles_filename generates correct filename", "[utils][filesystem]") {
    REQUIRE(subtitles_filename("video.mp4", "en", "srt") == "video.en.srt");
    REQUIRE(subtitles_filename("video.mp4", "en", ".srt") == "video.en.srt");
    REQUIRE(subtitles_filename("/path/to/video.mp4", "fr", "vtt") == "/path/to/video.fr.vtt");
}

TEST_CASE("temp_filename generates unique filenames", "[utils][filesystem]") {
    std::string temp1 = temp_filename("video.mp4", "tmp");
    std::string temp2 = temp_filename("video.mp4", "tmp");

    REQUIRE_FALSE(temp1.empty());
    REQUIRE_FALSE(temp2.empty());
    REQUIRE(temp1 != temp2);  // Should be unique
}

// ============================================================================
// Platform Utilities Tests
// ============================================================================

TEST_CASE("platform utilities work", "[utils][filesystem][platform]") {
    SECTION("get_home_directory returns non-empty path") {
        std::string home = get_home_directory();
        REQUIRE_FALSE(home.empty());
        REQUIRE(is_directory(home));
    }

    SECTION("get_current_directory returns valid path") {
        std::string cwd = get_current_directory();
        REQUIRE_FALSE(cwd.empty());
        REQUIRE(is_directory(cwd));
    }

    SECTION("get_temp_directory returns valid path") {
        std::string temp = get_temp_directory();
        REQUIRE_FALSE(temp.empty());
        REQUIRE(is_directory(temp));
    }

    SECTION("path separator is correct") {
        char sep = get_path_separator();
#ifdef _WIN32
        REQUIRE(sep == '\\');
#else
        REQUIRE(sep == '/');
#endif
    }
}

TEST_CASE("path conversion works", "[utils][filesystem][path]") {
    SECTION("to_unix_path converts backslashes") {
        REQUIRE(to_unix_path("C:\\Users\\test\\file.txt") == "C:/Users/test/file.txt");
    }

    SECTION("to_native_path uses platform separator") {
        std::string path = to_native_path("dir/subdir/file.txt");
#ifdef _WIN32
        REQUIRE(path == "dir\\subdir\\file.txt");
#else
        REQUIRE(path == "dir/subdir/file.txt");
#endif
    }
}

// ============================================================================
// Path Expansion Tests
// ============================================================================

TEST_CASE("expand_path expands tilde", "[utils][filesystem][expansion]") {
    SECTION("expands ~ to home directory") {
        std::string expanded = expand_path("~");
        REQUIRE(expanded == get_home_directory());
    }

    SECTION("expands ~/path") {
        std::string expanded = expand_path("~/Downloads");
        std::string expected = get_home_directory() + "/Downloads";
        REQUIRE(expanded == expected);
    }

    SECTION("does not expand middle ~") {
        std::string path = "/path/~something";
        REQUIRE(expand_path(path) == path);
    }
}
