#include "engine.h"
#include "get_file_path.h"

#include <filesystem>

int
main () {
    auto veoDir = GetPath ("veo");
    if (!std::filesystem::exists (veoDir)) {
        llvm::errs () << "No directory " << veoDir.string () << '\n';
        return 1;
    }
    bool ok = true;
    for (const auto &entry : std::filesystem::directory_iterator (veoDir)) {
        const auto &path = entry.path ();
        llvm::errs () << path.string () << ":\n";
        if (!TestEngine::Test (path)) {
            ok = false;
        } else {
            llvm::errs () << "success\n";
        }
    }
    return static_cast<int> (!ok);
}
