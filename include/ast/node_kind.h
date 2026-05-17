#pragma once
#include <cstdint>

namespace veo::ast {

enum class NodeKind : uint8_t {
    StmtStart,
    VarDef,
    FuncDef,
    Ret,
    ExprStmt,
    IfElse,
    StmtEnd,

    ExprStart,
    LitExpr,
    BinExpr,
    UnExpr,
    VarExpr,
    FuncCall,
    AsgnExpr,
    ExprEnd
};

}
