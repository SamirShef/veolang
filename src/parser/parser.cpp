#include <diagnostic/codes.h>
#include <parser/parser.h>

namespace veo::parser {

using namespace diagnostic;

Stmt *
Parser::parseStmt (bool expectSemi) {
    switch (_curTok.Kind) {
    case TokenKind::Let:
    case TokenKind::Const: {
        auto *vds = parseVarDef ();
        if (expectSemi) {
            if (!match (TokenKind::Semi)) {
                this->expectSemi ();
            }
        }
        return vds;
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
    return nullptr;
}

Stmt *
Parser::parseVarDef () {
    bool isConst = advance ().Kind == TokenKind::Const; // consume 'let' or 'const'
    return nullptr;
}

Expr *
Parser::parseExpr (bool allowStruct) {
    return parseExpr (allowStruct);
}

Expr *
Parser::parsePrimaryExpr (bool allowStruct) {
    _diag
        .Report (
            DiagCode::EExpectedExpr,
            "expected expression, found '" + _curTok.Val + "'",
            Severity::Error)
        .AddSpan (_curTok.Start, _curTok.End, "expected expression here");
    return nullptr;
}

void
Parser::synchronize () {
    advance ();
    while (!isAtEnd ()) {
        if (check (_lastTok, TokenKind::Semi)) {
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

void
Parser::expectSemi () {
    if (!match (TokenKind::Semi)) {
        auto expectedPos = llvm::SMLoc::getFromPointer (_lastTok.End.getPointer () + 1);
        _diag
            .Report (
                DiagCode::EUnexpectedToken,
                "expected ';', found '" + _curTok.Val + "'",
                Severity::Error)
            .AddSpan (
                _curTok.Start,
                _curTok.End,
                "found '" + _curTok.Val + "' instead",
                false)
            .AddSpan (expectedPos, expectedPos);
    }
}

bool
Parser::isAtEnd () const {
    return check (TokenKind::Eof);
}

}
