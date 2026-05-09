#pragma once
#include <diagnostic/engine.h>
#include <lexer/token.h>
#include <llvm/Support/SourceMgr.h>

namespace veo {

class Lexer {
    const char                   *_bufStart;
    const char                   *_bufEnd;
    const char                   *_curPtr;
    diagnostic::DiagnosticEngine &_diag; // NOLINT

public:
    Lexer (diagnostic::DiagnosticEngine &diag, llvm::SourceMgr &mgr, unsigned buffer)
        : _diag (diag) {
        const auto &memBuffer = mgr.getMemoryBuffer (buffer);
        _curPtr = _bufStart = memBuffer->getBufferStart ();
        _bufEnd             = memBuffer->getBufferEnd ();
    }

    Token
    NextToken ();

private:
    Token
    tokenizeId (const char *tokStart);

    Token
    tokenizeNumLit (const char *tokStart);

    Token
    tokenizeStrLit (const char *tokStart);

    Token
    tokenizeCharLit (const char *tokStart);

    Token
    tokenizeOp (const char *tokStart);

    void
    skipComments ();

    TokenKind
    parseNumSuffix (bool isFloat);

    char
    peek (int relPos = 0);
};

}
