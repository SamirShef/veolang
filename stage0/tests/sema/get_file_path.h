#pragma once
#include <filesystem>
#include <string>

inline std::filesystem::path
GetPath (const std::string &name) {
    return std::filesystem::path (std::string (SEMA_TEST_DIR)) / name;
}
