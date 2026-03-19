#pragma once

#include <string>

struct Paths {
    std::string raw_data_dir = "data/raw/";
    std::string output_dir = "data/output/";
};

inline Paths PATHS;