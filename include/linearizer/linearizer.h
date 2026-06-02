#pragma once
#include <hir/builder.h>
#include <hir/context.h>

namespace veo {

class HIRLinearizer {
    hir::Context &_ctx;     // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    hir::Builder &_builder; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    size_t        _nextTmpId = 0;

public:
    explicit HIRLinearizer (hir::Builder &builder)
        : _ctx (builder.GetContext ()), _builder (builder) {}

    void
    Linearize () {
        for (auto *func : _ctx.Functions ()) {
            linearizeFunc (func);
        }
    }

private:
    void
    linearizeFunc (hir::Function *func);

    void
    linearizeBasicBlock (hir::BasicBlock *bb);

    void
    linearizeStmt (hir::Node *node);

    void
    linearizeVarDef (hir::VarDef *vd);

    void
    linearizeRet (hir::Return *ret);

    hir::Node *
    linearizeExpr (hir::Node *expr);

    hir::Node *
    linearizeLiteralExpr (hir::LiteralExpr *le);

    hir::Node *
    linearizeBinaryExpr (hir::BinaryExpr *be);

    hir::Node *
    linearizeUnaryExpr (hir::UnaryExpr *ue);

    hir::Node *
    linearizeLoadVar (hir::LoadVar *lv);

    hir::Node *
    linearizeFuncCall (hir::FuncCall *fc);

    hir::Node *
    linearizeNilExpr (hir::NilExpr *ne);

    size_t
    getTmpId () {
        return _nextTmpId++;
    }

    std::string
    getTmpIdStr () {
        return std::to_string (getTmpId ());
    }

    hir::Node *
    emitToTmp (hir::Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end);
};

}
