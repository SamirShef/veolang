#pragma once
#include <ast/stmt.h>
#include <ast/stmts/func_def.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::ast {

struct Method {
    FuncDef *Func;
    bool     IsStatic;

    Method (FuncDef *func, bool isStatic) : Func (func), IsStatic (isStatic) {}
};

class ImplStmt : public Stmt {
    basic::Type        *_structType;
    basic::Type        *_traitType;
    std::vector<Method> _methods;

public:
    ImplStmt (
        basic::Type        *structType,
        basic::Type        *traitType,
        std::vector<Method> methods,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _structType (structType),
          _traitType (traitType),
          _methods (std::move (methods)),
          Stmt (NodeKind::ImplStmt, start, end) {}

    ast_classof (ImplStmt);

    basic::Type *&
    StructType () {
        return _structType;
    }

    basic::Type *&
    TraitType () {
        return _traitType;
    }

    std::vector<Method> &
    Methods () {
        return _methods;
    }
};

}
