#pragma once
#include <basic/symbols/module.h>
#include <basic/types/type.h>
#include <cstdint>
#include <filesystem>
#include <llvm/Bitstream/BitstreamWriter.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace veo::bitcode {

using StringID = uint32_t;
using TypeID   = uint32_t;
using ModuleID = uint32_t;
namespace fs   = std::filesystem;

class StringPool {
    std::vector<std::string>                  _vals;
    std::unordered_map<std::string, StringID> _map;

public:
    StringID
    GetID (const std::string &val) {
        if (auto it = _map.find (val); it != _map.end ()) {
            return it->second;
        }
        StringID id = _vals.size ();
        _vals.push_back (val);
        _map.emplace (val, id);
        return id;
    }

    const std::vector<std::string> &
    Values () const {
        return _vals;
    }
};

class TypePool {
    std::vector<basic::Type *>                _vals;
    std::unordered_map<basic::Type *, TypeID> _map;

public:
    TypeID
    GetID (basic::Type *val) {
        if (auto it = _map.find (val); it != _map.end ()) {
            return it->second;
        }
        TypeID id = _vals.size ();
        _vals.push_back (val);
        _map.emplace (val, id);
        return id;
    }

    const std::vector<basic::Type *> &
    Values () const {
        return _vals;
    }
};

class ModulePool {
    std::vector<symbols::Module *>                  _vals;
    std::unordered_map<symbols::Module *, ModuleID> _map;

public:
    ModuleID
    GetID (symbols::Module *val) {
        if (auto it = _map.find (val); it != _map.end ()) {
            return it->second;
        }
        ModuleID id = _vals.size ();
        _vals.push_back (val);
        _map.emplace (val, id);
        return id;
    }

    const std::vector<symbols::Module *> &
    Values () const {
        return _vals;
    }
};

class Serializer {
    StringPool _strPool;
    TypePool   _typePool;
    ModulePool _modPool;

public:
    void
    SerializeModule (symbols::Module *mod, const fs::path &outputPath);

private:
    static void
    emitMagicNumber (llvm::BitstreamWriter &writer);

    void
    collectType (basic::Type *type);

    void
    collectStringsAndTypes (symbols::Module *mod);

    void
    collectStringsAndTypesInVars (symbols::Module *mod);

    void
    collectStringsAndTypesInFuncs (symbols::Module *mod);

    void
    collectStringsAndTypesInStructs (symbols::Module *mod);

    void
    collectStringsAndTypesInTraits (symbols::Module *mod);

    void
    collectStringsAndTypesInTrait (const symbols::Trait &trait);

    void
    collectStringsAndTypesInPrimitiveMethods (symbols::Module *mod);

    void
    collectStringsAndTypesInPrimitiveTraitsImplement (symbols::Module *mod);

    void
    serializeStringPool (llvm::BitstreamWriter &writer);

    void
    serializeTypePool (llvm::BitstreamWriter &writer);

    void
    serializeModulePool (llvm::BitstreamWriter &writer);

    void
    serializeModuleSymbols (llvm::BitstreamWriter &writer, symbols::Module *mod);

    void
    serializeFunction (llvm::BitstreamWriter &writer, const symbols::Function &func);

    void
    serializeMethod (llvm::BitstreamWriter &writer, const symbols::Method &method);
};

}
