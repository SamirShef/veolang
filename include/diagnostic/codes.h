#pragma once
#include <cstdint>

namespace veo::diagnostic {

enum class Severity : uint8_t { Error, Warning, Note, Help };

enum class DiagCode : uint8_t {
    EUnexpectedToken,
    EExpectedStmt,
    EExpectedExpr,
    EUnclosedStrLit,
    EUnclosedCharLit,
    EIncorrectCharLitLen,
    EIntSuffixForFloat,
    EInvalidNumSuffix,
    ECannotImplCast,
    ELitOutOfRange,
    ECannotApplyOp,
    ECannotFindCommonType,
    EDivByZero,
    ERedefinition,
    EUndefined,
    ECannotFindFunction,
    ENoMatchingFunction,
    ECallIsAmbiguous,
    ENotAConst,
    ECannotInitRuntimeVal,

    WUnusedVar,
    WLossPrecision,
};

constexpr inline DiagCode errCodeStart = DiagCode::EUnexpectedToken;
constexpr inline DiagCode errCodeLast
    = static_cast<DiagCode> (static_cast<uint8_t> (DiagCode::WUnusedVar) - 1);

constexpr inline DiagCode warnCodeStart = DiagCode::WUnusedVar;
constexpr inline DiagCode warnCodeLast  = DiagCode::WUnusedVar;

}
