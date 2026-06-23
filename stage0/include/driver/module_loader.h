#pragma once
#include <basic/symbols/module.h>

namespace veo::driver {

class ModuleLoader {
    static inline std::unordered_map<std::string, symbols::Module *>
        _loadedMods; // NOLINT

public:
    static symbols::Module *
    LoadModule (const std::string &importPath) {
        if (auto it = _loadedMods.find (importPath); it != _loadedMods.end ()) {
            return it->second;
        }
        return nullptr;
    }

    static void
    AddModule (const std::string &importPath, symbols::Module *mod) {
        if (!_loadedMods.contains (importPath)) {
            _loadedMods.emplace (importPath, mod);
        }
    }
};

}
