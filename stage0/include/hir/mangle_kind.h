#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

inline std::vector<std::string>
SupportedLanguagesForMangling () {
    return { "Veo", "C" };
}

inline std::string
SupportedLanguagesForManglingAsString () {
    auto        langs = SupportedLanguagesForMangling ();
    std::string res;
    size_t      i = 0;
    for (auto &lang : langs) {
        if (i != 0) {
            res += ", ";
        }
        res += lang;
        ++i;
    }
    return res;
}

}
