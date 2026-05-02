#pragma once
#include <diagnostic/span.h>
#include <string>
#include <utility>

namespace veo::diagnostic {

struct Annotation {
    struct Span Span;
    std::string Label;
    bool        IsPrimary = true;

    Annotation (struct Span span, std::string label, bool isPrimary)
        : Span (span), Label (std::move (label)), IsPrimary (isPrimary) {
    }
};

}
