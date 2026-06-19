#include <ast/access_modifier.h>
#include <ast/exprs/asgn_expr.h>
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/cast_expr.h>
#include <ast/exprs/deref.h>
#include <ast/exprs/field_expr.h>
#include <ast/exprs/func_call.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/method_call.h>
#include <ast/exprs/nil.h>
#include <ast/exprs/ref.h>
#include <ast/exprs/struct_instance.h>
#include <ast/exprs/ternary_expr.h>
#include <ast/exprs/type_expr.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/break_continue.h>
#include <ast/stmts/expr_stmt.h>
#include <ast/stmts/extern_stmt.h>
#include <ast/stmts/for_loop.h>
#include <ast/stmts/func_def.h>
#include <ast/stmts/if_else.h>
#include <ast/stmts/impl_stmt.h>
#include <ast/stmts/ret.h>
#include <ast/stmts/struct_def.h>
#include <ast/stmts/trait_stmt.h>
#include <ast/stmts/var_def.h>
#include <basic/name.h>
#include <basic/types/all.h>
#include <basic/types/type.h>
#include <diagnostic/codes.h>
#include <lexer/keywords.h>
#include <parser/parser.h>

namespace veo {

using namespace diagnostic;
using namespace ast;

Stmt *
Parser::parseStmt (bool expectSemi) {
    if (isAtEnd ()) {
        return nullptr;
    }

    auto access = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
    switch (_curTok.Kind) {
    case TokenKind::Let:
    case TokenKind::Const: {
        return checkTrailingSemi (parseVarDef (access), expectSemi);
    }
    case TokenKind::Func: {
        return parseFuncDef (access);
    }
    case TokenKind::Ret: {
        return checkTrailingSemi (parseRet (), expectSemi);
    }
    case TokenKind::If: {
        return parseIfElse ();
    }
    case TokenKind::For: {
        return parseForLoop ();
    }
    case TokenKind::Break:
    case TokenKind::Continue: {
        return checkTrailingSemi (parseBreakContinue (), expectSemi);
    }
    case TokenKind::Struct: {
        return parseStructDef (access);
    }
    case TokenKind::Impl: {
        return parseImplStmt ();
    }
    case TokenKind::Trait: {
        return parseTraitStmt ();
    }
    case TokenKind::Extern: {
        return parseExternStmt ();
    }
    default: {
        Expr *expr = parseExpr ();
        if (expr == nullptr) {
            return nullptr;
        }
        if (expectSemi && !this->expectSemi ()) {
            return nullptr;
        }
        auto *stmt   = createNode<ExprStmt> (expr);
        stmt->End () = _curTok.End;
        return stmt;
    }
    }
}

ast::Stmt *
Parser::checkTrailingSemi (ast::Stmt *stmt, bool expect) {
    if (stmt == nullptr) {
        return nullptr;
    }
    if (expect && !this->expectSemi ()) {
        return nullptr;
    }
    return stmt;
}

bool
Parser::parseBlock (std::vector<Stmt *> &body) {
    if (!expectTok (TokenKind::LBrace, "{")) {
        return false;
    }
    parseStmtsIntoBlock (body);
    return true;
}

void
Parser::parseStmtsIntoBlock (std::vector<Stmt *> &body) {
    while (!isAtEnd () && !match (TokenKind::RBrace)) {
        Stmt *stmt = parseStmt ();
        if (stmt != nullptr) {
            body.push_back (stmt);
        }
    }
}

Stmt *
Parser::parseVarDef (ast::AccessModifier access) {
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
Parser::parseFuncDef (ast::AccessModifier access) {
    const Token    firstTok = advance ();
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }
    bool                      isGeneric = false;
    std::vector<GenericParam> genericParams;
    if (match (TokenKind::Lt)) {
        genericParams = std::move (parseGenericParams ());
        isGeneric     = true;
    }
    if (!expectTok (TokenKind::LParen, "(")) {
        return nullptr;
    }
    std::vector<Argument> args    = parseArguments ();
    basic::Type          *retType = nullptr;
    if (match (TokenKind::Colon)) {
        retType = consumeType ();
        if (retType == nullptr) {
            return nullptr;
        }
    } else {
        retType = createType<basic::NothType> ();
    }
    std::vector<Stmt *> body;
    bool                isDeclaration = false;
    if (check (TokenKind::Semi)) {
        isDeclaration = true;
        expectSemi ();
    } else if (!parseBlock (body)) {
        return nullptr;
    }
    return createNode<FuncDef> (
        std::move (name),
        retType,
        std::move (args),
        std::move (body),
        isDeclaration,
        std::move (genericParams),
        access,
        firstTok.Start,
        _lastTok.End);
}

std::vector<GenericParam>
Parser::parseGenericParams () {
    std::vector<GenericParam> params;
    while (!isAtEnd () && !matchGtForGeneric ()) {
        auto param = parseGenericParam ();
        if (!param.IsValid ()) {
            continue;
        }
        params.push_back (param);
        if (!checkGtForGeneric ()) {
            if (!expectTok (TokenKind::Comma, ",")) {
                break;
            }
        }
    }
    return std::move (params);
}

GenericParam
Parser::parseGenericParam () {
    basic::NameObj name;
    if (!expectName (name)) {
        return GenericParam::Invalid ();
    }
    return { name };
}

std::vector<basic::Type *>
Parser::parseGenericParamsForCall () {
    std::vector<basic::Type *> params;
    while (!isAtEnd () && !matchGtForGeneric ()) {
        auto *param = parseGenericParamForCall ();
        if (param == nullptr) {
            continue;
        }
        params.push_back (param);
        if (!checkGtForGeneric ()) {
            if (!expectTok (TokenKind::Comma, ",")) {
                break;
            }
        }
    }
    return std::move (params);
}

basic::Type *
Parser::parseGenericParamForCall () {
    return consumeType ();
}

Stmt *
Parser::parseRet () {
    const Token firstTok = advance ();
    Expr       *expr     = nullptr;
    if (!check (TokenKind::Semi)) {
        expr = parseExpr ();
        if (expr == nullptr) {
            return nullptr;
        }
    }
    return createNode<Return> (expr, firstTok.Start, _curTok.End);
}

Stmt *
Parser::parseIfElse () {
    const Token         firstTok = advance ();
    Expr               *cond     = parseExpr ((int) Precedence::Lowest, false);
    std::vector<Stmt *> thenBranch;
    if (!parseBlock (thenBranch)) {
        return nullptr;
    }
    std::vector<Stmt *> elseBranch;
    if (match (TokenKind::Else)) {
        if (match (TokenKind::LBrace)) {
            parseStmtsIntoBlock (elseBranch);
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
                if (indexator == nullptr) {
                    return nullptr;
                }
            }
        }
        cond = parseExpr ();
        if (cond == nullptr) {
            return nullptr;
        }
        if (!expectSemi ()) {
            return nullptr;
        }
        if (!check (TokenKind::LBrace)) {
            iteration = parseStmt (false);
            if (iteration == nullptr) {
                return nullptr;
            }
        }
    }

