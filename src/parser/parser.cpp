#include <ast/access_modifier.h>
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/func_def.h>
#include <ast/stmts/var_def.h>
#include <basic/name.h>
#include <basic/types/all.h>
#include <basic/types/type.h>
#include <diagnostic/codes.h>
#include <parser/parser.h>

namespace veo {

using namespace diagnostic;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static AccessModifier Access = AccessModifier::Priv;

Stmt *
Parser::parseStmt (bool expectSemi) {
    Access = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
    switch (_curTok.Kind) {
    case TokenKind::Let:
    case TokenKind::Const: {
        auto *vds = parseVarDef ();
        if (expectSemi) {
            if (!this->expectSemi ()) {
                return nullptr;
            }
        }
        return vds;
    }
    case TokenKind::Func: {
        return parseFuncDef ();
    }
        // clang-format off
    default: {}
        // clang-format on
    }
    _diag
        .Report (
            DiagCode::EExpectedStmt,
            "expected statement, found '" + _curTok.Val + "'",
            Severity::Error)
        .AddSpan (_curTok.Start, _curTok.End, "expected statement here");
    advance ();
    synchronize ();
    return nullptr;
}

Stmt *
Parser::parseVarDef () {
    AccessModifier access   = Access;
    const Token    firstTok = advance ();
    bool           isConst  = firstTok.Kind == TokenKind::Const;
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }

    basic::Type *type = nullptr;
    if (match (TokenKind::Colon)) {
        type = consumeType ();
        if (type == nullptr) {
            return nullptr;
        }
    }
    Expr *expr = nullptr;
    if (match (TokenKind::Eq)) {
        expr = parseExpr ();
        if (expr == nullptr) {
            return nullptr;
        }
    }
    return createNode<VarDef> (
        name,
        type,
        expr,
        isConst,
        access,
        firstTok.Start,
        _curTok.End);
}

Stmt *
Parser::parseFuncDef () {
    AccessModifier access   = Access;
    const Token    firstTok = advance ();
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }
    if (!expectTok (TokenKind::LParen, "(")) {
        return nullptr;
    }
    std::vector<Argument> args    = parseArguments ();
    basic::Type          *retType = nullptr;
    if (match (TokenKind::Colon)) {
        retType = consumeType ();
    }
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Stmt *> body;
    while (!match (TokenKind::RBrace)) {
        Stmt *stmt = parseStmt ();
        if (stmt != nullptr) {
            body.push_back (stmt);
        }
    }
    return createNode<FuncDef> (
        std::move (name),
        retType,
        std::move (args),
        std::move (body),
        access,
        firstTok.Start,
        _lastTok.End);
}

std::vector<Argument>
Parser::parseArguments () {
    std::vector<Argument> args;
    while (!match (TokenKind::RParen)) {
        const Argument arg = parseArgument ();
        if (arg.IsValid ()) {
            args.push_back (arg);
        }
    }
    return std::move (args);
}

Argument
Parser::parseArgument () {
    basic::NameObj name;
    if (!expectName (name)) {
        return Argument::Invalid ();
    }
    if (!expectTok (TokenKind::Colon, ":")) {
        return Argument::Invalid ();
    }
    basic::Type *type = consumeType ();
    if (type == nullptr) {
        return Argument::Invalid ();
    }
    return { name, type };
}

Expr *
Parser::parseExpr (int minPrec, bool allowStruct) {
    Expr *lhs = parsePrimaryExpr (allowStruct);
    if (lhs == nullptr) {
        return nullptr;
    }

    int prec = 0;
    while (minPrec < (prec = GetTokPrecedence (_curTok.Kind))) {
        const Token op = advance ();

        Expr *rhs = parseExpr (prec, allowStruct);
        lhs       = createNode<BinaryExpr> (
            TokToBinOp (op.Kind),
            lhs,
            rhs,
            lhs->Start (),
            _lastTok.End);
    }

    return lhs;
}

