#pragma once
#include <lexer/token.h>
#include <llvm/Support/SMLoc.h>
#include <string>

namespace veo::basic {

struct NameObj {
    std::string Val;
    llvm::SMLoc Start, End;

    NameObj (std::string val, llvm::SMLoc start, llvm::SMLoc end)
        : Val (std::move (val)), Start (start), End (end) {}
    explicit NameObj (Token tok) : NameObj (std::move (tok.Val), tok.Start, tok.End) {}
    NameObj () = default;
};

}
