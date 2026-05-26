#pragma once
#include <cstdint>

namespace veo::hir {

enum class NodeKind : uint8_t {
    VarDef,
    LoadVar,
    LoadGlobalVarByName,
    Func,
    FuncCall,
    Ret,
    LitExpr,
    BinExpr,
    UnExpr,
    ExprStmt,
    Store,
    Branch,
    StructDef,
    StructInstance,
    FieldExpr,
    TernaryExpr,
};

}
