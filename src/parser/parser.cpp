#include <ast/access_modifier.h>
#include <ast/exprs/asgn_expr.h>
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/field_expr.h>
#include <ast/exprs/func_call.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/method_call.h>
#include <ast/exprs/struct_instance.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/break_continue.h>
#include <ast/stmts/expr_stmt.h>
#include <ast/stmts/for_loop.h>
#include <ast/stmts/func_def.h>
#include <ast/stmts/if_else.h>
#include <ast/stmts/impl_stmt.h>
#include <ast/stmts/ret.h>
#include <ast/stmts/struct_def.h>
#include <ast/stmts/var_def.h>
#include <basic/name.h>
#include <basic/types/all.h>
#include <basic/types/type.h>
#include <diagnostic/codes.h>
#include <parser/parser.h>

namespace veo {

using namespace diagnostic;
using namespace ast;

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
    case TokenKind::Ret: {
        auto *ret = parseRet ();
        if (expectSemi) {
            if (!this->expectSemi ()) {
                return nullptr;
            }
        }
        return ret;
    }
    case TokenKind::If: {
        return parseIfElse ();
    }
    case TokenKind::For: {
        return parseForLoop ();
    }
    case TokenKind::Break:
    case TokenKind::Continue: {
        auto *bc = parseBreakContinue ();
        if (expectSemi) {
            if (!this->expectSemi ()) {
                return nullptr;
            }
        }
        return bc;
    }
    case TokenKind::Struct: {
        return parseStructDef ();
    }
    case TokenKind::Impl: {
        return parseImplStmt ();
    }
    case TokenKind::Id: {
        Expr *expr = parseExpr ();
        if (expectSemi) {
            if (!this->expectSemi ()) {
                return nullptr;
            }
        }
        if (expr == nullptr) {
            return nullptr;
        }
        auto *stmt   = createNode<ExprStmt> (expr);
        stmt->End () = _curTok.End;
        return stmt;
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

Stmt *
Parser::parseRet () {
    const Token firstTok = advance ();
    Expr       *expr     = nullptr;
    if (!check (TokenKind::Semi)) {
        expr = parseExpr ();
    }
    return createNode<Return> (expr, firstTok.Start, _curTok.End);
}

Stmt *
Parser::parseIfElse () {
    const Token firstTok = advance ();
    Expr       *cond     = parseExpr ((int) Precedence::Unary, false);
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Stmt *> thenBranch;
    while (!match (TokenKind::RBrace)) {
        Stmt *stmt = parseStmt ();
        if (stmt != nullptr) {
            thenBranch.push_back (stmt);
        }
    }
    std::vector<Stmt *> elseBranch;
    if (match (TokenKind::Else)) {
        if (match (TokenKind::LBrace)) {
            while (!match (TokenKind::RBrace)) {
                Stmt *stmt = parseStmt ();
                if (stmt != nullptr) {
                    elseBranch.push_back (stmt);
                }
            }
        } else {
            Stmt *stmt = parseStmt ();
            if (stmt != nullptr) {
                elseBranch.push_back (stmt);
            }
        }
    }
    return createNode<IfElseStmt> (
        cond,
        std::move (thenBranch),
        std::move (elseBranch),
        firstTok.Start,
        _lastTok.End);
}

Stmt *
Parser::parseForLoop () {
    const Token firstTok  = advance ();
    Stmt       *indexator = nullptr;
    Expr       *cond      = nullptr;
    Stmt       *iteration = nullptr;

    if (!check (TokenKind::LBrace)) {
        if (!match (TokenKind::Semi)) {
            if (check (TokenKind::Let)) {
                indexator = parseStmt (); // consume semi
            }
        }
        cond = parseExpr ();
        if (!expectSemi ()) {
            return nullptr;
        }
        if (!check (TokenKind::LBrace)) {
            iteration = parseStmt (false);
        }
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
    return createNode<ForLoopStmt> (
        cond,
        indexator,
        iteration,
        std::move (body),
        firstTok.Start,
        _lastTok.End);
}

Stmt *
Parser::parseBreakContinue () {
    const Token firstTok = advance ();
    auto        kind = firstTok.Kind == TokenKind::Break ? BreakContinue::Kind::Break
                                                         : BreakContinue::Kind::Continue;
    return createNode<BreakContinue> (kind, firstTok.Start, _curTok.End);
}

Stmt *
Parser::parseStructDef () {
    auto           access   = Access;
    const Token    firstTok = advance ();
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Field> fields = parseFields ();
    return createNode<StructDef> (
        std::move (name),
        std::move (fields),
        access,
        firstTok.Start,
        _lastTok.End);
}

ast::Stmt *
Parser::parseImplStmt () {
    const Token  firstTok          = advance ();
    basic::Type *structOrTraitType = consumeType ();
    basic::Type *structType        = structOrTraitType;
    basic::Type *traitType         = nullptr;
    if (structOrTraitType == nullptr) {
        return nullptr;
    }
    if (match (TokenKind::For)) {
        traitType  = structOrTraitType;
        structType = consumeType ();
    }
    if (structType == nullptr) {
        return nullptr;
    }
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Method> methods;
    while (!match (TokenKind::RBrace)) {
        Access = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
        bool  isStatic = match (TokenKind::Static);
        auto *method   = llvm::cast<FuncDef> (parseFuncDef ());
        if (method != nullptr) {
            methods.emplace_back (method, isStatic);
        } else {
            synchronize ();
        }
    }
    return createNode<ImplStmt> (
        structType,
        traitType,
        std::move (methods),
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
        if (!check (TokenKind::RParen)) {
            expectTok (TokenKind::Comma, ",");
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

std::vector<Field>
Parser::parseFields () {
    std::vector<Field> fields;
    while (!match (TokenKind::RBrace)) {
        const Field field = parseField ();
        if (field.IsValid ()) {
            fields.push_back (field);
        }
    }
    return std::move (fields);
}

Field
Parser::parseField () {
    AccessModifier access
        = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
    bool           isStatic = match (TokenKind::Static);
    bool           isConst  = match (TokenKind::Const);
    basic::NameObj name;
    if (!expectName (name)) {
        return Field::Invalid ();
    }
    if (!expectTok (TokenKind::Colon, ":")) {
        return Field::Invalid ();
    }
    basic::Type *type = consumeType ();
    if (type == nullptr) {
        return Field::Invalid ();
    }
    if (!expectTok (TokenKind::Semi, ";")) {
        return Field::Invalid ();
    }
    return { name, type, isStatic, isConst, access };
}

std::vector<std::tuple<basic::NameObj, Expr *>>
Parser::parseFieldsForInstance () {
    std::vector<std::tuple<basic::NameObj, Expr *>> fields;
    while (!match (TokenKind::RBrace)) {
        auto field         = parseFieldForInstance ();
        auto &[name, expr] = field;
        if (name != basic::NameObj () && expr != nullptr) {
            fields.push_back (field);
        }
        if (!check (TokenKind::RBrace)) {
            expectTok (TokenKind::Comma, ",");
        }
    }
    return std::move (fields);
}

std::tuple<basic::NameObj, Expr *>
Parser::parseFieldForInstance () {
    basic::NameObj name;
    if (!expectName (name)) {
        return { basic::NameObj (), nullptr };
    }
    if (!expectTok (TokenKind::Colon, ":")) {
        return { basic::NameObj (), nullptr };
    }
    Expr *expr = parseExpr ();
    if (expr == nullptr) {
        return { basic::NameObj (), nullptr };
    }
    return { name, expr };
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
        return createNode<LiteralExpr> (                                                 \
            std::move (tok.Val),                                                         \
            tok.Kind,                                                                    \
            tok.Start,                                                                   \
            tok.End);
#define int_lits(prefix)                                                                 \
    lit (prefix##8Lit) lit (prefix##16Lit) lit (prefix##32Lit) lit (prefix##64Lit)
    switch (tok.Kind) { // NOLINTBEGIN(bugprone-branch-clone)
        lit (BoolLit);
        lit (CharLit);
        int_lits (I);
        int_lits (U);
        lit (IntLit);
        lit (F32Lit);
        lit (F64Lit);

    case TokenKind::LParen: {
        Expr *expr     = parseExpr ();
        expr->Start () = tok.Start;
        expr->End ()   = _lastTok.End;
        if (!expectTok (TokenKind::RParen, ")")) {
            return nullptr;
        }
        return expr;
    }
    case TokenKind::Minus:
    case TokenKind::Bang: {
        return createNode<UnaryExpr> (
            TokToUnOp (tok.Kind),
            parsePrimaryExpr (allowStruct),
            tok.Start,
            _lastTok.End);
    }
    case TokenKind::Id: {
        if (allowStruct && match (TokenKind::LBrace)) { // StructInstance
            auto fields = parseFieldsForInstance ();
            return createNode<StructInstance> (
                basic::NameObj (tok),
                std::move (fields),
                tok.Start,
                _lastTok.End);
        }
        if (match (TokenKind::LParen)) { // FuncCall
            std::vector<Expr *> args;
            while (!match (TokenKind::RParen)) {
                Expr *expr = parseExpr ();
                if (expr != nullptr) {
                    args.emplace_back (expr);
                }
                if (!check (TokenKind::RParen)) {
                    expectTok (TokenKind::Comma, ",");
                }
            }
            return createNode<FuncCall> (
                basic::NameObj (tok),
                std::move (args),
                tok.Start,
                _lastTok.End);
        }
        // VarExpr
        Expr *expr = createNode<VarExpr> (basic::NameObj (tok), tok.Start, tok.End);

        if (check (TokenKind::Dot)) {
            expr = parseChain (expr, allowStruct);
        }

        if (isAsgnOp (_curTok.Kind)) {
            AsgnOp op   = TokToAsgnOp (advance ().Kind);
            Expr  *init = parseExpr ();
            if (init == nullptr) {
                return nullptr;
            }
            return createNode<AsgnExpr> (op, expr, init, expr->Start (), init->End ());
        }
        return expr;
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

Expr *
Parser::parseChain (Expr *base, bool allowStruct) {
    while (true) {
        if (match (TokenKind::Dot)) {
            const Token    tok = _curTok;
            basic::NameObj name;
            if (!expectName (name)) {
                _diag
                    .Report (
                        DiagCode::EExpectedIdOrMemberName,
                        "expected identifier or member name after '.'",
                        Severity::Error)
                    .AddSpan (
                        tok.Start,
                        tok.End,
                        "expected member, found '" + tok.Val + "'");
                return nullptr;
            }

            if (allowStruct && match (TokenKind::LBrace)) { // StructInstance
                auto fields = parseFieldsForInstance ();
                return createNode<StructInstance> (
                    std::move (name),
                    std::move (fields),
                    tok.Start,
                    _lastTok.End);
            }

            if (match (TokenKind::LParen)) {
                std::vector<Expr *> args;
                while (!match (TokenKind::RParen)) {
                    Expr *expr = parseExpr ();
                    if (expr != nullptr) {
                        args.emplace_back (expr);
                    }
                    if (!check (TokenKind::RParen)) {
                        expectTok (TokenKind::Comma, ",");
                    }
                }
                return createNode<MethodCall> (
                    base,
                    basic::NameObj (tok),
                    std::move (args),
                    tok.Start,
                    _lastTok.End);
            }
            base = createNode<FieldExpr> (base, std::move (name), tok.Start, tok.End);
        } else {
            break;
        }
    }
    return base;
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
        return new basic::IntegerType (
            1U << ((unsigned) tok.Kind - (unsigned) TokenKind::I8 + 3));
    case TokenKind::U8:
    case TokenKind::U16:
    case TokenKind::U32:
    case TokenKind::U64:
        return new basic::IntegerType (
            1U << ((unsigned) tok.Kind - (unsigned) TokenKind::U8 + 3),
            true);
    case TokenKind::F32:
    case TokenKind::F64:
        return new basic::FloatingType (
            (basic::FloatingKind) ((unsigned) tok.Kind - (unsigned) TokenKind::F32));
    case TokenKind::Id: {
        std::vector<basic::NameObj> path;
        path.emplace_back (tok);
        while (match (TokenKind::Dot)) {
            basic::NameObj name;
            if (!expectName (name)) {
                return nullptr;
            }
            path.emplace_back (name);
        }
        return new basic::NamedType (std::move (path));
    }
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
    advance ();
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
Parser::isAsgnOp (TokenKind kind) {
#define variant(kind) case TokenKind::kind:
    switch (kind) {
        variant (Eq) variant (PlusEq) variant (MinusEq) variant (StarEq) variant (SlashEq)
            variant (PercentEq) return true;
    default: return false;
    }
#undef variant
}

bool
Parser::expectSemi () {
    if (!match (TokenKind::Semi)) {
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected ';', found '" + _curTok.Val + "'",
                Severity::Error)
            .AddSpan (_curTok.Start, _curTok.End, "expected ';'");
        synchronize ();
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
        auto &err = _diag
                        .Report (
                            DiagCode::EUnexpectedToken,
                            "expected identifier, found '" + tok.Val + "'",
                            Severity::Error)
                        .AddSpan (tok.Start, tok.End, "expected identifier");
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
