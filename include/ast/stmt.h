#pragma once
#include <ast/access_modifier.h>
#include <ast/node.h>

namespace veo::ast {

class Stmt : public Node {
    AccessModifier _access;

public:
    Stmt (AccessModifier access, NodeKind kind, llvm::SMLoc start, llvm::SMLoc end)
        : _access (access), Node (kind, start, end) {}
    Stmt (NodeKind kind, llvm::SMLoc start, llvm::SMLoc end)
        : _access (AccessModifier::Priv), Node (kind, start, end) {}

    ast_classof_with_range (StmtStart, StmtEnd);

    AccessModifier
    Access () const {
        return _access;
    }
};

}
