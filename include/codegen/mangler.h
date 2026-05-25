#pragma once
#include <basic/symbols/struct.h>
#include <hir/func.h>
#include <hir/struct_def.h>
#include <hir/var_def.h>
#include <string>

namespace veo {

class Mangler {
public:
    static std::string
    MangleFunction (hir::Function *func);

    static std::string
    MangleMethod (symbols::Struct *sym, hir::Function *func);

    static std::string
    MangleGlobalVar (hir::VarDef *var);

    static std::string
    MangleStaticField (symbols::Struct *sym, const std::string &fieldName);

    static std::string
    MangleStruct (hir::StructDef *sd);

    static std::string
    MangleStructSymbol (symbols::Struct *sym);

    static std::string
    MangleModule (symbols::Module *mod);

    static std::string
    MangleType (basic::Type *type);
};

}
