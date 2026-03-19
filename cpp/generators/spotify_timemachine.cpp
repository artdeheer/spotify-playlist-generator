// history_analyzer.cpp
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "../common/config.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct PlayEvent {
    std::string track_uri;
    long long ms_played = 0;
    std::string timestamp;
    std::chrono::system_clock::time_point time_point;
};

struct GeneratorConfig {
    int timespan_days = 30;
    double min_listening_share_in_window = 0.03;
    int min_play_count_in_window = 10;
};

std::chrono::system_clock::time_point parse_timestamp_utc(const std::string& ts) {
    std::tm tm = {};
    std::istringstream ss(ts);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse timestamp: " + ts);
    }

#if defined(_WIN32)
    std::time_t time = _mkgmtime(&tm);
#else
    std::time_t time = timegm(&tm);
#endif

    if (time == -1) {
        throw std::runtime_error("Failed to convert timestamp: " + ts);
    }

    return std::chrono::system_clock::from_time_t(time);
}

std::vector<fs::path> list_json_files(const fs::path& directory) {
    std::vector<fs::path> files;

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        throw std::runtime_error("Directory does not exist: " + directory.string());
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::vector<PlayEvent> load_events_from_file(const fs::path& filepath) {
    std::ifstream in(filepath);
    if (!in) {
        throw std::runtime_error("Failed to open file: " + filepath.string());
    }

    json j;
    in >> j;

    if (!j.is_array()) {
        throw std::runtime_error("Expected JSON array in file: " + filepath.string());
    }

    std::vector<PlayEvent> events;
    events.reserve(j.size());

    for (const auto& item : j) {
        if (!item.is_object()) {
            continue;
        }

        if (!item.contains("spotify_track_uri") || item["spotify_track_uri"].is_null()) {
            continue;
        }

        const std::string track_uri = item["spotify_track_uri"].get<std::string>();
        if (track_uri.empty()) {
            continue;
        }

        if (!item.contains("ts") || item["ts"].is_null()) {
            continue;
        }

        PlayEvent event;
        event.track_uri = track_uri;
        event.timestamp = item["ts"].get<std::string>();
        event.time_point = parse_timestamp_utc(event.timestamp);

        if (item.contains("ms_played") && !item["ms_played"].is_null()) {
            event.ms_played = item["ms_played"].get<long long>();
        }

        events.push_back(std::move(event));
    }

    return events;
}

std::vector<PlayEvent> load_all_events(const fs::path& directory) {
    const auto files = list_json_files(directory);

    std::vector<PlayEvent> events;
    for (const auto& file : files) {
        auto file_events = load_events_from_file(file);
        events.insert(events.end(), file_events.begin(), file_events.end());
    }

    std::sort(events.begin(), events.end(), [](const PlayEvent& a, const PlayEvent& b) {
        return a.time_point < b.time_point;
    });

    return events;
}

std::vector<std::string> find_valid_songs(
    const std::vector<PlayEvent>& events,
    const GeneratorConfig& config
) {
    std::vector<std::string> valid_songs;
    std::unordered_set<std::string> seen;

    const auto window_size = std::chrono::hours(24 * config.timespan_days);

    for (size_t i = 0; i < events.size(); ++i) {
        const auto& subject = events[i];
        const auto window_end = subject.time_point + window_size;

        long long total_time_in_window = 0;
        long long subject_time_in_window = 0;
        int subject_play_count_in_window = 0;

        for (size_t j = i; j < events.size(); ++j) {
            const auto& comparator = events[j];

            if (comparator.time_point >= window_end) {
                break;
            }

            total_time_in_window += comparator.ms_played;

            if (comparator.track_uri == subject.track_uri) {
                subject_time_in_window += comparator.ms_played;
                ++subject_play_count_in_window;
            }
        }

        if (total_time_in_window <= 0) {
            continue;
        }

        const double listening_share =
            static_cast<double>(subject_time_in_window) /
            static_cast<double>(total_time_in_window);

        if (listening_share >= config.min_listening_share_in_window &&
            subject_play_count_in_window >= config.min_play_count_in_window) {
            if (seen.find(subject.track_uri) == seen.end()) {
                seen.insert(subject.track_uri);
                valid_songs.push_back(subject.track_uri);
            }
        }
    }

    return valid_songs;
}

void write_valid_songs_jsonl(
    const fs::path& output_file,
    const std::vector<std::string>& valid_songs
) {
    fs::create_directories(output_file.parent_path());

    std::ofstream out(output_file);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + output_file.string());
    }

    for (const auto& uri : valid_songs) {
        out << "{\"uri\":\"" << uri << "\"}\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: ./history_analyzer <user>\n";
            return 1;
        }

        const std::string user = argv[1];

        const fs::path input_dir = PATHS.raw_data_dir + user;
        const fs::path output_file = PATHS.output_dir + user + "/valid_songs.jsonl";

        GeneratorConfig config;
        config.timespan_days = 30;
        config.min_listening_share_in_window = 0.03;
        config.min_play_count_in_window = 10;

        const auto events = load_all_events(input_dir);

        std::cout << "Loaded " << events.size() << " valid play events.\n";

        const auto valid_songs = find_valid_songs(events, config);

        std::cout << "Found " << valid_songs.size() << " valid songs.\n";

        write_valid_songs_jsonl(output_file, valid_songs);

        std::cout << "Wrote output to: " << output_file << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}