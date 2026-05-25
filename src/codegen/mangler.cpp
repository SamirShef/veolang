#include <basic/symbols/module.h>
#include <basic/types/all.h>
#include <codegen/mangler.h>
#include <sstream>

namespace veo {

std::string
Mangler::MangleFunction (hir::Function *func) {
    auto              *sym = func->BaseSymbol ();
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
Mangler::MangleMethod (symbols::Struct *sym, hir::Function *func) {

    std::ostringstream oss;
    oss << "_VM";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    oss << "E" << func->Name ().Val.size () << func->Name ().Val << "I";
    for (auto &a : func->Args ()) {
        oss << MangleType (a.Type);
    }
    oss << "E";
    return oss.str ();
}

std::string
Mangler::MangleGlobalVar (hir::VarDef *var) {

    auto              *sym = var->BaseSymbol ();
    std::ostringstream oss;
    oss << "_VG";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
Mangler::MangleStaticField (symbols::Struct *sym, const std::string &fieldName) {
    std::ostringstream oss;
    oss << "_VF";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    oss << "E" << fieldName.size () << fieldName;
    return oss.str ();
}

std::string
Mangler::MangleStruct (hir::StructDef *sd) {

    return MangleStructSymbol (sd->BaseSymbol ());
}

std::string
Mangler::MangleStructSymbol (symbols::Struct *sym) {

    std::ostringstream oss;
    oss << "_VS";
    oss << MangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
Mangler::MangleModule (symbols::Module *mod) {
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
Mangler::MangleType (basic::Type *type) {
    switch (type->Kind ()) {
    case basic::TypeKind::Integer: {
        auto *it = llvm::cast<basic::IntegerType> (type);
        switch (it->BitWidth ()) {
        case 16: return "s";
        case 32: return "i";
        case 64: return "l";
        default: return "";
        }
    }
    case basic::TypeKind::Floating:
        return llvm::cast<basic::FloatingType> (type)->IsFloat () ? "f" : "d";
    case basic::TypeKind::Bool: return "b";
    case basic::TypeKind::Char: return "c";
    case basic::TypeKind::Struct: {
        auto *sym = type->AsStruct ()->BaseSymbol ();
        return "S" + std::to_string (sym->Name.Val.size ()) + sym->Name.Val;
    }
    default: {
    }
    }
    return "";
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}
