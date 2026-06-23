#pragma once
#include <ast/stmt.h>
#include <ast/stmts/func_def.h>
#include <ast/stmts/impl_stmt.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::ast {

class TraitStmt : public Stmt {
    basic::NameObj      _name;
    std::vector<Method> _methods;

public:
    TraitStmt (
        basic::NameObj      name,
        std::vector<Method> methods,
        AccessModifier      access,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _name (std::move (name)),
          _methods (std::move (methods)),
          Stmt (access, NodeKind::TraitStmt, start, end) {}

    ast_classof (TraitStmt);

    basic::NameObj
    Name () const {
        return _name;
    }

    std::vector<Method> &
    Methods () {
        return _methods;
    }
};

}