Expr *
Parser::parsePrimaryExpr (bool allowStruct) {
    const Token tok = advance ();
#define lit(kind)                                                                        \
    case TokenKind::kind:                                                                \
        return createNode<LiteralExpr> (std::move (tok.Val), tok.Start, tok.End);
#define int_lits(prefix)                                                                 \
    lit (prefix##8Lit) lit (prefix##16Lit) lit (prefix##32Lit) lit (prefix##64Lit)       \
        lit (prefix##128Lit)
    switch (tok.Kind) { // NOLINTBEGIN(bugprone-branch-clone)
        lit (BoolLit);
        lit (CharLit);
        int_lits (I);
        int_lits (U);
        lit (IntLit);
        lit (F32Lit);
        lit (F64Lit);

    case TokenKind::Id: {
        return createNode<VarExpr> (basic::NameObj (tok), tok.Start, tok.End);
    }
    default: {
    }
    } // NOLINTEND(bugprone-branch-clone)
#undef int_lits
#undef lit
    _diag
        .Report (
            DiagCode::EExpectedExpr,
            "expected expression, found '" + tok.Val + "'",
            Severity::Error)
        .AddSpan (tok.Start, tok.End, "expected expression here");
    return nullptr;
}

// NOLINTBEGIN(cppcoreguidelines-owning-memory)
basic::Type *
Parser::consumeType () {
    const Token tok = advance ();
    switch (tok.Kind) {
    case TokenKind::Bool: return new basic::BoolType ();
    case TokenKind::Char: return new basic::CharType ();
    case TokenKind::I8:
    case TokenKind::I16:
    case TokenKind::I32:
    case TokenKind::I64:
    case TokenKind::I128:
        return new basic::IntegerType (
            1U << ((unsigned) tok.Kind - (unsigned) TokenKind::I8 + 3));
    case TokenKind::U8:
    case TokenKind::U16:
    case TokenKind::U32:
    case TokenKind::U64:
    case TokenKind::U128:
        return new basic::IntegerType (
            1U << ((unsigned) tok.Kind - (unsigned) TokenKind::U8 + 3),
            true);
    case TokenKind::F32:
    case TokenKind::F64:
        return new basic::FloatingType (
            (basic::FloatingKind) ((unsigned) TokenKind::F64 - (unsigned) tok.Kind));
    default:
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected type, found '" + tok.Val + "'",
                Severity::Error)
            .AddSpan (tok.Start, tok.End, "expected a type name");
        return nullptr;
    }
}
// NOLINTEND(cppcoreguidelines-owning-memory)

void
Parser::synchronize () {
    while (!isAtEnd ()) {
        if (check (_lastTok, TokenKind::Semi) || check (_lastTok, TokenKind::RBrace)) {
            return;
        }
        if (isKeyword (_curTok.Kind)) {
            return;
        }
        advance ();
    }
}

bool
Parser::match (TokenKind kind) {
    if (check (kind)) {
        advance ();
        return true;
    }
    return false;
}

bool
Parser::check (const Token &tok, TokenKind kind) {
    return tok.Kind == kind;
}

Token
Parser::advance () {
    const Token tok = _curTok;
    _lastTok        = _curTok;
    _curTok         = _nextTok;
    _nextTok        = _lex.NextToken ();
    return tok;
}

bool
Parser::isKeyword (TokenKind kind) {
#define variant(kind) case TokenKind::kind:
    switch (kind) {
        variant (Let) variant (Const) variant (Func) variant (Ret) variant (If)
            variant (Else) variant (For) variant (Break) variant (Continue)
                variant (Struct) variant (Pub) variant (Impl) variant (Trait)
                    variant (Del) variant (Mod) variant (Import)
                        variant (Static) return true;
    default: return false;
    }
#undef variant
}

bool
Parser::expectSemi () {
    if (!match (TokenKind::Semi)) {
        auto expectedPos = llvm::SMLoc::getFromPointer (_lastTok.End.getPointer () + 1);
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected ';', found '" + _curTok.Val + "'",
                Severity::Error)
            .AddSpan (_curTok.Start, _curTok.End, "expected ';'", false)
            .AddSpan (expectedPos, expectedPos);
        return false;
    }
    return true;
}

bool
Parser::expectTok (TokenKind kind, const std::string &val) {
    if (!match (kind)) {
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected '" + val + "', found '" + _curTok.Val + "'",
                Severity::Error)
            .AddSpan (_curTok.Start, _curTok.End, "expected '" + val + "'");
        return false;
    }
    return true;
}

bool
Parser::expectName (basic::NameObj &res) {
    const Token tok = advance ();
    res             = basic::NameObj (tok);
    if (tok.Kind != TokenKind::Id) {
        auto &err
            = _diag
                  .Report (
                      DiagCode::EUnexpectedToken,
                      "expected identifier, found '" + _curTok.Val + "'",
                      Severity::Error)
                  .AddSpan (_curTok.Start, _curTok.End, "expected identifier", false)
                  .AddSpan (tok.Start, tok.End);
        if (isKeyword (tok.Kind)) {
            err.AddNote ("'" + tok.Val + "' is a reserved keyword");
        } else {
            err.AddNote ("'" + tok.Val + "' is a operator");
        }
        return false;
    }
    return true;
}

bool
Parser::isAtEnd () const {
    return check (TokenKind::Eof);
}

}