    std::vector<Stmt *> body;
    if (!parseBlock (body)) {
        return nullptr;
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
Parser::parseStructDef (ast::AccessModifier access) {
    const Token    firstTok = advance ();
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }
    std::vector<GenericParam> genericParams;
    if (match (TokenKind::Lt)) {
        genericParams = std::move (parseGenericParams ());
    }
    bool isDeclaration = false;
    if (check (TokenKind::Semi)) {
        isDeclaration = true;
        expectSemi ();
    } else if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Field> fields;
    if (!isDeclaration) {
        fields = std::move (parseFields ());
    }
    return createNode<StructDef> (
        std::move (name),
        std::move (fields),
        isDeclaration,
        std::move (genericParams),
        access,
        firstTok.Start,
        _lastTok.End);
}

ast::Stmt *
Parser::parseImplStmt () {
    const Token  firstTok          = advance ();
    basic::Type *structOrTraitType = consumeType ();
    if (structOrTraitType == nullptr) {
        return nullptr;
    }
    basic::Type *structType = structOrTraitType;
    basic::Type *traitType  = nullptr;
    if (structOrTraitType == nullptr) {
        return nullptr;
    }
    if (match (TokenKind::For)) {
        traitType  = structOrTraitType;
        structType = consumeType ();
        if (structType == nullptr) {
            return nullptr;
        }
    }
    if (structType == nullptr) {
        return nullptr;
    }
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Method> methods;
    while (!isAtEnd () && !match (TokenKind::RBrace)) {
        auto access = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
        bool isStatic = match (TokenKind::Static);
        auto *fd      = parseFuncDef (access);
        if (fd != nullptr) {
            auto *method = llvm::cast<FuncDef> (fd);
            methods.emplace_back (method, isStatic);
        }
    }
    return createNode<ImplStmt> (
        structType,
        traitType,
        std::move (methods),
        firstTok.Start,
        _lastTok.End);
}

ast::Stmt *
Parser::parseTraitStmt () {
    const Token    firstTok = advance ();
    basic::NameObj name;
    if (!expectName (name)) {
        return nullptr;
    }
    if (!expectTok (TokenKind::LBrace, "{")) {
        return nullptr;
    }
    std::vector<Method> methods;
    while (!isAtEnd () && !match (TokenKind::RBrace)) {
        auto access = match (TokenKind::Pub) ? AccessModifier::Pub : AccessModifier::Priv;
        bool isStatic = match (TokenKind::Static);
        auto *fd      = parseFuncDef (access);
        if (fd != nullptr) {
            auto *method = llvm::cast<FuncDef> (fd);
            methods.emplace_back (method, isStatic);
        }
    }
    return createNode<TraitStmt> (
        std::move (name),
        std::move (methods),
        firstTok.Start,
        _lastTok.End);
}

Stmt *
Parser::parseExternStmt () {
    const Token    firstTok = advance ();
    basic::NameObj from;
    if (check (TokenKind::StrLit)) {
        from = basic::NameObj (advance ());
    }
    std::vector<Stmt *> body;
    if (match (TokenKind::LBrace)) {
        parseStmtsIntoBlock (body);
    } else {
        Stmt *stmt = parseStmt ();
        if (stmt != nullptr) {
            body.push_back (stmt);
        }
    }
    return createNode<ExternStmt> (
        std::move (from),
        std::move (body),
        firstTok.Start,
        _lastTok.End);
}

std::vector<Argument>
Parser::parseArguments () {
    std::vector<Argument> args;
    size_t                i = 0;
    while (!isAtEnd () && !match (TokenKind::RParen)) {
        if (i != 0) {
            if (!expectTok (TokenKind::Comma, ",")) {
                break;
            }
        }
        const Argument arg = parseArgument ();
        if (arg.IsValid ()) {
            args.push_back (arg);
        } else {
            break;
        }
        ++i;
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

void
Parser::parseArgumentsForCall (std::vector<ast::Expr *> &args) {
    size_t i = 0;
    while (!isAtEnd () && !match (TokenKind::RParen)) {
        if (i != 0) {
            if (!expectTok (TokenKind::Comma, ",")) {
                break;
            }
        }
        Expr *expr = parseExpr ();
        if (expr != nullptr) {
            args.emplace_back (expr);
        }
        ++i;
    }
}

std::vector<Field>
Parser::parseFields () {
    std::vector<Field> fields;
    while (!isAtEnd () && !match (TokenKind::RBrace)) {
        const Field field = parseField ();
        if (field.IsValid ()) {
            fields.push_back (field);
        } else {
            break;
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
    size_t                                          i = 0;
    while (!isAtEnd () && !match (TokenKind::RBrace)) {
        if (i != 0) {
            if (!expectTok (TokenKind::Comma, ",")) {
                break;
            }
        }
        auto field         = parseFieldForInstance ();
        auto &[name, expr] = field;
        if (name != basic::NameObj () && expr != nullptr) {
            fields.push_back (field);
        } else {
            break;
        }
        ++i;
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
    while (!isAtEnd () && minPrec < (prec = GetTokPrecedence (_curTok.Kind))) {
        if (match (TokenKind::Question)) { // ternary operator
            lhs = parseTernary (lhs);
            if (lhs == nullptr) {
                return nullptr;
            }
            continue;
        }
        if (check (TokenKind::Dot)) {
            lhs = parseChain (lhs, allowStruct);
            if (lhs == nullptr) {
                return nullptr;
            }
            continue;
        }
        if (isAsgnOp (_curTok.Kind)) {
            AsgnOp op   = TokToAsgnOp (advance ().Kind);
            Expr  *init = parseExpr (prec - 1, allowStruct);
            if (init == nullptr) {
                return nullptr;
            }
            lhs = createNode<AsgnExpr> (op, lhs, init, lhs->Start (), init->End ());
            continue;
        }

        const Token op = advance ();

        Expr *rhs = parseExpr (prec, allowStruct);
        if (rhs == nullptr) {
            return nullptr;
        }
        lhs = createNode<BinaryExpr> (
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
    auto *typeExpr = tryParseAsTypeExpr ();
    if (typeExpr != nullptr) {
        return typeExpr;
    }

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
        lit (ISizeLit);
        lit (USizeLit);

    case TokenKind::LParen: {
        Expr *expr = parseExpr ();
        if (expr == nullptr) {
            return nullptr;
        }
        expr->Start () = tok.Start;
        expr->End ()   = _lastTok.End;
        if (!expectTok (TokenKind::RParen, ")")) {
            return nullptr;
        }
        return expr;
    }
    case TokenKind::Minus:
    case TokenKind::Bang:
    case TokenKind::Tilde: {
        return createNode<UnaryExpr> (
            TokToUnOp (tok.Kind),
            parseExpr ((int) Precedence::Unary, allowStruct),
            tok.Start,
            _lastTok.End);
    }
    case TokenKind::Id: {
        std::vector<basic::Type *> genericParams;
        bool                       hasGenerics = false;
        if (match (TokenKind::Bang)) {
            if (!expectTok (TokenKind::Lt, "<")) {
                return nullptr;
            }
            genericParams = std::move (parseGenericParamsForCall ());
            hasGenerics   = true;
        }
        if (allowStruct && match (TokenKind::LBrace)) { // StructInstance
            auto fields = parseFieldsForInstance ();
            return createNode<StructInstance> (
                createType<basic::NamedType> (
                    std::vector<basic::NameObj>{ basic::NameObj (tok) },
                    std::move (genericParams)),
                std::move (fields),
                tok.Start,
                _lastTok.End);
        }
        if (match (TokenKind::LParen)) { // FuncCall
            std::vector<Expr *> args;
            parseArgumentsForCall (args);
            return createNode<FuncCall> (
                basic::NameObj (tok),
                std::move (args),
                std::move (genericParams),
                tok.Start,
                _lastTok.End);
        }
        if (hasGenerics) {
            _diag
                .Report (
                    DiagCode::EUnexpectedToken,
                    "expected '{' or '(' after generic arguments",
                    Severity::Error)
                .AddSpan (tok.Start, tok.End);
            return nullptr;
        }
        // VarExpr
        return createNode<VarExpr> (basic::NameObj (tok), tok.Start, tok.End);
    }
    case TokenKind::Star: {
        Expr *expr = parseExpr ((int) Precedence::Unary, allowStruct);
        if (expr == nullptr) {
            return nullptr;
        }
        return createNode<DerefExpr> (expr, tok.Start, expr->End ());
    }
    case TokenKind::BitAnd: {
        Expr *expr = parseExpr ((int) Precedence::Unary, allowStruct);
        if (expr == nullptr) {
            return nullptr;
        }
        return createNode<RefExpr> (expr, tok.Start, expr->End ());
    }
    case TokenKind::Nil: {
        return createNode<NilExpr> (tok.Start, tok.End);
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
    synchronize ();
    return nullptr;
}

Expr *
Parser::tryParseAsTypeExpr () {
    switch (_curTok.Kind) {
    case TokenKind::Bool:
    case TokenKind::Char:
    case TokenKind::I8:
    case TokenKind::I16:
    case TokenKind::I32:
    case TokenKind::I64:
    case TokenKind::U8:
    case TokenKind::U16:
    case TokenKind::U32:
    case TokenKind::U64:
    case TokenKind::F32:
    case TokenKind::F64:
    case TokenKind::USize:
    case TokenKind::ISize: {
        llvm::SMLoc  start = _curTok.Start;
        basic::Type *type  = consumeType ();
        if (type == nullptr) {
            return nullptr;
        }
        return createNode<TypeExpr> (type, start, _lastTok.End);
    }
    default: {
        return nullptr;
    }
    }
}

Expr *
Parser::parseChain (Expr *base, bool allowStruct) {
    std::vector<basic::NameObj> path;
    bool                        allowTypeExpr = true;
    while (true) {
        if (match (TokenKind::Dot)) {
            if (match (TokenKind::LParen)) { // cast
                base          = parseCastOperator (base);
                allowTypeExpr = false;
                continue;
            }

            const Token    tok = _curTok;
            basic::NameObj name;
            if (!expectName (name)) {
                return nullptr;
            }
            path.emplace_back (name);
            std::vector<basic::Type *> genericParams;
            bool                       hasGenerics = false;
            if (match (TokenKind::Bang)) {
                if (!expectTok (TokenKind::Lt, "<")) {
                    return nullptr;
                }
                genericParams = std::move (parseGenericParamsForCall ());
                hasGenerics   = true;
            }

            if (allowStruct && match (TokenKind::LBrace)) { // StructInstance
                auto fields = parseFieldsForInstance ();
                return createNode<StructInstance> (
                    createType<basic::NamedType> (
                        std::move (path),
                        std::move (genericParams)),
                    std::move (fields),
                    path.front ().Start,
                    _lastTok.End);
            }

            if (match (TokenKind::LParen)) {
                allowStruct   = false;
                allowTypeExpr = false;
                std::vector<Expr *> args;
                parseArgumentsForCall (args);
                base = createNode<MethodCall> (
                    base,
                    basic::NameObj (tok),
                    std::move (args),
                    std::move (genericParams),
                    base->Start (),
                    _lastTok.End);
            } else {
                if (hasGenerics) {
                    _diag
                        .Report (
                            DiagCode::EUnexpectedToken,
                            "expected '{' or '(' after generic arguments",
                            Severity::Error)
                        .AddSpan (tok.Start, tok.End);
                    return nullptr;
                }
                base = createNode<FieldExpr> (
                    base,
                    std::move (name),
                    base->Start (),
                    tok.End);
            }
        } else {
            break;
        }
    }
    return base;
}

Expr *
Parser::parseCastOperator (ast::Expr *base) {
    basic::Type *type = consumeType ();
    if (type == nullptr) {
        return nullptr;
    }
    if (!expectTok (TokenKind::RParen, ")")) {
        return nullptr;
    }
    return createNode<ast::CastExpr> (type, base, base->Start (), _lastTok.End);
}

Expr *
Parser::parseTernary (Expr *cond) {
    Expr *trueVal = parseExpr ();
    if (trueVal == nullptr) {
        return nullptr;
    }
    if (!expectTok (TokenKind::Colon, ":")) {
        return nullptr;
    }
    Expr *falseVal = parseExpr ();
    if (falseVal == nullptr) {
        return nullptr;
    }
    return createNode<TernaryExpr> (
        cond,
        trueVal,
        falseVal,
        cond->Start (),
        falseVal->End ());
}

basic::Type *
Parser::consumeType () {
#define kind(kind) case TokenKind::kind:
#define int_type(prefix, is_unsigned)                                                    \
    return createType<basic::IntegerType> (                                              \
        1U << ((unsigned) tok.Kind - (unsigned) TokenKind::prefix##8 + 3),               \
        is_unsigned);
#define float_type()                                                                     \
    return createType<basic::FloatingType> (                                             \
        (basic::FloatingKind) ((unsigned) tok.Kind - (unsigned) TokenKind::F32));

    const Token tok = advance ();
    switch (tok.Kind) {
        kind (Bool) return createType<basic::BoolType> ();
        kind (Char) return createType<basic::CharType> ();
        kind (I8) kind (I16) kind (I32) kind (I64) int_type (I, false);
        kind (U8) kind (U16) kind (U32) kind (U64) int_type (U, true);
        kind (F32) kind (F64) float_type ();
        kind (Id) {
            std::vector<basic::NameObj> path;
            path.emplace_back (tok);
            while (match (TokenKind::Dot)) {
                basic::NameObj name;
                if (!expectName (name)) {
                    return nullptr;
                }
                path.emplace_back (name);
            }
            std::vector<basic::Type *> genericParams;
            if (match (TokenKind::Lt)) {
                genericParams = std::move (parseGenericParamsForCall ());
            }
            return createType<basic::NamedType> (
                std::move (path),
                std::move (genericParams));
        }
        kind (Star) return createType<basic::PointerType> (consumeType ());
        kind (ISize) kind (USize) return createType<basic::SizeType> (
            tok.Kind == TokenKind::USize);

    default:
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected type, found '" + tok.Val + "'",
                Severity::Error)
            .AddSpan (tok.Start, tok.End, "expected a type name");
        synchronize ();
        return nullptr;
    }
#undef float_type
#undef int_type
#undef kind
}

void
Parser::synchronize () {
    if (isSynchronizationToken ()) {
        return;
    }
    advance ();
    while (!isAtEnd ()) {
        if (isSynchronizationToken ()) {
            return;
        }
        advance ();
    }
}

bool
Parser::isSynchronizationToken () {
    if (isStmtStart (_curTok.Kind)) {
        return true;
    }
    if (check (_lastTok, TokenKind::Semi) || check (_lastTok, TokenKind::RBrace)
        || check (_lastTok, TokenKind::RParen)) {
        return true;
    }
    return false;
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
Parser::isKeyword (const Token &tok) {
    return keywords.contains (tok.Val);
}

bool
Parser::isStmtStart (TokenKind kind) {
#define variant(kind) case TokenKind::kind:
    switch (kind) {
        variant (Let) variant (Const) variant (Func) variant (Ret) variant (If)
            variant (Else) variant (For) variant (Break) variant (Continue)
                variant (Struct) variant (Pub) variant (Impl) variant (Trait)
                    variant (Mod) variant (Import) variant (Static) return true;
    default: return false;
    }
#undef variant
}

bool
Parser::isAsgnOp (TokenKind kind) {
#define variant(kind) case TokenKind::kind:
    switch (kind) {
        variant (Eq) variant (PlusEq) variant (MinusEq) variant (StarEq) variant (SlashEq)
            variant (PercentEq) variant (AndEq) variant (OrEq)
                variant (CarretEq) return true;
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
        synchronize ();
        return false;
    }
    return true;
}

bool
Parser::expectName (basic::NameObj &res) {
    const Token tok = advance ();
    if (tok.Kind != TokenKind::Id) {
        auto &err = _diag
                        .Report (
                            DiagCode::EUnexpectedToken,
                            "expected identifier, found '" + tok.Val + "'",
                            Severity::Error)
                        .AddSpan (tok.Start, tok.End, "expected identifier");
        if (isKeyword (tok)) {
            err.AddNote ("'" + tok.Val + "' is a reserved keyword");
        } else {
            err.AddNote ("'" + tok.Val + "' is a operator");
        }
        synchronize ();
        return false;
    }
    res = basic::NameObj (tok);
    return true;
}

bool
Parser::expectGtForGeneric () {
    if (check (TokenKind::GtEq)) {
        _curTok = Token (
            TokenKind::Eq,
            { _curTok.Val[1] },
            llvm::SMLoc::getFromPointer (_curTok.Start.getPointer () + 1),
            _curTok.End);
        return true;
    }
    // TODO: add support '>>'
    return expectTok (TokenKind::Gt, ">");
}

bool
Parser::matchGtForGeneric () {
    if (check (TokenKind::GtEq)) {
        _curTok = Token (
            TokenKind::Eq,
            { _curTok.Val[1] },
            llvm::SMLoc::getFromPointer (_curTok.Start.getPointer () + 1),
            _curTok.End);
        return true;
    }
    // TODO: add support '>>'
    return match (TokenKind::Gt);
}

bool
Parser::checkGtForGeneric () {
    if (check (TokenKind::GtEq)) {
        return true;
    }
    // TODO: add support '>>'
    return check (TokenKind::Gt);
}

bool
Parser::isAtEnd () const {
    return check (TokenKind::Eof);
}

}
