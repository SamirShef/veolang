#pragma once
#include <llvm/Support/SourceMgr.h>

enum class CommandKind : uint8_t { NoErrors, Error, Warning, Note };

struct Command {
    CommandKind Kind;
    std::string Val;
    unsigned    Line;
};

class CommandsParser {
    const char *_bufStart;
    const char *_bufEnd;
    const char *_curPtr;

public:
    CommandsParser (llvm::SourceMgr &mgr, unsigned buffer) {
        const auto &memBuffer = mgr.getMemoryBuffer (buffer);
        _curPtr = _bufStart = memBuffer->getBufferStart ();
        _bufEnd             = memBuffer->getBufferEnd ();
    }

    std::vector<Command>
    Parse () {
        std::vector<Command> commands;
        unsigned             line = 1;

        while (!isAtEnd ()) {
            char c = peek ();
            if (c == '\n') {
                ++line;
                advance ();
                continue;
            }
            if (c == '/' && peek (1) == '/') {
                auto command = parseCommand (line);
                if (command.has_value ()) {
                    commands.push_back (std::move (*command));
                }
                continue;
            }
            advance ();
        }

        return std::move (commands);
    }

private:
    std::optional<Command>
    parseCommand (unsigned line) {
        advance ();
        advance ();

        std::string text;
        while (!isAtEnd () && peek () != '\n') {
            text += advance ();
        }

        std::string_view view (text);

#define try_match(marker, kind) tryMatch (view, marker, line, CommandKind::kind)
        if (auto cmd = try_match ("expected-no-errors", NoErrors)) {
            return cmd;
        }
        if (auto cmd = try_match ("expected-error", Error)) {
            return cmd;
        }
        if (auto cmd = try_match ("expected-warning", Warning)) {
            return cmd;
        }
        if (auto cmd = try_match ("expected-note", Note)) {
            return cmd;
        }
#undef try_match

        return std::nullopt;
    }

    static std::optional<Command>
    tryMatch (
        std::string_view view, std::string_view marker, unsigned line, CommandKind kind) {
        size_t pos = view.find (marker);
        if (pos == std::string_view::npos) {
            return std::nullopt;
        }
        size_t   idx        = pos + marker.length ();
        unsigned targetLine = line;

        if (idx < view.length () && view[idx] == '@') {
            ++idx;
            if (idx < view.length ()) {
                int sign = 1;
                if (view[idx] == '+') {
                    sign = 1;
                    ++idx;
                } else if (view[idx] == '-') {
                    sign = -1;
                    ++idx;
                }

                size_t numStart = idx;
                while (idx < view.length () && (isdigit (view[idx]) != 0)) {
                    ++idx;
                }

                if (idx == numStart) {
                    return std::nullopt;
                }

                int offset = 0;
                for (size_t i = numStart; i < idx; ++i) {
                    offset = (offset * 10) + (view[i] - '0');
                }

                int calculatedLine = static_cast<int> (line) + (sign * offset);
                targetLine
                    = calculatedLine > 0 ? static_cast<unsigned> (calculatedLine) : 1;
            } else {
                return std::nullopt;
            }
        }

        if (kind != CommandKind::NoErrors) {
            if (idx >= view.length () || view[idx] != ':') {
                return std::nullopt;
            }
            ++idx;
        }

        const auto &valView = view.substr (idx);

        size_t start = valView.find_first_not_of (" \t");
        size_t end   = valView.find_last_not_of (" \t");

        std::string finalVal;
        if (start != std::string_view::npos && end != std::string_view::npos) {
            finalVal = std::string (valView.substr (start, end - start + 1));
        }

        return Command{ .Kind = kind, .Val = std::move (finalVal), .Line = targetLine };
    }

    bool
    isAtEnd () const {
        return _curPtr >= _bufEnd;
    }

    char
    peek (int rpos = 0) {
        const auto *newPos = _curPtr + rpos;
        if (newPos < _bufStart || newPos >= _bufEnd) {
            return '\0';
        }
        return *newPos;
    }

    char
    advance () {
        char c = peek ();
        ++_curPtr;
        return c;
    }
};
