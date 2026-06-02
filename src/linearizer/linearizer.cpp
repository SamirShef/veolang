#include <linearizer/linearizer.h>

namespace veo {

using namespace hir;

void
HIRLinearizer::linearizeFunc (Function *func) {
    for (auto *bb : func->Body ()) {
        linearizeBasicBlock (bb);
    }
}

void
HIRLinearizer::linearizeBasicBlock (BasicBlock *bb) {
    _builder.SetInsertionPoint (bb);

    auto oldNodes = std::move (bb->Body ());
    bb->Clear ();

    for (auto *node : oldNodes) {
        linearizeStmt (node);
    }
}

void
HIRLinearizer::linearizeStmt (Node *node) {
    switch (node->Kind ()) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (node)); break;
        variant (VarDef, linearizeVarDef, VarDef);
        variant (Ret, linearizeRet, Return);
    default: {
    }
#undef variant
    }
}

void
HIRLinearizer::linearizeVarDef (VarDef *vd) {
    auto *init = linearizeExpr (vd->Init ());
    _builder.CreateVariable (
        vd->Name (),
        vd->Type (),
        init,
        vd->IsConst (),
        false,
        vd->Start (),
        vd->End (),
        vd->BaseSymbol ());
}

void
HIRLinearizer::linearizeRet (Return *ret) {
    auto *expr = linearizeExpr (ret->Expr ());
    _builder.CreateRet (expr, ret->Start (), ret->End ());
}

Node *
HIRLinearizer::linearizeExpr (Node *expr) {
    if (expr == nullptr) {
        return nullptr;
    }
    switch (expr->Kind ()) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: return func (llvm::cast<type> (expr));
        variant (LitExpr, linearizeLiteralExpr, LiteralExpr);
        variant (BinExpr, linearizeBinaryExpr, BinaryExpr);
        variant (UnExpr, linearizeUnaryExpr, UnaryExpr);
        variant (LoadVar, linearizeLoadVar, LoadVar);
        variant (FuncCall, linearizeFuncCall, FuncCall);
        variant (NilExpr, linearizeNilExpr, NilExpr);
    default: {
    }
#undef variant
    }
    return nullptr;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Node *
HIRLinearizer::linearizeLiteralExpr (LiteralExpr *le) {
    return le;
}

Node *
HIRLinearizer::linearizeBinaryExpr (BinaryExpr *be) {
    hir::Node *flatLhs = linearizeExpr (be->Lhs ());
    hir::Node *flatRhs = linearizeExpr (be->Rhs ());

    auto *flatBin = _builder.CreateBinary (
        be->Op (),
        be->CommonType (),
        flatLhs,
        flatRhs,
        be->Start (),
        be->End ());

    return emitToTmp (flatBin, be->CommonType (), be->Start (), be->End ());
}

Node *
HIRLinearizer::linearizeUnaryExpr (UnaryExpr *ue) {
    auto *flatRhs = linearizeExpr (ue->Rhs ());

    auto *flatUn = _builder.CreateUnary (
        ue->Op (),
        ue->CommonType (),
        flatRhs,
        ue->Start (),
        ue->End ());

    return emitToTmp (flatUn, ue->CommonType (), ue->Start (), ue->End ());
}

Node *
HIRLinearizer::linearizeLoadVar (LoadVar *lv) {
    return lv;
}

Node *
HIRLinearizer::linearizeFuncCall (FuncCall *fc) {
    std::vector<Node *> args;
    args.reserve (fc->Args ().size ());
    for (auto *a : fc->Args ()) {
        args.emplace_back (linearizeExpr (a));
    }
    return emitToTmp (
        _builder.CreateCall (fc->Function (), std::move (args), fc->Start (), fc->End ()),
        fc->Function ()->RetType,
        fc->Start (),
        fc->End ());
}

Node *
HIRLinearizer::linearizeNilExpr (NilExpr *ne) {
    return ne;
}
// NOLINTEND(readability-convert-member-functions-to-static)

Node *
HIRLinearizer::emitToTmp (
    Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
    size_t id  = getTmpId ();
    auto  *tmp = _builder.CreateVariable (
        basic::NameObj ("t." + std::to_string (id), {}, {}),
        type,
        expr,
        true,
        false,
        start,
        end,
        nullptr);

    return _builder.CreateLoadVar (tmp, type, false, start, end);
}

}
