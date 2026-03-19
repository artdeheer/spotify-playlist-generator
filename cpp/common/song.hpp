// cpp/common/song.hpp
#pragma once

#include <string>

struct Song {
    std::string track_id;
    std::string title = "";
    std::string artist = "";
};