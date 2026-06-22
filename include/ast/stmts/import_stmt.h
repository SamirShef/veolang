#pragma once
#include <ast/stmt.h>
#include <basic/name.h>
#include <vector>

namespace veo::ast {

class ImportStmt : public Stmt {
    std::vector<basic::NameObj> _path;

public:
    ImportStmt (std::vector<basic::NameObj> path, llvm::SMLoc start, llvm::SMLoc end)
        : _path (std::move (path)), Stmt (NodeKind::ImportStmt, start, end) {}

    ast_classof (ImportStmt);

    const std::vector<basic::NameObj> &
    Path () const {
        return _path;
    }
};

}
