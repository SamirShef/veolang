#pragma once
#include <ast/ast.h>
#include <ast/stmts/func_def.h>
#include <basic/name.h>
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
    diagnostic::DiagnosticEngine &_diag; // NOLINT
    Lexer                         _lex;  // NOLINT
    llvm::BumpPtrAllocator        _allocator;

public:
    explicit Parser (diagnostic::DiagnosticEngine &diag, Lexer &lex)
        : _diag (diag), _lex (lex) {
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
            array = static_cast<ast::Node **> (_allocator.Allocate<ast::Node *> (count));
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
        void *mem = _allocator.Allocate (sizeof (T), alignof (T));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto *obj = new (mem) T (std::forward<Args> (args)...);
        return obj;
    }

    ast::Stmt *
    parseStmt (bool expectSemi = true);

    ast::Stmt *
    parseVarDef ();

    ast::Stmt *
    parseFuncDef ();

    ast::Stmt *
    parseRet ();

    std::vector<ast::Argument>
    parseArguments ();

    ast::Argument
    parseArgument ();

    ast::Expr *
    parseExpr (int minPrec = (int) Precedence::Unary, bool allowStruct = true);

    ast::Expr *
    parsePrimaryExpr (bool allowStruct = true);

    basic::Type *
    consumeType ();

    void
    synchronize ();

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
    isKeyword (TokenKind kind);

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
