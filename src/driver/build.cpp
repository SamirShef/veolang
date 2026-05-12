#include "basic/symbols/module.h"

#include <driver/build.h>
#include <driver/compiler.h>
#include <filesystem>
#include <llvm/Support/raw_ostream.h>
#include <toml++/toml.h>

namespace veo::driver {

fs::path
BuildDriver::GetProjectRoot (fs::path curPath) {
    while (curPath != curPath.root_path () && !fs::exists (curPath / "veo.toml")) {
        curPath = curPath.parent_path ();
    }
    if (curPath == curPath.root_path ()) {
        // TODO: report normal error
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "Manifest file 'veo.toml' does not exists\n"
                      << llvm::raw_fd_ostream::RESET;
        exit (1);
    }
    return curPath;
}

void
BuildDriver::Build () {
    Manifest manif = parseManifest (_projectRoot / "veo.toml");
    llvm::errs () << "Building package: " << manif.ProjectName << '\n';

    std::ifstream file (manif.EntryPointPath);
    if (!file.is_open ()) {
        llvm::errs () << llvm::raw_fd_ostream::RED << "Error: Could not open the file "
                      << manif.EntryPointPath.string () << '\n'
                      << llvm::raw_fd_ostream::RESET;
        exit (1);
    }

    auto *mod        = new symbols::Module (manif.EntryPointPath.stem ().string ());
    auto  compileRes = Compile (_projectRoot, manif.EntryPointPath, mod);
    if (!compileRes.Success) {
        exit (1);
    }
}

Manifest
BuildDriver::parseManifest (const fs::path &path) {
    if (!fs::exists (path)) {
        // TODO: report normal error
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "Package does not have manifest file 'veo.toml'\n"
                      << llvm::raw_fd_ostream::RESET;
        exit (1);
    }
    toml::table manif     = toml::parse_file (path.string ()).table ();
    std::string pkgName   = manif["package"]["name"].value_or ("");
    std::string entryPath = manif["package"]["entry"].value_or ("");
    if (pkgName.empty () || entryPath.empty ()) {
        // TODO: report normal error
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "Manifest file 'veo.toml' is incorrect (";
        if (pkgName.empty ()) {
            llvm::errs () << "does not have package name";
        }
        if (entryPath.empty ()) {
            llvm::errs () << (pkgName.empty () ? ", " : "")
                          << "does not have entry point path";
        }
        llvm::errs () << ")\n" << llvm::raw_fd_ostream::RESET;
        exit (1);
    }
    return { .ProjectName    = pkgName,
             .ManifestPath   = fs::absolute (path),
             .EntryPointPath = fs::path (entryPath) };
}

}
