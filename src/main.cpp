#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/extractor/youtube.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cxxopts.hpp>
#include <fmt/core.h>

using namespace ytdlp;

/**
 * Select best video and audio formats for merging (like yt-dlp does).
 * Returns pair of (video_format, audio_format) or (progressive_format, empty) if no DASH available.
 */
std::pair<core::InfoDict, std::optional<core::InfoDict>> select_best_formats(
    const std::vector<core::InfoDict>& formats
) {
    if (formats.empty()) {
        throw std::runtime_error("No formats available");
    }

    std::vector<core::InfoDict> progressive_formats;
    std::vector<core::InfoDict> video_only_formats;
    std::vector<core::InfoDict> audio_only_formats;

    for (const auto& fmt : formats) {
        bool has_video = fmt.contains("vcodec") &&
                        fmt["vcodec"].is_string() &&
                        fmt["vcodec"].get<std::string>() != "none";
        bool has_audio = fmt.contains("acodec") &&
                        fmt["acodec"].is_string() &&
                        fmt["acodec"].get<std::string>() != "none";

        if (has_video && has_audio) {
            progressive_formats.push_back(fmt);
        } else if (has_video && !has_audio) {
            video_only_formats.push_back(fmt);
        } else if (!has_video && has_audio) {
            audio_only_formats.push_back(fmt);
        }
    }

    // Find best video-only format (prefer AV1 > VP9 > H.264, then highest resolution, then lower filesize)
    // This matches Python yt-dlp's behavior - it prefers format 401 (AV1) over 313 (VP9) at 2160p
    auto get_codec_score = [](const std::string& vcodec) -> int {
        if (vcodec.find("av01") != std::string::npos || vcodec.find("av1") != std::string::npos) {
            return 300;  // AV1 - most efficient, modern codec
        } else if (vcodec.find("vp9") != std::string::npos || vcodec.find("vp09") != std::string::npos) {
            return 200;  // VP9 - good efficiency
        } else if (vcodec.find("avc") != std::string::npos || vcodec.find("h264") != std::string::npos) {
            return 100;  // H.264/AVC - older but compatible
        }
        return 50;  // Unknown codec
    };

    core::InfoDict best_video;
    int best_score = -1;

    for (const auto& fmt : video_only_formats) {
        int height = 0;
        if (fmt.contains("height") && fmt["height"].is_number()) {
            height = fmt["height"].get<int>();
        }

        std::string vcodec = "none";
        if (fmt.contains("vcodec") && fmt["vcodec"].is_string()) {
            vcodec = fmt["vcodec"].get<std::string>();
        }

        int64_t filesize = 0;
        if (fmt.contains("filesize") && fmt["filesize"].is_number()) {
            filesize = fmt["filesize"].get<int64_t>();
        }

        // Score = resolution_priority + codec_preference - filesize_penalty
        // Higher resolution = higher score
        // Better codec (AV1 > VP9 > H.264) = higher score
        // Smaller file at same resolution = higher score (more reliable, less likely to timeout)
        int codec_score = get_codec_score(vcodec);
        int resolution_score = height;  // 2160, 1440, 1080, etc.
        int filesize_penalty = (filesize > 0) ? static_cast<int>(filesize / 10000000) : 0;  // Penalize large files slightly

        int total_score = resolution_score + codec_score - filesize_penalty;

        if (total_score > best_score) {
            best_video = fmt;
            best_score = total_score;
        }
    }

    // Find best audio-only format (highest bitrate)
    core::InfoDict best_audio;
    int best_audio_bitrate = 0;
    for (const auto& fmt : audio_only_formats) {
        int bitrate = 0;
        if (fmt.contains("abr") && fmt["abr"].is_number()) {
            bitrate = fmt["abr"].get<int>();
        }
        if (bitrate > best_audio_bitrate) {
            best_audio = fmt;
            best_audio_bitrate = bitrate;
        }
    }

    // If we have both best video and best audio, return them for merging
    if (best_score > -1 && best_audio_bitrate > 0) {
        return {best_video, best_audio};
    }

    // Fallback: find best progressive format
    core::InfoDict best_progressive;
    int best_progressive_height = 0;
    for (const auto& fmt : progressive_formats) {
        int height = 0;
        if (fmt.contains("height") && fmt["height"].is_number()) {
            height = fmt["height"].get<int>();
        }
        if (height > best_progressive_height) {
            best_progressive = fmt;
            best_progressive_height = height;
        }
    }

    if (best_progressive_height > 0) {
        return {best_progressive, std::nullopt};
    }

    // Last resort: return first format with URL
    for (const auto& fmt : formats) {
        if (fmt.contains("url") && fmt["url"].is_string()) {
            return {fmt, std::nullopt};
        }
    }

    throw std::runtime_error("No downloadable format found");
}

