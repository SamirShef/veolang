#pragma once
#include <basic/symbols/struct.h>
#include <hir/func.h>
#include <hir/mangle_kind.h>
#include <hir/struct_def.h>
#include <hir/var_def.h>
#include <string>

namespace veo {

class Mangler {
public:
    static std::string
    MangleFunction (
        const hir::Function *func, hir::MangleKind mangleKind = hir::MangleKind::Veo);

    static std::string
    MangleMethod (
        basic::Type    *base,
        hir::Function  *func,
        hir::MangleKind mangleKind = hir::MangleKind::Veo);

    static std::string
    MangleGlobalVar (
        const hir::VarDef *var, hir::MangleKind mangleKind = hir::MangleKind::Veo);

    static std::string
    MangleStaticField (const symbols::Struct *sym, const std::string &fieldName);

    static std::string
    MangleStruct (
        const hir::StructDef *sd, hir::MangleKind mangleKind = hir::MangleKind::Veo);

    static std::string
    MangleStructSymbol (
        const symbols::Struct *sym, hir::MangleKind mangleKind = hir::MangleKind::Veo);

    static std::string
    MangleModule (const symbols::Module *mod);

    static std::string
    MangleType (
        const basic::Type *type, hir::MangleKind mangleKind = hir::MangleKind::Veo);
};

}
