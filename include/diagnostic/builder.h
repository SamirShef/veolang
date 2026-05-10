#pragma once
#include <diagnostic/annotation.h>
#include <diagnostic/codes.h>
#include <utility>
#include <vector>

namespace veo::diagnostic {

class DiagnosticBuilder {
    DiagCode                 _code;
    std::string              _message;
    Severity                 _severity;
    std::vector<Annotation>  _spans;
    std::vector<std::string> _notes;

public:
    DiagnosticBuilder (DiagCode code, std::string message, Severity severity)
        : _code (code), _message (std::move (message)), _severity (severity) {}

    DiagnosticBuilder &
    AddSpan (Span span, std::string label = "", bool isPrimary = true) {
        _spans.emplace_back (span, std::move (label), isPrimary);
        return *this;
    }

    DiagnosticBuilder &
    AddSpan (
        llvm::SMLoc start,
        llvm::SMLoc end,
        std::string label     = "",
        bool        isPrimary = true) {
        return AddSpan (Span (start, end), std::move (label), isPrimary);
    }

    DiagnosticBuilder &
    AddNote (std::string text) {
        _notes.emplace_back (std::move (text));
        return *this;
    }

    DiagCode
    Code () const {
        return _code;
    }

    const std::string &
    Message () {
        return _message;
    }

    Severity
    GetSeverity () const {
        return _severity;
    }

    std::vector<Annotation> &
    Spans () {
        return _spans;
    }

    const std::vector<std::string> &
    Notes () {
        return _notes;
    }
};

}
