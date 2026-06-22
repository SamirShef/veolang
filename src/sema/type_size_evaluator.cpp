#include <basic/types/all.h>
#include <sema/type_size_evaluator.h>

namespace veo {

using namespace basic;

size_t
TypeSizeEvaluator::EvaluateAlignment (unsigned ptrSizeInBits, Type *type) {
    switch (type->Kind ()) {
    case TypeKind::Pointer: return (ptrSizeInBits + 7) / 8;
    case TypeKind::Integer: return (type->AsInteger ()->BitWidth () + 7) / 8;
    case TypeKind::Size: return (ptrSizeInBits + 7) / 8;
    case TypeKind::Floating:
        return type->AsFloating ()->GetFloatingKind () == FloatingKind::Float ? 4 : 8;
    case TypeKind::Bool: return 1;
    case TypeKind::Char: return 4;
    case TypeKind::Struct: {
        size_t maxAlign = 1;
        for (auto &field : type->AsStruct ()->BaseSymbol ()->Fields) {
            maxAlign = std::max (maxAlign, EvaluateAlignment (ptrSizeInBits, field.Type));
        }
        return maxAlign;
    }
    case TypeKind::Alias:
        return EvaluateAlignment (ptrSizeInBits, type->AsAlias ()->Base ());
    case basic::TypeKind::Trait:
    case basic::TypeKind::Named:
    case basic::TypeKind::Noth:
    case basic::TypeKind::TraitThis:
    case basic::TypeKind::Module: return 1;
    }
    return 1;
}

size_t
TypeSizeEvaluator::EvaluateSize (unsigned ptrSizeInBits, Type *type) {
    switch (type->Kind ()) {
    case TypeKind::Pointer: return (ptrSizeInBits + 7) / 8;
    case TypeKind::Integer: return (type->AsInteger ()->BitWidth () + 7) / 8;
    case TypeKind::Size: return (ptrSizeInBits + 7) / 8;
    case TypeKind::Floating:
        return type->AsFloating ()->GetFloatingKind () == FloatingKind::Float ? 4 : 8;
    case TypeKind::Bool: return 1;
    case TypeKind::Char: return 4;
    case TypeKind::Struct:
        return evaluateStructSize (ptrSizeInBits, type->AsStruct ()->BaseSymbol ());
    case TypeKind::Alias: return EvaluateSize (ptrSizeInBits, type->AsAlias ()->Base ());
    case TypeKind::Noth:
    case TypeKind::Trait:
    case TypeKind::TraitThis:
    case TypeKind::Named:
    case TypeKind::Module: return 0;
    }
    return 0;
}

size_t
TypeSizeEvaluator::evaluateStructSize (unsigned ptrSizeInBits, symbols::Struct *s) {
    size_t currentSize = 0;
    size_t maxAlign    = 1;

    for (const auto &field : s->Fields) {
        size_t fieldSize  = EvaluateSize (ptrSizeInBits, field.Type);
        size_t fieldAlign = EvaluateAlignment (ptrSizeInBits, field.Type);

        maxAlign    = std::max (maxAlign, fieldAlign);
        currentSize = (currentSize + fieldAlign - 1) / fieldAlign * fieldAlign;
        currentSize += fieldSize;
    }

    currentSize = (currentSize + maxAlign - 1) / maxAlign * maxAlign;

    return currentSize;
}

}
