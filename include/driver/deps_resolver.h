#pragma once
#include <diagnostic/engine.h>
#include <lexer/token.h>
#include <llvm/Support/SourceMgr.h>

namespace veo::driver {

class DepsResolver {
    const char *_bufStart{};
    const char *_bufEnd;
    const char *_curPtr;

public:
    DepsResolver (const char *bufStart, const char *bufEnd)
        : _bufEnd (bufEnd), _curPtr (_bufStart = bufStart) {}

    std::vector<std::string>
    Deps ();

private:
    Token
    nextToken ();

    Token
    tokenizeId (const char *tokStart);

    Token
    tokenizeOp (const char *tokStart);

    char
    peek (int relPos = 0);
};

}
