#pragma once
#include <ast/ast.h>
#include <cstddef>
#include <diagnostic/engine.h>
#include <lexer/lexer.h>
#include <lexer/token.h>
#include <llvm/Support/Allocator.h>

namespace veo::parser {

using namespace veo::ast;

struct ParseResult {
    Node **Nodes;
    size_t Count;
    bool   HasErrors;
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
        std::vector<Node *> nodes;
        bool                hasErrors = false;
        while (!isAtEnd ()) {
            Node *node = parseStmt ();
            if (node != nullptr) {
                nodes.push_back (node);
            } else {
                hasErrors = true;
                synchronize ();
            }
        }
        size_t count = nodes.size ();
        Node **array = nullptr;
        if (count != 0) {
            array = static_cast<Node **> (_allocator.Allocate<Node *> (count));
            std::memcpy (
                static_cast<void *> (array),
                static_cast<void *> (nodes.data ()),
                sizeof (Node *) * count);
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

    Stmt *
    parseStmt (bool expectSemi = true);

    Stmt *
    parseVarDef ();

    Expr *
    parseExpr (bool allowStruct = true);

    Expr *
    parsePrimaryExpr (bool allowStruct = true);

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

    void
    expectSemi ();

    bool
    isAtEnd () const;
};

}
