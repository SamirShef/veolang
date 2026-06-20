#include <basic/types/floating.h>
#include <basic/types/integer.h>
#include <basic/types/pointer.h>
#include <basic/types/size.h>
#include <basic/types/struct.h>
#include <basic/types/trait.h>
#include <bitcode/block_id.h>
#include <bitcode/serializer.h>
#include <llvm/Support/FileSystem.h>

namespace veo::bitcode {

#define cast(expr, type) ((type) (expr))

void
Serializer::SerializeModule (symbols::Module *mod, const fs::path &outputPath) {
    std::error_code       ec;
    llvm::raw_fd_ostream  os (outputPath.string (), ec, llvm::sys::fs::OF_None);
    llvm::BitstreamWriter writer (os);

    emitMagicNumber (writer);

    _modPool.GetID (mod);

    collectStringsAndTypes (mod);

    serializeStringPool (writer);
    serializeTypePool (writer);
    serializeModulePool (writer);

    serializeModuleSymbols (writer, mod);
}

void
Serializer::emitMagicNumber (llvm::BitstreamWriter &writer) {
    writer.Emit ('V', 8);
    writer.Emit ('E', 8);
    writer.Emit ('O', 8);
}

void
Serializer::collectType (basic::Type *type) {
    if (type == nullptr) {
        return;
    }

    _typePool.GetID (type);

    if (type->IsPointer ()) {
        auto *ptrType = cast (type, basic::PointerType *);
        collectType (ptrType->Base ());
    } else if (type->IsStruct ()) {
        auto *strType = cast (type, basic::StructType *);
        if (auto *sym = strType->BaseSymbol ()) {
            _strPool.GetID (sym->Name.Val);
            if (sym->Parent != nullptr) {
                _modPool.GetID (sym->Parent);
            }
        }
    } else if (type->IsTrait ()) {
        auto *traitType = cast (type, basic::TraitType *);
        if (auto *sym = traitType->BaseSymbol ()) {
            _strPool.GetID (sym->Name.Val);
            if (sym->Parent != nullptr) {
                _modPool.GetID (sym->Parent);
            }
        }
    }
}

void
Serializer::collectStringsAndTypes (symbols::Module *mod) {
    if (mod == nullptr) {
        return;
    }

    _strPool.GetID (mod->Name);
    collectStringsAndTypesInVars (mod);
    collectStringsAndTypesInFuncs (mod);
    collectStringsAndTypesInStructs (mod);
    collectStringsAndTypesInTraits (mod);
    collectStringsAndTypesInPrimitiveMethods (mod);
    collectStringsAndTypesInPrimitiveTraitsImplement (mod);

    for (auto &[name, impMod] : mod->Imports) {
        _strPool.GetID (name);
        if (impMod != nullptr) {
            _modPool.GetID (impMod);
        }
        collectStringsAndTypes (impMod);
    }
    for (auto &[name, subMod] : mod->Submods) {
        _strPool.GetID (name);
        if (subMod != nullptr) {
            _modPool.GetID (subMod);
        }
        collectStringsAndTypes (subMod);
    }

    collectStringsAndTypes (mod->Parent);
}

void
Serializer::collectStringsAndTypesInVars (symbols::Module *mod) {
    for (auto &[name, var] : mod->Vars) {
        _strPool.GetID (name);
        collectType (var.Type);
    }
}

void
Serializer::collectStringsAndTypesInFuncs (symbols::Module *mod) {
    for (auto &[name, candidates] : mod->Funcs) {
        _strPool.GetID (name);
        for (auto &func : candidates.Candidates) {
            collectType (func->RetType);
            for (auto &arg : func->Args) {
                _strPool.GetID (arg.Name.Val);
                collectType (arg.Type);
            }
        }
    }
}

void
Serializer::collectStringsAndTypesInStructs (symbols::Module *mod) {
    for (auto &[name, s] : mod->Structs) {
        _strPool.GetID (name);
        for (auto &field : s.Fields) {
            _strPool.GetID (field.Name.Val);
            collectType (field.Type);
        }
        for (auto &[name, candidates] : s.Methods) {
            _strPool.GetID (name);
            for (auto &method : candidates.Candidates) {
                collectType (method->Func->RetType);
                for (auto &arg : method->Func->Args) {
                    _strPool.GetID (arg.Name.Val);
                    collectType (arg.Type);
                }
            }
        }
    }
}

void
Serializer::collectStringsAndTypesInTraits (symbols::Module *mod) {
    for (auto &[_, trait] : mod->Traits) {
        collectStringsAndTypesInTrait (trait);
    }
}

void
Serializer::collectStringsAndTypesInTrait (const symbols::Trait &trait) {
    _strPool.GetID (trait.Name.Val);
    for (const auto &[name, candidates] : trait.Methods) {
        _strPool.GetID (name);
        for (const auto &method : candidates.Candidates) {
            collectType (method->Func->RetType);
            for (auto &arg : method->Func->Args) {
                _strPool.GetID (arg.Name.Val);
                collectType (arg.Type);
            }
        }
    }
}

void
Serializer::collectStringsAndTypesInPrimitiveMethods (symbols::Module *mod) {
    for (auto &[type, methods] : mod->PrimitiveMethods) {
        collectType (type);
        for (auto &[name, candidates] : methods) {
            _strPool.GetID (name);
            for (auto &method : candidates.Candidates) {
                collectType (method->Func->RetType);
                for (auto &arg : method->Func->Args) {
                    _strPool.GetID (arg.Name.Val);
                    collectType (arg.Type);
                }
            }
        }
    }
}

void
Serializer::collectStringsAndTypesInPrimitiveTraitsImplement (symbols::Module *mod) {
    for (auto &[type, traits] : mod->PrimitiveTraitsImplement) {
        collectType (type);
        for (auto &trait : traits) {
            collectStringsAndTypesInTrait (*trait);
        }
    }
}

void
Serializer::serializeStringPool (llvm::BitstreamWriter &writer) {
    writer.EnterSubblock (cast (BlockID::StringPool, int), 3);
    uint32_t size = _strPool.Values ().size ();
    writer.Emit (size, 32);
    for (const auto &s : _strPool.Values ()) {
        writer.Emit (cast (s.size (), uint32_t), 32);
        for (char c : s) {
            writer.Emit (c, 8);
        }
    }
    writer.ExitBlock ();
}

void
Serializer::serializeTypePool (llvm::BitstreamWriter &writer) {
    writer.EnterSubblock (cast (BlockID::TypePool, unsigned), 3);

    uint32_t size = _typePool.Values ().size ();
    writer.Emit (size, 32);

    for (auto *type : _typePool.Values ()) {
        writer.Emit (cast (type->Kind (), uint32_t), 8);

        switch (type->Kind ()) {
        case basic::TypeKind::Integer: {
            auto *it = cast (type, basic::IntegerType *);
            writer.Emit (it->BitWidth (), 32);
            writer.Emit (cast (it->IsUnsigned (), uint32_t), 1);
            break;
        }
        case basic::TypeKind::Size: {
            auto *st = cast (type, basic::SizeType *);
            writer.Emit (cast (st->IsUnsigned (), uint32_t), 1);
            break;
        }
        case basic::TypeKind::Floating: {
            auto *ft = cast (type, basic::FloatingType *);
            writer.Emit (cast (ft->GetFloatingKind (), uint32_t), 8);
            break;
        }
        case basic::TypeKind::Pointer: {
            auto *pt = cast (type, basic::PointerType *);
            writer.Emit (_typePool.GetID (pt->Base ()), 32);
            break;
        }
        case basic::TypeKind::Struct: {
            auto *st   = cast (type, basic::StructType *);
            auto *sSym = st->BaseSymbol ();
            writer.Emit (_strPool.GetID (sSym->Name.Val), 32);
            writer.Emit (
                sSym->Parent != nullptr ? _modPool.GetID (sSym->Parent) : 0xFFFFFFFF,
                32);
            break;
        }
        case basic::TypeKind::Trait: {
            auto *tt   = cast (type, basic::TraitType *);
            auto *tSym = tt->BaseSymbol ();
            writer.Emit (_strPool.GetID (tSym->Name.Val), 32);
            writer.Emit (
                tSym->Parent != nullptr ? _modPool.GetID (tSym->Parent) : 0xFFFFFFFF,
                32);
            break;
        }
        default: break;
        }
    }

    writer.ExitBlock ();
}

void
Serializer::serializeModulePool (llvm::BitstreamWriter &writer) {
    writer.EnterSubblock (cast (BlockID::ModulePool, unsigned), 3);

    uint32_t size = _modPool.Values ().size ();
    writer.Emit (size, 32);

    for (auto *mod : _modPool.Values ()) {
        writer.Emit (_strPool.GetID (mod->Name), 32);
        writer.Emit (
            mod->Parent != nullptr ? _modPool.GetID (mod->Parent) : 0xFFFFFFFF,
            32);
    }

    writer.ExitBlock ();
}

void
Serializer::serializeModuleSymbols (llvm::BitstreamWriter &writer, symbols::Module *mod) {
    writer.EnterSubblock (cast (BlockID::Symbols, unsigned), 3);

    writer.Emit (_modPool.GetID (mod), 32);

    writer.Emit (cast (mod->Vars.size (), uint32_t), 32);
    for (auto &[name, var] : mod->Vars) {
        writer.Emit (_strPool.GetID (name), 32);
        writer.Emit (_typePool.GetID (var.Type), 32);
        writer.Emit (cast (var.IsConst, uint32_t), 1);
        writer.Emit (cast (var.IsGlobal, uint32_t), 1);
        writer.Emit (cast (var.MangleKind, uint32_t), 8);
        writer.Emit (
            var.Parent != nullptr ? _modPool.GetID (var.Parent) : 0xFFFFFFFF,
            32);
    }

    writer.Emit (cast (mod->Funcs.size (), uint32_t), 32);
    for (auto &[name, candidates] : mod->Funcs) {
        writer.Emit (_strPool.GetID (name), 32);
        writer.Emit (cast (candidates.Candidates.size (), uint32_t), 32);
        for (auto &func : candidates.Candidates) {
            serializeFunction (writer, *func);
        }
    }

    writer.Emit (cast (mod->Structs.size (), uint32_t), 32);
    for (auto &[name, s] : mod->Structs) {
        writer.Emit (_strPool.GetID (name), 32);
        writer.Emit (cast (s.MangleKind, uint32_t), 8);
        writer.Emit (cast (s.IsComplete, uint32_t), 1);
        writer.Emit (s.Parent != nullptr ? _modPool.GetID (s.Parent) : 0xFFFFFFFF, 32);

        writer.Emit (cast (s.Fields.size (), uint32_t), 32);
        for (auto &field : s.Fields) {
            writer.Emit (_strPool.GetID (field.Name.Val), 32);
            writer.Emit (_typePool.GetID (field.Type), 32);
            writer.Emit (cast (field.IsStatic, uint32_t), 1);
            writer.Emit (cast (field.IsConst, uint32_t), 1);
            writer.Emit (cast (field.Access, uint32_t), 8);
            writer.Emit (cast (field.Index, uint32_t), 32);
        }

        writer.Emit (cast (s.Methods.size (), uint32_t), 32);
        for (auto &[mName, candidates] : s.Methods) {
            writer.Emit (_strPool.GetID (mName), 32);
            writer.Emit (cast (candidates.Candidates.size (), uint32_t), 32);
            for (auto &method : candidates.Candidates) {
                serializeMethod (writer, *method);
            }
        }
    }

    writer.Emit (cast (mod->Traits.size (), uint32_t), 32);
    for (auto &[name, trait] : mod->Traits) {
        writer.Emit (_strPool.GetID (trait.Name.Val), 32);
        writer.Emit (
            trait.Parent != nullptr ? _modPool.GetID (trait.Parent) : 0xFFFFFFFF,
            32);

        writer.Emit (cast (trait.Methods.size (), uint32_t), 32);
        for (auto &[mName, candidates] : trait.Methods) {
            writer.Emit (_strPool.GetID (mName), 32);
            writer.Emit (cast (candidates.Candidates.size (), uint32_t), 32);
            for (auto &method : candidates.Candidates) {
                serializeMethod (writer, *method);
            }
        }
    }

    writer.Emit (cast (mod->PrimitiveMethods.size (), uint32_t), 32);
    for (auto &[type, methods] : mod->PrimitiveMethods) {
        writer.Emit (_typePool.GetID (type), 32);
        writer.Emit (cast (methods.size (), uint32_t), 32);
        for (auto &[mName, candidates] : methods) {
            writer.Emit (_strPool.GetID (mName), 32);
            writer.Emit (cast (candidates.Candidates.size (), uint32_t), 32);
            for (auto &method : candidates.Candidates) {
                serializeMethod (writer, *method);
            }
        }
    }

    writer.Emit (cast (mod->PrimitiveTraitsImplement.size (), uint32_t), 32);
    for (auto &[type, traits] : mod->PrimitiveTraitsImplement) {
        writer.Emit (_typePool.GetID (type), 32);
        writer.Emit (cast (traits.size (), uint32_t), 32);
        for (auto *trait : traits) {
            writer.Emit (_strPool.GetID (trait->Name.Val), 32);
            writer.Emit (
                trait->Parent != nullptr ? _modPool.GetID (trait->Parent) : 0xFFFFFFFF,
                32);
        }
    }

    writer.Emit (cast (mod->Imports.size (), uint32_t), 32);
    for (auto &[name, impMod] : mod->Imports) {
        writer.Emit (_strPool.GetID (name), 32);
        writer.Emit (impMod != nullptr ? _modPool.GetID (impMod) : 0xFFFFFFFF, 32);
    }

    writer.Emit (cast (mod->Submods.size (), uint32_t), 32);
    for (auto &[name, subMod] : mod->Submods) {
        writer.Emit (_strPool.GetID (name), 32);
        writer.Emit (subMod != nullptr ? _modPool.GetID (subMod) : 0xFFFFFFFF, 32);
    }

    writer.ExitBlock ();
}

void
Serializer::serializeFunction (
    llvm::BitstreamWriter &writer, const symbols::Function &func) {
    writer.Emit (_typePool.GetID (func.RetType), 32);
    writer.Emit (cast (func.IsGeneric, uint32_t), 1);
    writer.Emit (func.Parent != nullptr ? _modPool.GetID (func.Parent) : 0xFFFFFFFF, 32);
    writer.Emit (cast (func.MangleKind, uint32_t), 8);

    writer.Emit (cast (func.Args.size (), uint32_t), 32);
    for (const auto &arg : func.Args) {
        writer.Emit (_strPool.GetID (arg.Name.Val), 32);
        writer.Emit (_typePool.GetID (arg.Type), 32);
    }
}

void
Serializer::serializeMethod (
    llvm::BitstreamWriter &writer, const symbols::Method &method) {
    writer.Emit (cast (method.Access, uint32_t), 8);
    writer.Emit (cast (method.IsStatic, uint32_t), 1);
    writer.Emit (cast (method.IsGeneric, uint32_t), 1);
    writer.Emit (cast (method.IsConst, uint32_t), 1);

    serializeFunction (writer, *method.Func);
}

#undef cast

}
