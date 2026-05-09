#pragma once
#include <lexer/token_kind.h>
#include <llvm/Support/SMLoc.h>
#include <string>

namespace veo {

struct Token {
    TokenKind   Kind;
    std::string Val;
    llvm::SMLoc Start;
    llvm::SMLoc End;

    Token (TokenKind kind, std::string val, llvm::SMLoc start, llvm::SMLoc end)
        : Kind (kind), Val (std::move (val)), Start (start), End (end) {}
    Token () : Kind (TokenKind::Unknown) {}
};

}
