#pragma once

#include <string>
#include <vector>
#include "song.hpp"

void write_jsonl(const std::string& path, const std::vector<Song>& songs);