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
    ForLoop,
    BreakContinue,
    StructDef,
    StmtEnd,

    ExprStart,
    LitExpr,
    BinExpr,
    UnExpr,
    VarExpr,
    FuncCall,
    AsgnExpr,
    FieldExpr,
    ExprEnd
};

}
