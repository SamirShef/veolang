#include <basic/symbols/module.h>
#include <basic/types/all.h>
#include <codegen/mangler.h>
#include <sstream>

namespace veo {

std::string
Mangler::MangleFunction (const hir::Function *func, hir::MangleKind mangleKind) {
    auto *sym = func->BaseSymbol ();
    if (mangleKind == hir::MangleKind::C) {
        return sym->Name.Val;
    }
    std::ostringstream oss;
    oss << "_VF";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val << "I";
    for (auto &a : sym->Args) {
        oss << MangleType (a.Type);
    }
    oss << "E";
    return oss.str ();
}

std::string
Mangler::MangleMethod (
    basic::Type *base, hir::Function *func, hir::MangleKind mangleKind) {
    if (mangleKind == hir::MangleKind::C) {
        return func->Name ().Val;
    }
    std::ostringstream oss;
    oss << "_VM";
    if (base->IsStruct ()) {
        auto *sym = base->AsStruct ()->BaseSymbol ();
        oss << MangleModule (sym->Parent);
        oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    } else {
        const auto &name = base->ToString ();
        oss << "E" << name.size () << name;
    }
    oss << "E" << func->Name ().Val.size () << func->Name ().Val << "I";
    for (auto &a : func->Args ()) {
        oss << MangleType (a->Type ());
    }
    oss << "E";
    return oss.str ();
}

std::string
Mangler::MangleGlobalVar (const hir::VarDef *var, hir::MangleKind mangleKind) {
    auto *sym = var->BaseSymbol ();
    if (mangleKind == hir::MangleKind::C) {
        return sym->Name.Val;
    }
    std::ostringstream oss;
    oss << "_VG";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
Mangler::MangleStaticField (const symbols::Struct *sym, const std::string &fieldName) {
    std::ostringstream oss;
    oss << "_VF";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    oss << "E" << fieldName.size () << fieldName;
    return oss.str ();
}

std::string
Mangler::MangleStruct (const hir::StructDef *sd, hir::MangleKind mangleKind) {
    return MangleStructSymbol (sd->BaseSymbol ());
}

std::string
Mangler::MangleStructSymbol (const symbols::Struct *sym, hir::MangleKind mangleKind) {
    if (mangleKind == hir::MangleKind::C) {
        return sym->Name.Val;
    }
    std::ostringstream oss;
    oss << "_VS";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
Mangler::MangleModule (const symbols::Module *mod) {
    if (mod == nullptr) {
        return "";
    }
    std::ostringstream oss;
    oss << MangleModule (mod->Parent);
    oss << mod->Name.size () << mod->Name;
    return oss.str ();
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
std::string
Mangler::MangleType (const basic::Type *type, hir::MangleKind mangleKind) {
    switch (type->Kind ()) {
    case basic::TypeKind::Integer: {
        const auto *it = llvm::cast<basic::IntegerType> (type);
        switch (it->BitWidth ()) {
        case 8: return "b";
        case 16: return "s";
        case 32: return "i";
        case 64: return "l";
        default: return "";
        }
    }
    case basic::TypeKind::Floating:
        return llvm::cast<basic::FloatingType> (type)->IsFloat () ? "f" : "d";
    case basic::TypeKind::Bool: return "B";
    case basic::TypeKind::Char: return "c";
    case basic::TypeKind::Struct: {
        auto *sym = type->AsStruct ()->BaseSymbol ();
        return "S" + std::to_string (sym->Name.Val.size ()) + sym->Name.Val;
    }
    case basic::TypeKind::Size: return "z";
    case basic::TypeKind::Pointer: return "P" + MangleType (type->AsPointer ()->Base ());
    case basic::TypeKind::Noth: return "n";
    case basic::TypeKind::Alias: return MangleType (type->AsAlias ()->Base ());
    case basic::TypeKind::Trait:
    case basic::TypeKind::Named:
    case basic::TypeKind::TraitThis:
    case basic::TypeKind::Module: break;
    }
    return "";
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}
