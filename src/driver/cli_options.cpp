#include <driver/build.h>
#include <driver/cli_options.h>
#include <filesystem>
#include <fstream>
#include <llvm/Support/raw_ostream.h>

namespace veo::driver {

void
ExecuteArguments () {
    if (NewSub) {
        CreateNewPackage (NewNameOpt);
    }
    if (InitSub) {
        fs::path curDir = fs::current_path ();
        InitNewPackage (curDir.stem ().string ());
    }
    if (BuildSub) {
        BuildPackage ();
    }
    if (TestSub) {
        TestPackage ();
    }
    if (CheckSub) {
        CheckPackage ();
    }
    if (FetchSub) {
        FetchRegistry ();
    }
    if (!ExplainOpt.empty ()) {
        Explain ();
    }
}

void
InitNewPackage (const std::string &name) {
    fs::create_directory ("src");
    if (fs::exists ("veo.toml")) {
        llvm::errs () << llvm::raw_fd_ostream::RED << "Package '" << name
                      << "' already initialized (contains veo.toml)\n"
                      << llvm::raw_fd_ostream::RESET;
        return;
    }
    std::ofstream veoToml ("veo.toml");
    veoToml << "[package]\n";
    veoToml << "name = \"" << name << "\"\n";
    veoToml << "entry = \"src/main.veo\"\n";
    veoToml.close ();

    if (!fs::exists ("src/main.veo")) {
        std::ofstream mainVeo ("src/main.veo");
        mainVeo << "func main() {\n";
        mainVeo << "    return 0;\n";
        mainVeo << "}\n";
        mainVeo.close ();
    }
}

void
CreateNewPackage (const std::string &name) {
    fs::path rootPath (name);
    if (fs::exists (rootPath)) {
        llvm::errs () << llvm::raw_fd_ostream::RED << "Directory '" << name
                      << "' already exists\n"
                      << llvm::raw_fd_ostream::RESET;
        return;
    }
    fs::create_directory (rootPath);
    fs::current_path (rootPath);
    InitNewPackage (name);
}

void
BuildPackage () {
    fs::path    projectRoot = BuildDriver::GetProjectRoot (fs::current_path ());
    BuildDriver driver (projectRoot);
    driver.Build ();
}

void
RunPackage () {
    PrintOptUnimplementedError ("run");
}

void
TestPackage () {
    PrintOptUnimplementedError ("test");
}

void
CheckPackage () {
    PrintOptUnimplementedError ("check");
}

void
FetchRegistry () {
    PrintOptUnimplementedError ("fetch");
}

void
EmitIR (llvm::Module *mod, const fs::path &output) {
    PrintOptUnimplementedError ("--emit-ir");
}

void
Explain () {
    PrintOptUnimplementedError ("--explain");
}

}
