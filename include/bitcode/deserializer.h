#pragma once
#include <basic/symbols/module.h>
#include <basic/types/pool.h>
#include <filesystem>
#include <llvm/Bitstream/BitstreamReader.h>

namespace veo::bitcode {

namespace fs = std::filesystem;

class Deserializer {
    basic::TypePool &_globalTypePool; // NOLINT

    std::vector<std::string>       _stringPool;
    std::vector<basic::Type *>     _typePool;
    std::vector<symbols::Module *> _modulePool;

public:
    explicit Deserializer (basic::TypePool &globalTypePool)
        : _globalTypePool (globalTypePool) {}

    symbols::Module *
    DeserializeModule (const fs::path &inputPath);

private:
    static bool
    readMagicNumber (llvm::BitstreamCursor &cursor);

    void
    deserializeStringPool (llvm::BitstreamCursor &cursor);

    void
    deserializeTypePool (llvm::BitstreamCursor &cursor);

    void
    deserializeModulePool (llvm::BitstreamCursor &cursor);

    void
    deserializeModuleSymbols (llvm::BitstreamCursor &cursor, symbols::Module *mod);
};

}
