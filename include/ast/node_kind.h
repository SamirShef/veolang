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
    ImplStmt,
    TraitStmt,
    ExternStmt,
    ImportStmt,
    StmtEnd,

    ExprStart,
    LitExpr,
    BinExpr,
    UnExpr,
    VarExpr,
    FuncCall,
    MethodCall,
    AsgnExpr,
    FieldExpr,
    StructInstance,
    TernaryExpr,
    CastExpr,
    RefExpr,
    DerefExpr,
    NilExpr,
    TypeExpr,
    ExprEnd
};

}
