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
        variant (ExprStmt, linearizeExprStmt, ExprStmt);
        variant (Branch, linearizeBranch, Branch);
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

void
HIRLinearizer::linearizeExprStmt (ExprStmt *es) {
    auto *expr = linearizeExpr (es->Expr ());
    _builder.CreateExprStmt (expr, es->Start (), es->End ());
}

void
HIRLinearizer::linearizeBranch (Branch *br) {
    auto *cond = br->Cond () != nullptr ? linearizeExpr (br->Cond ()) : nullptr;
    _builder.CreateBr (cond, br->Then (), br->Else (), br->Start (), br->End ());
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
        variant (Store, linearizeStore, Store);
        variant (FieldExpr, linearizeFieldExpr, FieldExpr);
        variant (StructInstance, linearizeStructInstance, StructInstance);
        variant (LoadGlobalVarByName, linearizeLoadGlobalVarByName, LoadGlobalVarByName);
        // variant (TernaryExpr, linearizeTernaryExpr, TernaryExpr);
        variant (Cast, linearizeCast, Cast);
        variant (RefExpr, linearizeRefExpr, RefExpr);
        variant (DerefExpr, linearizeDerefExpr, DerefExpr);
        variant (NilExpr, linearizeNilExpr, NilExpr);
    default: {
    }
#undef variant
    }
    return nullptr;
}

Node *
HIRLinearizer::linearizeLValue (Node *expr) {
    bool oldAsLvalue = _asLValue;
    _asLValue        = true;
    return linearizeExpr (expr);
    _asLValue = oldAsLvalue;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Node *
HIRLinearizer::linearizeLiteralExpr (LiteralExpr *le) {
    return le;
}

Node *
HIRLinearizer::linearizeBinaryExpr (BinaryExpr *be) {
    auto *flatLhs = linearizeExpr (be->Lhs ());
    auto *flatRhs = linearizeExpr (be->Rhs ());
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
    auto *flatUn  = _builder.CreateUnary (
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
HIRLinearizer::linearizeStore (Store *store) {
    auto *ptr  = linearizeLValue (store->Ptr ());
    auto *expr = linearizeExpr (store->Expr ());
    return emitToTmp (
        _builder.CreateStore (ptr, expr, store->Type (), store->Start (), store->End ()),
        store->Type (),
        store->Start (),
        store->End ());
}

Node *
HIRLinearizer::linearizeFieldExpr (FieldExpr *fe) {
    auto *base = linearizeExpr (fe->Base ());
    return emitToTmp (
        _builder
            .CreateFieldExpr (base, fe->Index (), fe->Type (), fe->Start (), fe->End ()),
        fe->Type (),
        fe->Start (),
        fe->End ());
}

Node *
HIRLinearizer::linearizeStructInstance (StructInstance *si) {
    std::vector<std::pair<size_t, Node *>> fields;
    fields.reserve (si->Fields ().size ());
    for (auto &[index, node] : si->Fields ()) {
        fields.emplace_back (index, linearizeExpr (node));
    }
    return emitToTmp (
        _builder.CreateStructInstance (
            std::move (fields),
            si->BaseSymbol (),
            si->Type (),
            si->Start (),
            si->End ()),
        si->Type (),
        si->Start (),
        si->End ());
}

Node *
HIRLinearizer::linearizeLoadGlobalVarByName (LoadGlobalVarByName *load) {
    return load;
}

// Node *
// HIRLinearizer::linearizeTernaryExpr (TernaryExpr *te) {
//
// }

Node *
HIRLinearizer::linearizeCast (Cast *cast) {
    auto *expr = linearizeExpr (cast->Expr ());
    return emitToTmp (
        _builder.CreateCast (
            cast->GetCastKind (),
            cast->Type (),
            expr,
            cast->Start (),
            cast->End ()),
        cast->Type (),
        cast->Start (),
        cast->End ());
}

Node *
HIRLinearizer::linearizeRefExpr (RefExpr *re) {
    auto *expr = linearizeLValue (re->Expr ());
    return emitToTmp (
        _builder.CreateReference (expr, re->Type (), re->Start (), re->End ()),
        re->Type (),
        re->Start (),
        re->End ());
}

Node *
HIRLinearizer::linearizeDerefExpr (DerefExpr *de) {
    auto *expr = linearizeExpr (de->Expr ());
    return emitToTmp (
        _builder.CreateDereference (expr, de->Type (), de->Start (), de->End ()),
        de->Type (),
        de->Start (),
        de->End ());
}

Node *
HIRLinearizer::linearizeNilExpr (NilExpr *ne) {
    return ne;
}
// NOLINTEND(readability-convert-member-functions-to-static)

Node *
HIRLinearizer::emitToTmp (
    Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
    if (_asLValue) {
        return expr;
    }
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
