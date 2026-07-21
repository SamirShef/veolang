#pragma once
#include <ast/stmt.h>
#include <basic/name.h>
#include <vector>

namespace veo::ast {

class ExternStmt : public Stmt {
    basic::NameObj      _from;
    std::vector<Stmt *> _body;

public:
    ExternStmt (
        basic::NameObj from, std::vector<Stmt *> body, llvm::SMLoc start, llvm::SMLoc end)
        : _from (std::move (from)),
          _body (std::move (body)),
          Stmt (NodeKind::ExternStmt, start, end) {}

    ast_classof (ExternStmt);

    const basic::NameObj &
    From () const {
        return _from;
    }

    std::vector<Stmt *> &
    Body () {
        return _body;
    }
};

}
