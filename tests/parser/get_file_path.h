#pragma once
#include <filesystem>
#include <string>

inline std::filesystem::path
GetFilePath (const std::string &name) {
    return std::filesystem::path (std::string (PARSER_TEST_DIR)) / name;
}
