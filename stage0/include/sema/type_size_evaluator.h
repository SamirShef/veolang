#pragma once
#include <basic/symbols/struct.h>
#include <basic/types/type.h>

namespace veo {

class TypeSizeEvaluator {
public:
    static size_t
    EvaluateAlignment (unsigned ptrSizeInBits, basic::Type *type);

    static size_t
    EvaluateSize (unsigned ptrSizeInBits, basic::Type *type);

private:
    static size_t
    evaluateStructSize (unsigned ptrSizeInBits, symbols::Struct *s);
};

}
