#pragma once
#include <ast/context.h>
#include <ast/expr.h>
#include <ast/stmts/func_def.h>
#include <ast/stmts/struct_def.h>
#include <basic/name.h>
#include <basic/types/pool.h>
#include <basic/types/type.h>
#include <cstddef>
#include <diagnostic/engine.h>
#include <lexer/lexer.h>
#include <lexer/token.h>
#include <llvm/Support/Allocator.h>
#include <parser/precedence.h>

namespace veo {

struct ParseResult {
    ast::Node **Nodes;
    size_t      Count;
    bool        HasErrors;
};

class Parser {
    Token                         _lastTok;
    Token                         _curTok;
    Token                         _nextTok;
    diagnostic::DiagnosticEngine &_diag;     // NOLINT
    basic::TypePool              &_typePool; // NOLINT
    Lexer                         _lex;
    ast::Context                 &_context; // NOLINT

public:
    explicit Parser (
        diagnostic::DiagnosticEngine &diag,
        Lexer                        &lex,
        basic::TypePool              &typePool,
        ast::Context                 &context)
        : _diag (diag), _lex (lex), _typePool (typePool), _context (context) {
        advance ();
        advance ();
    }

    ParseResult
    Parse () {
        std::vector<ast::Node *> nodes;
        bool                     hasErrors = false;
        while (!isAtEnd ()) {
            auto *node = parseStmt ();
            if (node != nullptr) {
                nodes.push_back (node);
            } else {
                hasErrors = true;
            }
        }
        size_t      count = nodes.size ();
        ast::Node **array = nullptr;
        if (count != 0) {
            array
                = static_cast<ast::Node **> (_context.AllocateArray<ast::Node *> (count));
            std::memcpy (
                static_cast<void *> (array),
                static_cast<void *> (nodes.data ()),
                sizeof (ast::Node *) * count);
        }
        return { .Nodes = array, .Count = count, .HasErrors = hasErrors };
    }

private:
    template <typename T, typename... Args>
    T *
    createNode (Args &&...args) {
        return _context.CreateNode<T> (std::forward<Args> (args)...);
    }

    template <typename T, typename... Args>
    basic::Type *
    createType (Args &&...args) {
        return _typePool.GetOrCreate<T> (std::forward<Args> (args)...);
    }

    ast::Stmt *
    parseStmt (bool expectSemi = true);

    ast::Stmt *
    checkTrailingSemi (ast::Stmt *stmt, bool expect);

    bool
    parseBlock (std::vector<ast::Stmt *> &body);

    void
    parseStmtsIntoBlock (std::vector<ast::Stmt *> &body);

    ast::Stmt *
    parseVarDef (ast::AccessModifier access);

    ast::Stmt *
    parseFuncDef (ast::AccessModifier access);

    ast::Stmt *
    parseRet ();

    ast::Stmt *
    parseIfElse ();

    ast::Stmt *
    parseForLoop ();

    ast::Stmt *
    parseBreakContinue ();

    ast::Stmt *
    parseStructDef (ast::AccessModifier access);

    ast::Stmt *
    parseImplStmt ();

    std::vector<ast::Argument>
    parseArguments ();

    ast::Argument
    parseArgument ();

    void
    parseArgumentsForCall (std::vector<ast::Expr *> &args);

    std::vector<ast::Field>
    parseFields ();

    ast::Field
    parseField ();

    std::vector<std::tuple<basic::NameObj, ast::Expr *>>
    parseFieldsForInstance ();

    std::tuple<basic::NameObj, ast::Expr *>
    parseFieldForInstance ();

    ast::Expr *
    parseExpr (int minPrec = (int) Precedence::Lowest, bool allowStruct = true);

    ast::Expr *
    parsePrimaryExpr (bool allowStruct = true);

    ast::Expr *
    parseChain (ast::Expr *base, bool allowStruct = true);

    ast::Expr *
    parseTernary (ast::Expr *cond);

    basic::Type *
    consumeType ();

    void
    synchronize ();

    bool
    isSynchronizationToken ();

    bool
    match (TokenKind kind);

    static bool
    check (const Token &tok, TokenKind kind);

    bool
    check (TokenKind kind) const {
        return check (_curTok, kind);
    }

    Token
    advance ();

    static bool
    isKeyword (const Token &tok);

    static bool
    isStmtStart (TokenKind kind);

    static bool
    isAsgnOp (TokenKind kind);

    bool
    expectSemi ();

    bool
    expectTok (TokenKind kind, const std::string &val);

    bool
    expectName (basic::NameObj &res);

    bool
    isAtEnd () const;
};

}
