#include "jsonl_writer.hpp"

#include <fstream>
#include <stdexcept>

void write_jsonl(const std::string& path, const std::vector<Song>& songs) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    for (const auto& song : songs) {
        out << "{\"track_id\":\"" << song.track_id
            << "\",\"title\":\"" << song.title
            << "\",\"artist\":\"" << song.artist
            << "\"}\n";
    }
}