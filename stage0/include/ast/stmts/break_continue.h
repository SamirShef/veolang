#pragma once
#include <ast/stmt.h>

namespace veo::ast {

class BreakContinue : public Stmt {
public:
    enum class Kind : uint8_t { Break, Continue };

private:
    Kind _kind;

public:
    BreakContinue (Kind kind, llvm::SMLoc start, llvm::SMLoc end)
        : _kind (kind), Stmt (NodeKind::BreakContinue, start, end) {}

    ast_classof (BreakContinue);

    Kind
    GetKind () const {
        return _kind;
    }
};

}
