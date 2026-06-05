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
    MangleFunction (const hir::Function *func);

    static std::string
    MangleMethod (basic::Type *base, hir::Function *func);

    static std::string
    MangleGlobalVar (const hir::VarDef *var);

    static std::string
    MangleStaticField (const symbols::Struct *sym, const std::string &fieldName);

    static std::string
    MangleStruct (const hir::StructDef *sd);

    static std::string
    MangleStructSymbol (const symbols::Struct *sym);

    static std::string
    MangleModule (const symbols::Module *mod);

    static std::string
    MangleType (const basic::Type *type);
};

}
