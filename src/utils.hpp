#pragma once

#include <filesystem>

static inline uint32_t get_file_size(const char* file_path)
{
    std::filesystem::path path { file_path };
    return std::filesystem::file_size(path);
}
