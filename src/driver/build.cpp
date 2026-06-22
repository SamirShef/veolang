#include <basic/symbols/module.h>
#include <basic/types/pool.h>
#include <bitcode/deserializer.h>
#include <bitcode/serializer.h>
#include <driver/build.h>
#include <driver/cli_options.h>
#include <driver/compiler.h>
#include <driver/deps_resolver.h>
#include <driver/module_loader.h>
#include <filesystem>
#include <fstream>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <sstream>
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
    const auto &entryModImportPath = fs::absolute (manif.EntryPointPath)
                                         .lexically_relative (_projectRoot / "src")
                                         .stem ()
                                         .string ();
    scanDeps (entryModImportPath);
    sortDeps (entryModImportPath);

    std::vector<std::string> objFiles;
    std::string              targetTripleStr = TargetTripleOpt.empty ()
                                                   ? llvm::sys::getDefaultTargetTriple ()
                                                   : TargetTripleOpt.getValue ();
    llvm::Triple             triple (targetTripleStr);
    auto artefactDir = manif.ManifestPath.parent_path ()
                       / ("build/targets/" + targetTripleStr + "/obj");

    basic::TypePool       typePool;
    bitcode::Serializer   serializer;
    bitcode::Deserializer deserializer (typePool);

    for (const auto &importPath : _compilationQueue) {
        const auto &fileItem    = _graph.at (importPath);
        const auto &compileUnit = fileItem.Path;

        auto objPath   = artefactDir
                         / fs::absolute (compileUnit)
                               .parent_path ()
                               .lexically_relative (manif.ManifestPath.parent_path ())
                         / (compileUnit.stem ().string () + ".o");
        objPath        = objPath.lexically_normal ();
        auto vmetaPath = fs::path (objPath).replace_extension (".vmeta");
        objFiles.push_back (objPath.string ());

        if (isArtefactsFresh (compileUnit, objPath, vmetaPath)) {
            auto *loadedMod = deserializer.DeserializeModule (vmetaPath);
            if (loadedMod != nullptr) {
                llvm::errs () << "  Loaded precompiled metadata for: " << importPath
                              << '\n';
                continue;
            }
            llvm::errs () << "Module " + vmetaPath.string () + " was not be loaded\n";
            exit (1);
        }

        fs::create_directories (objPath.parent_path ());

        auto *mod = new symbols::Module (compileUnit.stem ().string ());
        ModuleLoader::AddModule (importPath, mod);

        llvm::errs () << "  Compilation module " << mod->Name << '\n';

        auto compileRes
            = Compile (_projectRoot, compileUnit, objPath, mod, typePool, triple);
        if (!compileRes.Success) {
            exit (1);
        }

        serializer.SerializeModule (mod, vmetaPath);
    }

    for (const auto &cSrc : manif.CSources) {
        auto absoluteCSrc = manif.ManifestPath.parent_path () / cSrc;
        if (!fs::exists (absoluteCSrc)) {
            llvm::errs () << llvm::raw_fd_ostream::RED << "Error: C source file "
                          << cSrc.string () << " does not exist\n"
                          << llvm::raw_fd_ostream::RESET;
            exit (1);
        }

        auto cObjPath = artefactDir
                        / fs::absolute (absoluteCSrc)
                              .parent_path ()
                              .lexically_relative (manif.ManifestPath.parent_path ())
                        / (cSrc.stem ().string () + ".o");
        cObjPath      = cObjPath.lexically_normal ();

        fs::create_directories (cObjPath.parent_path ());

        if (!CompileCFile (
                targetTripleStr,
                absoluteCSrc.string (),
                cObjPath.string (),
                OptimizationLevelOpt)) {
            llvm::errs () << llvm::raw_fd_ostream::RED
                          << "Error: Failed to compile C source " << cSrc.string ()
                          << '\n'
                          << llvm::raw_fd_ostream::RESET;
            exit (1);
        }

        objFiles.push_back (cObjPath.string ());
    }

    auto exePath = artefactDir / GetOutputName (manif.ProjectName, triple);
    if (LinkObjectFiles (targetTripleStr, exePath.string (), objFiles)) {
        llvm::errs ().changeColor (llvm::raw_fd_ostream::GREEN, true)
            << "SUCCESS: " << llvm::raw_fd_ostream::RESET << exePath.string () << "\n";
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

    std::vector<fs::path> csources;
    if (auto *arr = manif["c"]["sources"].as_array ()) {
        for (auto &el : *arr) {
            if (auto *s = el.as_string ()) {
                csources.emplace_back (s->get ());
            }
        }
    }
    return { .ProjectName    = pkgName,
             .ManifestPath   = fs::absolute (path),
             .EntryPointPath = fs::path (entryPath),
             .CSources       = std::move (csources) };
}

fs::path
BuildDriver::resolveImportPathToFilePath (const std::string &importPath) {
    std::string relPath;
    for (const auto &c : importPath) {
        relPath += c == '.' ? '/' : c;
    }
    auto res = _projectRoot / "src" / fs::path (relPath + ".veo");
    return std::move (res);
}

void
BuildDriver::scanDeps (const std::string &importPath) {
    if (_graph.contains (importPath)) {
        return;
    }

    auto path = resolveImportPathToFilePath (importPath);
    if (!fs::exists (path)) {
        return;
    }
    auto file = File{
        .Path  = path,
        .Deps  = std::move (resolveDeps (path.string ())),
        .State = VisitState::Unvisited,
    };
    _graph[importPath] = file;
    for (const auto &dep : file.Deps) {
        scanDeps (dep);
    }
}

void
BuildDriver::sortDeps (const std::string &importPath) {
    auto it = _graph.find (importPath);
    if (it == _graph.end ()) {
        return;
    }
    auto &file = it->second;

    if (file.State == VisitState::Visiting) {
        llvm::errs ().changeColor (llvm::raw_fd_ostream::RED)
            << "Cyclic dependency detected in " << llvm::raw_fd_ostream::RESET << "'"
            << file.Path.string () << "'\n";
        exit (1);
    }
    if (file.State == VisitState::Visited) {
        return;
    }

    file.State = VisitState::Visiting;
    for (const auto &dep : file.Deps) {
        sortDeps (dep);
    }
    file.State = VisitState::Visited;
    _compilationQueue.push_back (importPath);
}

std::vector<std::string>
BuildDriver::resolveDeps (const std::string &absolutePath) {
    const auto &path = fs::path (absolutePath);
    if (!fs::exists (path)) {
        return {};
    }
    std::ifstream     file (path);
    std::stringstream content;
    content << file.rdbuf ();
    const auto &contentStr = content.str ();
    if (contentStr.empty ()) {
        return {};
    }
    DepsResolver depsResolver (&contentStr.front (), &contentStr.back ());
    auto         deps = std::move (depsResolver.Deps ());
    file.close ();
    return std::move (deps);
}

bool
BuildDriver::isArtefactsFresh (
    const fs::path &srcPath, const fs::path &objPath, const fs::path &vmetaPath) {
    if (!fs::exists (objPath) || !fs::exists (vmetaPath)) {
        return false;
    }
    return fs::last_write_time (objPath) > fs::last_write_time (srcPath)
           && fs::last_write_time (vmetaPath) > fs::last_write_time (srcPath);
}

}