/**
 * Download a file from URL to output path using streaming.
 */
bool download_file(const std::string& url, const std::string& output_path,
                   networking::CurlHttpClient& http_client) {
    fmt::print("Downloading from: {}\n", url);
    fmt::print("Saving to: {}\n", output_path);

    // Create headers for YouTube-compatible download
    std::map<std::string, std::string> headers = {
        {"User-Agent", "com.google.android.youtube/19.09.37 (Linux; U; Android 11) gzip"},
        {"Accept", "*/*"},
        {"Accept-Language", "en-US,en;q=0.9"},
        {"Accept-Encoding", "gzip, deflate"},
        {"Range", "bytes=0-"}
    };

    // Progress tracking
    auto start_time = std::chrono::steady_clock::now();
    int64_t last_bytes = 0;
    auto last_update = start_time;

    // Progress callback - shows download progress like yt-dlp
    auto progress_callback = [&](int64_t downloaded, int64_t total) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();

        // Update every 500ms to avoid spamming
        if (elapsed > 500 || downloaded == total) {
            double speed = 0.0;
            if (elapsed > 0) {
                speed = (downloaded - last_bytes) / (elapsed / 1000.0); // bytes per second
            }

            if (total > 0) {
                double percent = (100.0 * downloaded) / total;
                double total_mb = total / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);

                // Calculate ETA
                int64_t remaining = total - downloaded;
                int eta_seconds = (speed > 0) ? (remaining / speed) : 0;

                fmt::print("\r[download] {:.1f}% of {:.2f}MB at {:.2f}MB/s ETA {:02d}:{:02d}",
                          percent, total_mb, speed_mb, eta_seconds / 60, eta_seconds % 60);
                std::fflush(stdout);
            } else {
                // Total size unknown
                double downloaded_mb = downloaded / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);
                fmt::print("\r[download] {:.2f}MB at {:.2f}MB/s", downloaded_mb, speed_mb);
                std::fflush(stdout);
            }

            last_bytes = downloaded;
            last_update = now;
        }
    };

    try {
        // Use streaming download
        bool success = http_client.download_to_file(url, output_path, headers, progress_callback);

        if (success) {
            fmt::print("\nDownload complete!\n");
            return true;
        } else {
            fmt::print("\n");
            return false;
        }

    } catch (const std::exception& e) {
        fmt::print("\nDownload failed: {}\n", e.what());
        return false;
    }
}

