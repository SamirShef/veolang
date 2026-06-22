#include "basic/types/alias.h"

#include <basic/types/bool.h>
#include <basic/types/char.h>
#include <basic/types/floating.h>
#include <basic/types/integer.h>
#include <basic/types/noth.h>
#include <basic/types/pointer.h>
#include <basic/types/size.h>
#include <basic/types/struct.h>
#include <basic/types/trait.h>
#include <basic/types/trait_this.h>
#include <bitcode/block_id.h>
#include <bitcode/deserializer.h>
#include <driver/module_loader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

namespace veo::bitcode {

// NOLINTBEGIN
symbols::Module *
Deserializer::DeserializeModule (const fs::path &inputPath) {
    auto bufferOrErr = llvm::MemoryBuffer::getFile (inputPath.string ());
    if (!bufferOrErr) {
        return nullptr;
    }

    llvm::BitstreamCursor cursor (bufferOrErr.get ()->getMemBufferRef ());

    if (!readMagicNumber (cursor)) {
        return nullptr;
    }

    while (!cursor.AtEndOfStream ()) {
        auto entry = cursor.advance ();
        if (entry->Kind != llvm::BitstreamEntry::SubBlock) {
            break;
        }

        switch (static_cast<BlockID> (entry->ID)) {
        case BlockID::StringPool: {
            cursor.EnterSubBlock (entry->ID);
            deserializeStringPool (cursor);
            auto next = cursor.advance ();
            if (next->Kind != llvm::BitstreamEntry::EndBlock) {
                return nullptr;
            }
            break;
        }
        case BlockID::TypePool: {
            cursor.EnterSubBlock (entry->ID);
            deserializeTypePool (cursor);
            auto next = cursor.advance ();
            if (next->Kind != llvm::BitstreamEntry::EndBlock) {
                return nullptr;
            }
            break;
        }
        case BlockID::ModulePool: {
            cursor.EnterSubBlock (entry->ID);
            deserializeModulePool (cursor);
            auto next = cursor.advance ();
            if (next->Kind != llvm::BitstreamEntry::EndBlock) {
                return nullptr;
            }
            break;
        }
        case BlockID::Symbols: {
            cursor.EnterSubBlock (entry->ID);
            if (_modulePool.empty ()) {
                return nullptr;
            }
            symbols::Module *mod = _modulePool[cursor.Read (32).get ()];
            deserializeModuleSymbols (cursor, mod);
            auto next = cursor.advance ();
            if (next->Kind != llvm::BitstreamEntry::EndBlock) {
                return nullptr;
            }

            for (const auto &fixup : _structFixups) {
                if (fixup.ParentModuleID != 0xFFFFFFFF
                    && fixup.ParentModuleID < _modulePool.size ()) {
                    auto       *parentMod = _modulePool[fixup.ParentModuleID];
                    const auto &name      = _stringPool[fixup.NameID];

                    auto it = parentMod->Structs.find (name);
                    if (it != parentMod->Structs.end ()) {
                        fixup.Type->_base = &it->second;
                    }
                }
            }

            for (const auto &fixup : _traitFixups) {
                if (fixup.ParentModuleID != 0xFFFFFFFF
                    && fixup.ParentModuleID < _modulePool.size ()) {
                    auto       *parentMod = _modulePool[fixup.ParentModuleID];
                    const auto &name      = _stringPool[fixup.NameID];

                    auto it = parentMod->Traits.find (name);
                    if (it != parentMod->Traits.end ()) {
                        fixup.Type->_base = &it->second;
                    }
                }
            }

            for (const auto &fixup : _traitThisFixups) {
                if (fixup.ParentModuleID != 0xFFFFFFFF
                    && fixup.ParentModuleID < _modulePool.size ()) {
                    auto       *parentMod = _modulePool[fixup.ParentModuleID];
                    const auto &name      = _stringPool[fixup.NameID];

                    auto it = parentMod->Traits.find (name);
                    if (it != parentMod->Traits.end ()) {
                        fixup.Type->_trait = &it->second;
                    }
                }
            }

            return mod;
        }
        default: cursor.SkipBlock (); break;
        }
    }

    return _modulePool.empty () ? nullptr : _modulePool[0];
}
// NOLINTEND

bool
Deserializer::readMagicNumber (llvm::BitstreamCursor &cursor) {
    return cursor.Read (8).get () == 'V' && cursor.Read (8).get () == 'E'
           && cursor.Read (8).get () == 'O';
}

void
Deserializer::deserializeStringPool (llvm::BitstreamCursor &cursor) {
    uint32_t size = cursor.Read (32).get ();
    _stringPool.resize (size);
    for (uint32_t i = 0; i < size; ++i) {
        uint32_t    len = cursor.Read (32).get ();
        std::string str;
        str.reserve (len);
        for (uint32_t j = 0; j < len; ++j) {
            str.push_back (static_cast<char> (cursor.Read (8).get ()));
        }
        _stringPool[i] = std::move (str);
    }
}

void
Deserializer::deserializeTypePool (llvm::BitstreamCursor &cursor) {
    uint32_t size = cursor.Read (32).get ();
    _typePool.resize (size);

    struct PointerFixup {
        basic::PointerType *PointerType;
        uint32_t            BaseTypeID;
    };
    std::vector<PointerFixup> pointerFixups;

    struct AliasFixup {
        basic::AliasType *AliasType;
        uint32_t          BaseTypeID;
    };
    std::vector<AliasFixup> aliasFixups;

    for (uint32_t i = 0; i < size; ++i) {
        auto         kind = static_cast<basic::TypeKind> (cursor.Read (8).get ());
        basic::Type *type = nullptr;

        switch (kind) {
        case basic::TypeKind::Integer: {
            uint32_t width      = cursor.Read (32).get ();
            bool     isUnsigned = cursor.Read (1).get () != 0;
            type = _globalTypePool.GetOrCreate<basic::IntegerType> (width, isUnsigned);
            break;
        }
        case basic::TypeKind::Floating: {
            uint32_t floatingKind = cursor.Read (8).get ();
            type                  = _globalTypePool.GetOrCreate<basic::FloatingType> (
                static_cast<basic::FloatingKind> (floatingKind));
            break;
        }
        case basic::TypeKind::Size: {
            bool isUnsigned = cursor.Read (1).get () != 0;
            type            = _globalTypePool.GetOrCreate<basic::SizeType> (isUnsigned);
            break;
        }
        case basic::TypeKind::Pointer: {
            uint32_t baseID = cursor.Read (32).get ();
            type            = _globalTypePool.GetOrCreate<basic::PointerType> (nullptr);
            pointerFixups.emplace_back (llvm::cast<basic::PointerType> (type), baseID);
            break;
        }
        case basic::TypeKind::Bool: {
            type = _globalTypePool.GetOrCreate<basic::BoolType> ();
            break;
        }
        case basic::TypeKind::Char: {
            type = _globalTypePool.GetOrCreate<basic::CharType> ();
            break;
        }
        case basic::TypeKind::Noth: {
            type = _globalTypePool.GetOrCreate<basic::NothType> ();
            break;
        }
        case basic::TypeKind::Struct: {
            uint32_t nameID   = cursor.Read (32).get ();
            uint32_t parentID = cursor.Read (32).get ();
            type              = _globalTypePool.GetOrCreate<basic::StructType> (nullptr);
            _structFixups.push_back (
                { .Type           = llvm::cast<basic::StructType> (type),
                  .NameID         = nameID,
                  .ParentModuleID = parentID });
            break;
        }
        case basic::TypeKind::Trait: {
            uint32_t nameID   = cursor.Read (32).get ();
            uint32_t parentID = cursor.Read (32).get ();
            type              = _globalTypePool.GetOrCreate<basic::TraitType> (nullptr);
            _traitFixups.push_back (
                { .Type           = llvm::cast<basic::TraitType> (type),
                  .NameID         = nameID,
                  .ParentModuleID = parentID });
            break;
        }
        case basic::TypeKind::TraitThis: {
            uint32_t nameID   = cursor.Read (32).get ();
            uint32_t parentID = cursor.Read (32).get ();
            type = _globalTypePool.GetOrCreate<basic::TraitThisType> (nullptr);
            _traitThisFixups.push_back (
                { .Type           = llvm::cast<basic::TraitThisType> (type),
                  .NameID         = nameID,
                  .ParentModuleID = parentID });
            break;
        }
        case basic::TypeKind::Alias: {
            uint32_t nameID = cursor.Read (32).get ();
            uint32_t baseID = cursor.Read (32).get ();
            type            = _globalTypePool.GetOrCreate<basic::AliasType> (
                basic::NameObj (_stringPool[nameID], {}, {}),
                nullptr);
            aliasFixups.emplace_back (llvm::cast<basic::AliasType> (type), baseID);
            break;
        }
        }
        _typePool[i] = type;
    }

    for (const auto &fixup : pointerFixups) {
        fixup.PointerType->_base = _typePool[fixup.BaseTypeID];
    }
    for (const auto &fixup : aliasFixups) {
        fixup.AliasType->_base = _typePool[fixup.BaseTypeID];
    }
}

void
Deserializer::deserializeModulePool (llvm::BitstreamCursor &cursor) {
    uint32_t size = cursor.Read (32).get ();
    _modulePool.resize (size);

    struct TempModuleData {
        std::string Name;
        uint32_t    ParentID{};
    };
    std::vector<TempModuleData> tempMods (size);

    for (uint32_t i = 0; i < size; ++i) {
        uint32_t nameID   = cursor.Read (32).get ();
        uint32_t parentID = cursor.Read (32).get ();

        const auto &modName = _stringPool[nameID];
        tempMods[i]         = { .Name = modName, .ParentID = parentID };

        auto *mod = driver::ModuleLoader::LoadModule (modName);
        if (mod == nullptr) {
            mod = new symbols::Module (modName); // NOLINT
            driver::ModuleLoader::AddModule (modName, mod);
        }
        _modulePool[i] = mod;
    }

    for (uint32_t i = 0; i < size; ++i) {
        uint32_t parentID = tempMods[i].ParentID;
        if (parentID != 0xFFFFFFFF) {
            auto *parent                      = _modulePool[parentID];
            _modulePool[i]->Parent            = parent;
            parent->Submods[tempMods[i].Name] = _modulePool[i];
        }
    }
}

void
Deserializer::deserializeModuleSymbols (
    llvm::BitstreamCursor &cursor, symbols::Module *mod) {
    uint32_t numVars = cursor.Read (32).get ();
    mod->Vars.reserve (numVars);
    for (uint32_t i = 0; i < numVars; ++i) {
        uint32_t nameID     = cursor.Read (32).get ();
        uint32_t typeID     = cursor.Read (32).get ();
        bool     isConst    = cursor.Read (1).get () == 1;
        bool     isGlobal   = cursor.Read (1).get () == 1;
        auto     access     = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
        auto     mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());
        auto     parentID   = cursor.Read (32).get ();

        basic::NameObj nameObj;
        nameObj.Val = _stringPool[nameID];

        symbols::Variable var (
            nameObj,
            _typePool[typeID],
            isConst,
            isGlobal,
            parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
            access,
            mangleKind);
        mod->Vars.emplace (nameObj.Val, std::move (var));
    }

