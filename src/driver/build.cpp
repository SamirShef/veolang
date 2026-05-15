#include <basic/symbols/module.h>
#include <driver/build.h>
#include <driver/compiler.h>
#include <filesystem>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
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

    auto *mod         = new symbols::Module (manif.ProjectName);
    auto  artefactDir = manif.ManifestPath.parent_path () / "build" / "obj";
    auto  objPath     = artefactDir
                        / fs::absolute (manif.EntryPointPath)
                              .parent_path ()
                              .lexically_relative (manif.ManifestPath.parent_path ())
                        / (mod->Name + ".o");
    objPath           = objPath.lexically_normal ();
    fs::create_directories (objPath.parent_path ());
    auto compileRes = Compile (_projectRoot, manif.EntryPointPath, objPath, mod);
    if (!compileRes.Success) {
        exit (1);
    }
    std::string  targetTripleStr = llvm::sys::getDefaultTargetTriple ();
    llvm::Triple triple (targetTripleStr);
    auto         exePath = artefactDir / GetOutputName (manif.ProjectName, triple);
    if (LinkObjectFiles (exePath, { objPath.string () })) {
        llvm::errs () << llvm::raw_fd_ostream::GREEN
                      << "SUCCESS: " << llvm::raw_fd_ostream::RESET << exePath.string ()
                      << "\n";
    } else {
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
