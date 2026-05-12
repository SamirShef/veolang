#pragma once
#include <cstdint>

namespace veo::ast {

enum class NodeKind : uint8_t {
    StmtStart,
    VarDef,
    FuncDef,
    Ret,
    ExprStmt,
    StmtEnd,

    ExprStart,
    LitExpr,
    BinExpr,
    UnExpr,
    VarExpr,
    ExprEnd
};

}