    uint32_t numFuncs = cursor.Read (32).get ();
    for (uint32_t i = 0; i < numFuncs; ++i) {
        uint32_t       nameID = cursor.Read (32).get ();
        basic::NameObj nameObj;
        nameObj.Val = _stringPool[nameID];

        uint32_t numCandidates = cursor.Read (32).get ();
        mod->Funcs[nameObj.Val].Candidates.reserve (numCandidates);
        for (uint32_t j = 0; j < numCandidates; ++j) {
            uint32_t retTypeID = cursor.Read (32).get ();
            bool     isGeneric = cursor.Read (1).get () == 1;
            uint32_t parentID  = cursor.Read (32).get ();
            auto     access = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
            auto     mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());

            symbols::Function func (
                nameObj,
                _typePool[retTypeID],
                {},
                isGeneric,
                parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
                access,
                mangleKind);

            uint32_t numArgs = cursor.Read (32).get ();
            func.Args.reserve (numArgs);
            for (uint32_t j = 0; j < numArgs; ++j) {
                const auto &name = _stringPool[cursor.Read (32).get ()];
                auto       *type = _typePool[cursor.Read (32).get ()];
                func.Args.emplace_back (basic::NameObj (name, {}, {}), type);
            }

            mod->Funcs[nameObj.Val].Candidates.push_back (
                std::make_unique<symbols::Function> (std::move (func)));
        }
    }

    uint32_t numStructs = cursor.Read (32).get ();
    for (uint32_t i = 0; i < numStructs; ++i) {
        uint32_t nameID     = cursor.Read (32).get ();
        auto     access     = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
        auto     mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());
        bool     isComplete = cursor.Read (1).get () == 1;
        uint32_t parentID   = cursor.Read (32).get ();

        uint32_t                    numFields = cursor.Read (32).get ();
        std::vector<symbols::Field> fields;
        fields.reserve (numFields);
        for (uint32_t j = 0; j < numFields; ++j) {
            uint32_t nameID   = cursor.Read (32).get ();
            uint32_t typeID   = cursor.Read (32).get ();
            bool     isStatic = cursor.Read (1).get () == 1;
            bool     isConst  = cursor.Read (1).get () == 1;
            auto     access   = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
            uint32_t index    = cursor.Read (32).get ();
            fields.emplace_back (
                basic::NameObj (_stringPool[nameID], {}, {}),
                _typePool[typeID],
                isStatic,
                isConst,
                access,
                index);
        }

        uint32_t numMethods = cursor.Read (32).get ();
        std::unordered_map<std::string, symbols::MethodCandidates> methods;
        methods.reserve (numMethods);
        for (uint32_t j = 0; j < numMethods; ++j) {
            uint32_t methodNameID  = cursor.Read (32).get ();
            auto     methodNameObj = basic::NameObj (_stringPool[methodNameID], {}, {});
            uint32_t numCandidates = cursor.Read (32).get ();
            methods[methodNameObj.Val].Candidates.reserve (numCandidates);
            for (uint32_t k = 0; k < numCandidates; ++k) {
                auto access   = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                bool isStatic = cursor.Read (1).get () == 1;
                bool isGeneric = cursor.Read (1).get () == 1;
                bool isConst   = cursor.Read (1).get () == 1;

                uint32_t retTypeID       = cursor.Read (32).get ();
                bool     isGenericUnused = cursor.Read (1).get () == 1;
                uint32_t parentID        = cursor.Read (32).get ();
                auto     accessUnused
                    = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                auto mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());

                symbols::Function func (
                    methodNameObj,
                    _typePool[retTypeID],
                    {},
                    isGenericUnused,
                    parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
                    accessUnused,
                    mangleKind);

                uint32_t numArgs = cursor.Read (32).get ();
                func.Args.reserve (numArgs);
                for (uint32_t j = 0; j < numArgs; ++j) {
                    const auto &name = _stringPool[cursor.Read (32).get ()];
                    auto       *type = _typePool[cursor.Read (32).get ()];
                    func.Args.emplace_back (basic::NameObj (name, {}, {}), type);
                }

                auto funcPtr = std::make_unique<symbols::Function> (std::move (func));

                methods[methodNameObj.Val].Candidates.push_back (
                    std::make_unique<symbols::Method> (
                        std::move (funcPtr),
                        access,
                        isStatic,
                        isGeneric));
            }
        }

        auto            nameObj = basic::NameObj (_stringPool[nameID], {}, {});
        symbols::Struct s (
            nameObj,
            std::move (fields),
            parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
            access,
            mangleKind);
        s.Methods = std::move (methods);
        mod->Structs.emplace (nameObj.Val, std::move (s));
    }

    uint32_t numTraits = cursor.Read (32).get ();
    mod->Traits.reserve (numTraits);
    for (uint32_t i = 0; i < numTraits; ++i) {
        uint32_t nameID   = cursor.Read (32).get ();
        uint32_t parentID = cursor.Read (32).get ();
        auto     access   = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
        auto     nameObj  = basic::NameObj (_stringPool[nameID], {}, {});

        uint32_t numMethods = cursor.Read (32).get ();
        std::unordered_map<std::string, symbols::MethodCandidates> methods;
        methods.reserve (numMethods);
        for (uint32_t j = 0; j < numMethods; ++j) {
            uint32_t methodNameID  = cursor.Read (32).get ();
            auto     methodNameObj = basic::NameObj (_stringPool[methodNameID], {}, {});
            uint32_t numCandidates = cursor.Read (32).get ();
            methods[methodNameObj.Val].Candidates.reserve (numCandidates);
            for (uint32_t k = 0; k < numCandidates; ++k) {
                auto access   = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                bool isStatic = cursor.Read (1).get () == 1;
                bool isGeneric = cursor.Read (1).get () == 1;
                bool isConst   = cursor.Read (1).get () == 1;

                uint32_t retTypeID       = cursor.Read (32).get ();
                bool     isGenericUnused = cursor.Read (1).get () == 1;
                uint32_t parentID        = cursor.Read (32).get ();
                auto     accessUnused
                    = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                auto mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());

                symbols::Function func (
                    methodNameObj,
                    _typePool[retTypeID],
                    {},
                    isGeneric,
                    parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
                    accessUnused,
                    mangleKind);

                uint32_t numArgs = cursor.Read (32).get ();
                func.Args.reserve (numArgs);
                for (uint32_t j = 0; j < numArgs; ++j) {
                    const auto &name = _stringPool[cursor.Read (32).get ()];
                    auto       *type = _typePool[cursor.Read (32).get ()];
                    func.Args.emplace_back (basic::NameObj (name, {}, {}), type);
                }

                auto funcPtr = std::make_unique<symbols::Function> (std::move (func));

                methods[methodNameObj.Val].Candidates.push_back (
                    std::make_unique<symbols::Method> (
                        std::move (funcPtr),
                        access,
                        isStatic,
                        isGeneric));
            }
        }
        symbols::Trait trait (
            std::move (nameObj),
            parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
            access);
        trait.Methods = std::move (methods);
        mod->Traits.emplace (trait.Name.Val, std::move (trait));
    }

    uint32_t numPrimitiveMethods = cursor.Read (32).get ();
    mod->PrimitiveMethods.reserve (numPrimitiveMethods);
    for (uint32_t i = 0; i < numPrimitiveMethods; ++i) {
        uint32_t typeID = cursor.Read (32).get ();
        auto    *type   = _typePool[typeID];

        uint32_t numMethods = cursor.Read (32).get ();
        std::unordered_map<std::string, symbols::MethodCandidates> methods;
        methods.reserve (numMethods);
        for (uint32_t j = 0; j < numMethods; ++j) {
            uint32_t methodNameID  = cursor.Read (32).get ();
            auto     methodNameObj = basic::NameObj (_stringPool[methodNameID], {}, {});
            uint32_t numCandidates = cursor.Read (32).get ();
            methods[methodNameObj.Val].Candidates.reserve (numCandidates);
            for (uint32_t k = 0; k < numCandidates; ++k) {
                auto access   = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                bool isStatic = cursor.Read (1).get () == 1;
                bool isGeneric = cursor.Read (1).get () == 1;
                bool isConst   = cursor.Read (1).get () == 1;

                uint32_t retTypeID       = cursor.Read (32).get ();
                bool     isGenericUnused = cursor.Read (1).get () == 1;
                uint32_t parentID        = cursor.Read (32).get ();
                auto     accessUnused
                    = static_cast<ast::AccessModifier> (cursor.Read (8).get ());
                auto mangleKind = static_cast<hir::MangleKind> (cursor.Read (8).get ());

                symbols::Function func (
                    methodNameObj,
                    _typePool[retTypeID],
                    {},
                    isGeneric,
                    parentID != 0xFFFFFFFF ? _modulePool[parentID] : mod,
                    accessUnused,
                    mangleKind);

                uint32_t numArgs = cursor.Read (32).get ();
                func.Args.reserve (numArgs);
                for (uint32_t j = 0; j < numArgs; ++j) {
                    const auto &name = _stringPool[cursor.Read (32).get ()];
                    auto       *type = _typePool[cursor.Read (32).get ()];
                    func.Args.emplace_back (basic::NameObj (name, {}, {}), type);
                }

                auto funcPtr = std::make_unique<symbols::Function> (std::move (func));

                methods[methodNameObj.Val].Candidates.push_back (
                    std::make_unique<symbols::Method> (
                        std::move (funcPtr),
                        access,
                        isStatic,
                        isGeneric));
            }
        }
        mod->PrimitiveMethods.emplace (type, std::move (methods));
    }

    uint32_t numPrimitiveTraitsImplement = cursor.Read (32).get ();
    mod->PrimitiveTraitsImplement.reserve (numPrimitiveTraitsImplement);
    for (uint32_t i = 0; i < numPrimitiveTraitsImplement; ++i) {
        uint32_t typeID    = cursor.Read (32).get ();
        uint32_t numTraits = cursor.Read (32).get ();
        auto    *type      = _typePool[typeID];
        for (uint32_t j = 0; j < numTraits; ++j) {
            uint32_t        nameID   = cursor.Read (32).get ();
            uint32_t        parentID = cursor.Read (32).get ();
            const auto     &name     = _stringPool[nameID];
            symbols::Trait *trait    = nullptr;
            for (auto &[name, t] : mod->Traits) {
                if (t.Name.Val == name) {
                    trait = &t;
                    break;
                }
            }
            mod->PrimitiveTraitsImplement[type].push_back (trait);
        }
    }
}

}
