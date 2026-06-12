#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace veo::hir {

enum class MangleKind : uint8_t { Veo, C };

inline std::optional<MangleKind>
MangleKindFromString (const std::string &str) {
    if (str.empty () || str == "Veo") {
        return MangleKind::Veo;
    }
    if (str == "C") {
        return MangleKind::C;
    }
    return std::nullopt;
}

}
