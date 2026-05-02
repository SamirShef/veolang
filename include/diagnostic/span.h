#pragma once
#include <llvm/Support/SMLoc.h>

namespace veo::diagnostic {

struct Span {
    llvm::SMLoc Start;
    llvm::SMLoc End;

    Span (llvm::SMLoc start, llvm::SMLoc end) : Start (start), End (end) {
    }
};

}