int main(int argc, char** argv) {
    try {
        // Parse command line options
        cxxopts::Options options("yt-dlp-cpp", "YouTube video downloader (C++ port)");
        options.add_options()
            ("u,url", "Video URL to download", cxxopts::value<std::string>())
            ("o,output", "Output filename", cxxopts::value<std::string>()->default_value("video.mp4"))
            ("q,quiet", "Quiet mode", cxxopts::value<bool>()->default_value("false"))
            ("i,info", "Print video info only (don't download)", cxxopts::value<bool>()->default_value("false"))
            ("h,help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help") || !result.count("url")) {
            fmt::print("{}\n", options.help());
            return 0;
        }

        std::string url = result["url"].as<std::string>();
        std::string output = result["output"].as<std::string>();
        bool quiet = result["quiet"].as<bool>();
        bool info_only = result["info"].as<bool>();

        if (!quiet) {
            fmt::print("yt-dlp-cpp - YouTube Downloader (C++ Port)\n");
            fmt::print("============================================\n\n");
        }

        // Initialize YoutubeDL
        core::YoutubeDLParams params;
        params.quiet = quiet;
        core::YoutubeDL ydl(params);

        // Create YouTube extractor
        extractor::YoutubeIE youtube(&ydl);

        if (!quiet) {
            fmt::print("Extracting video info...\n");
        }

        // Extract video information
        fmt::print("Calling extract()...\n");
        core::InfoDict info = youtube.extract(url);
        fmt::print("Extract completed successfully!\n");

        if (!quiet || info_only) {
            fmt::print("\nVideo Information:\n");
            fmt::print("------------------\n");

            if (info.contains("title")) {
                fmt::print("Title: {}\n", info["title"].get<std::string>());
            }
            if (info.contains("id")) {
                fmt::print("ID: {}\n", info["id"].get<std::string>());
            }
            if (info.contains("uploader")) {
                fmt::print("Uploader: {}\n", info["uploader"].get<std::string>());
            }
            if (info.contains("duration")) {
                int duration = info["duration"].get<int>();
                fmt::print("Duration: {}:{:02d}\n", duration / 60, duration % 60);
            }
            if (info.contains("view_count")) {
                fmt::print("Views: {}\n", info["view_count"].get<int64_t>());
            }

            if (info.contains("formats") && info["formats"].is_array()) {
                fmt::print("\nAvailable formats: {}\n", info["formats"].size());

                // Debug: Show all formats (or first 20 if too many)
                if (!quiet) {
                    auto formats = info["formats"].get<std::vector<core::InfoDict>>();
                    size_t max_show = std::min(formats.size(), size_t(20));
                    for (size_t i = 0; i < max_show; i++) {
                        const auto& fmt = formats[i];
                        fmt::print("  Format #{}: ", i+1);
                        if (fmt.contains("format_id")) {
                            fmt::print("itag={} ", fmt["format_id"].get<std::string>());
                        }
                        if (fmt.contains("ext")) {
                            fmt::print("{} ", fmt["ext"].get<std::string>());
                        }
                        if (fmt.contains("height")) {
                            fmt::print("{}p ", fmt["height"].get<int>());
                        }
                        if (fmt.contains("vcodec")) {
                            fmt::print("vcodec={} ", fmt["vcodec"].get<std::string>());
                        }
                        if (fmt.contains("acodec")) {
                            fmt::print("acodec={} ", fmt["acodec"].get<std::string>());
                        }
                        if (fmt.contains("filesize")) {
                            double mb = fmt["filesize"].get<int64_t>() / (1024.0 * 1024.0);
                            fmt::print("size={:.1f}MB ", mb);
                        }
                        fmt::print("\n");
                    }
                    if (formats.size() > max_show) {
                        fmt::print("  ... and {} more formats\n", formats.size() - max_show);
                    }
                }
            }
        }

        if (info_only) {
            return 0;
        }

        // Get formats
        if (!info.contains("formats") || !info["formats"].is_array()) {
            fmt::print(stderr, "Error: No formats available\n");
            return 1;
        }

        std::vector<core::InfoDict> formats = info["formats"].get<std::vector<core::InfoDict>>();

        if (!quiet) {
            fmt::print("\nSelecting best formats...\n");
        }

        // Select best video and audio formats (like yt-dlp)
        auto [video_format, audio_format_opt] = select_best_formats(formats);

        if (!video_format.contains("url")) {
            fmt::print(stderr, "Error: Selected video format has no URL\n");
            return 1;
        }

        networking::CurlHttpClient http_client;

        // If we have separate video and audio, download and merge
        if (audio_format_opt.has_value()) {
            auto& audio_format = audio_format_opt.value();

            if (!quiet) {
                fmt::print("Selected video: ");
                if (video_format.contains("format_id")) fmt::print("itag {}", video_format["format_id"].get<std::string>());
                if (video_format.contains("height")) fmt::print(" {}p", video_format["height"].get<int>());
                fmt::print("\n");

                fmt::print("Selected audio: ");
                if (audio_format.contains("format_id")) fmt::print("itag {}", audio_format["format_id"].get<std::string>());
                if (audio_format.contains("abr")) fmt::print(" {}kbps", audio_format["abr"].get<int>());
                fmt::print("\n\n");
            }

            // Download video and audio to temporary files
            std::string video_temp = output + ".video.tmp";
            std::string audio_temp = output + ".audio.tmp";

            std::string video_url = video_format["url"].get<std::string>();
            std::string audio_url = audio_format["url"].get<std::string>();

            if (!quiet) fmt::print("Downloading video...\n");
            if (!download_file(video_url, video_temp, http_client)) {
                // Fallback: try progressive format if DASH fails
                fmt::print(stderr, "\nVideo download failed (403 - restricted). Falling back to progressive format...\n");

                // Find best progressive format
                core::InfoDict progressive_format;
                int best_height = 0;
                for (const auto& fmt : formats) {
                    bool has_video = fmt.contains("vcodec") &&
                                    fmt["vcodec"].is_string() &&
                                    fmt["vcodec"].get<std::string>() != "none";
                    bool has_audio = fmt.contains("acodec") &&
                                    fmt["acodec"].is_string() &&
                                    fmt["acodec"].get<std::string>() != "none";

                    if (has_video && has_audio) {
                        int height = 0;
                        if (fmt.contains("height") && fmt["height"].is_number()) {
                            height = fmt["height"].get<int>();
                        }
                        if (height > best_height) {
                            progressive_format = fmt;
                            best_height = height;
                        }
                    }
                }

                if (best_height == 0 || !progressive_format.contains("url")) {
                    fmt::print(stderr, "✗ No progressive format available\n");
                    return 1;
                }

                fmt::print("Trying progressive format: {}p\n", best_height);
                std::string progressive_url = progressive_format["url"].get<std::string>();
                if (!download_file(progressive_url, output, http_client)) {
                    fmt::print(stderr, "\n✗ Progressive format download also failed\n");
                    return 1;
                }

                if (!quiet) {
                    fmt::print("\n✓ Download successful (progressive format)!\n");
                    fmt::print("  Saved to: {}\n", output);
                }
                return 0;
            }

            if (!quiet) fmt::print("\nDownloading audio...\n");
            if (!download_file(audio_url, audio_temp, http_client)) {
                fmt::print(stderr, "\n✗ Audio download failed\n");
                std::remove(video_temp.c_str());
                return 1;
            }

            // Merge using FFmpeg
            if (!quiet) fmt::print("\nMerging video and audio with FFmpeg...\n");
            std::string ffmpeg_cmd = fmt::format(
                "ffmpeg -i \"{}\" -i \"{}\" -c copy -y \"{}\" 2>&1",
                video_temp, audio_temp, output
            );

            int ffmpeg_result = std::system(ffmpeg_cmd.c_str());

            // Clean up temporary files
            std::remove(video_temp.c_str());
            std::remove(audio_temp.c_str());

            if (ffmpeg_result != 0) {
                fmt::print(stderr, "\n✗ FFmpeg merge failed. Make sure ffmpeg is installed.\n");
                return 1;
            }

            if (!quiet) {
                fmt::print("\n✓ Download and merge successful!\n");
                fmt::print("  Saved to: {}\n", output);
            }
            return 0;

        } else {
            // Progressive format - direct download
            if (!quiet) {
                fmt::print("Selected format: ");
                if (video_format.contains("format_id")) {
                    fmt::print("itag {}", video_format["format_id"].get<std::string>());
                }
                if (video_format.contains("ext")) {
                    fmt::print(" ({})", video_format["ext"].get<std::string>());
                }
                if (video_format.contains("height")) {
                    fmt::print(" {}p", video_format["height"].get<int>());
                }
                fmt::print(" (progressive)\n\n");
            }

            std::string download_url = video_format["url"].get<std::string>();
            bool success = download_file(download_url, output, http_client);

            if (success) {
                if (!quiet) {
                    fmt::print("\n✓ Download successful!\n");
                    fmt::print("  Saved to: {}\n", output);
                }
                return 0;
            } else {
                fmt::print(stderr, "\n✗ Download failed\n");
                return 1;
            }
        }

    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}
