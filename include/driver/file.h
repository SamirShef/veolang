#pragma once
#include <basic/symbols/module.h>
#include <filesystem>
#include <vector>

namespace veo::driver {

namespace fs = std::filesystem;

enum class VisitState : uint8_t { Unvisited, Visiting, Visited };

struct File {
    fs::path                 Path;
    std::vector<std::string> Deps;
    VisitState               State = VisitState::Unvisited;
    symbols::Module         *Mod   = nullptr;
};

}
