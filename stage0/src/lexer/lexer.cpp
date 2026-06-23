#include <diagnostic/codes.h>
#include <lexer/keywords.h>
#include <lexer/lexer.h>
#include <llvm/Support/SMLoc.h>

namespace veo {

using namespace diagnostic;

#define loc(ptr) llvm::SMLoc::getFromPointer (ptr)

bool
IsDigit (char c, int base) {
    switch (base) {
    case 2: return c >= '0' && c <= '1';
    case 8: return c >= '0' && c <= '7';
    case 10: return isdigit (c) != 0;
    case 16: return isxdigit (c) != 0;
    default: return false;
    }
}

Token
Lexer::NextToken () {
    if (peek () == '/' && (peek (1) == '/' || peek (1) == '*')) {
        skipComments ();
        return NextToken ();
    }
    if (isspace (peek ()) != 0) {
        ++_curPtr;
        return NextToken ();
    }

    if (_curPtr >= _bufEnd) {
        return { TokenKind::Eof, "", loc (_bufEnd), loc (_bufEnd) };
    }
    if ((isalpha (peek ()) != 0) || peek () == '_' || peek () == '@') {
        return tokenizeId (_curPtr);
    }
    if ((isdigit (peek ()) != 0) || peek () == '.' && (isdigit (peek (1)) != 0)) {
        return tokenizeNumLit (_curPtr);
    }
    if (peek () == '\'') {
        return tokenizeCharLit (_curPtr);
    }
    if (peek () == '"') {
        return tokenizeStrLit (_curPtr);
    }
    return tokenizeOp (_curPtr);
}

Token
Lexer::tokenizeId (const char *tokStart) {
    while ((isalnum (peek ()) != 0) || peek () == '_'
           || (peek () == '@' && _curPtr == tokStart)) {
        ++_curPtr;
    }
    std::string val (tokStart, _curPtr - tokStart);

#define tok(kind)                                                                        \
    {                                                                                    \
        kind, val, loc (tokStart), loc (_curPtr)                                         \
    }

    if (const auto &it = keywords.find (val); it != keywords.end ()) {
        return tok (it->second);
    }
    if (val[0] == '@') {
        val.erase (0, 1);
    }
    return tok (TokenKind::Id);

#undef tok
}

Token
Lexer::tokenizeNumLit (const char *tokStart) {
    std::string val;
    bool        isFloat = false;
    int         base    = 10;
    if (peek () == '.') {
        val += "0.";
        isFloat = true;
        ++_curPtr;
    } else if (peek () == '0') {
        switch (tolower (peek (1))) {
        case 'x':
            base = 16;
            val += "0x";
            break;
        case 'o':
            base = 8;
            val += "0o";
            break;
        case 'b':
            base = 2;
            val += "0b";
            break;
        default: base = 10;
        }
        if (base != 10) {
            _curPtr += 2;
        }
    }
    while (peek () != '\0'
           && (IsDigit (peek (), base) || peek () == '.' || peek () == '_')) {
        if (peek () == '_') {
            if (!IsDigit (peek (-1), base)) {
                break;
            }
            ++_curPtr;
            continue;
        }
        if (peek () == '.') {
            if (isFloat || !IsDigit (peek (1), base) || base != 10) {
                break;
            }
            isFloat = true;
        }
        val += *_curPtr++;
    }
    TokenKind kind = parseNumSuffix (isFloat);
    return { kind, val, loc (tokStart), loc (_curPtr) };
}

Token
Lexer::tokenizeStrLit (const char *tokStart) {
    ++_curPtr;
    while (peek () != '\0' && peek () != '\"') {
        ++_curPtr;
    }
    if (peek () == '\0') {
        _diag
            .Report (
                DiagCode::EUnclosedStrLit,
                "missing terminating '\"' character",
                Severity::Error)
            .AddSpan (loc (tokStart), loc (_curPtr));
    }
    std::string val (tokStart + 1, _curPtr - tokStart - 1);
    ++_curPtr;
    return { TokenKind::StrLit, val, loc (tokStart), loc (_curPtr) };
}

Token
Lexer::tokenizeCharLit (const char *tokStart) {
    ++_curPtr;
    unsigned len = 0;
    while (peek () != '\0' && peek () != '\'') {
        ++_curPtr;
        ++len;
    }
    bool unclosed = false;
    if (peek () == '\0') {
        unclosed = true;
        _diag
            .Report (
                DiagCode::EUnclosedCharLit,
                "missing terminating '\'' character",
                Severity::Error)
            .AddSpan (loc (tokStart), loc (_curPtr));
    }
    ++_curPtr;
    if (len > 1 && !unclosed) {
        _diag
            .Report (
                DiagCode::EIncorrectCharLitLen,
                "character literal too long",
                Severity::Error)
            .AddSpan (loc (tokStart), loc (_curPtr));
    } else if (len == 0) {
        _diag
            .Report (
                DiagCode::EIncorrectCharLitLen,
                "empty character literal",
                Severity::Error)
            .AddSpan (loc (tokStart), loc (_curPtr));
    }
    std::string val (tokStart + 1, _curPtr - tokStart - 2);
    return { TokenKind::CharLit, val, loc (tokStart), loc (_curPtr) };
}

Token
Lexer::tokenizeOp (const char *tokStart) {
#define tok(kind)                                                                        \
    { TokenKind::kind,                                                                   \
      std::string (tokStart, _curPtr - tokStart),                                        \
      loc (tokStart),                                                                    \
      loc (_curPtr) }

#define simple(c, kind)                                                                  \
    case c: {                                                                            \
        return tok (kind);                                                               \
    }

#define equal(c, kind)                                                                   \
    case c: {                                                                            \
        if (peek () == '=') {                                                            \
            ++_curPtr;                                                                   \
            return tok (kind##Eq);                                                       \
        }                                                                                \
        return tok (kind);                                                               \
    }

#define triple(c, kind1, kind2, kind3)                                                   \
    case c: {                                                                            \
        if (peek () == (c)) {                                                            \
            ++_curPtr;                                                                   \
            return tok (kind2);                                                          \
        }                                                                                \
        if (peek () == (c)) {                                                            \
            ++_curPtr;                                                                   \
            return tok (kind3);                                                          \
        }                                                                                \
        return tok (kind1);                                                              \
    }

    switch (*_curPtr++) {
        simple (';', Semi);
        simple (',', Comma);
        simple ('.', Dot);
        simple ('(', LParen);
        simple (')', RParen);
        simple ('{', LBrace);
        simple ('}', RBrace);
        simple ('[', LBracket);
        simple (']', RBracket);
        simple ('~', Tilde);
        simple ('?', Question);
        simple (':', Colon);
        simple ('$', Dollar);
        equal ('=', Eq);
        equal ('!', Bang);
        equal ('>', Gt);
        equal ('<', Lt);
        equal ('+', Plus);
        equal ('-', Minus);
        equal ('*', Star);
        equal ('/', Slash);
        equal ('%', Percent);
        equal ('^', Carret);
        triple ('&', BitAnd, LogAnd, AndEq);
        triple ('|', BitOr, LogOr, OrEq);
    default: return tok (Unknown);
    }

#undef simple
#undef equal
#undef double_op
#undef tok
}

void
Lexer::skipComments () {
    _curPtr += 2;
    bool isMultilineComment = peek (-1) == '*';
    if (isMultilineComment) {
        while (peek () != '\0' && (peek () != '*' || peek (1) != '/')) {
            ++_curPtr;
        }
        _curPtr += 2;
    } else {
        while (peek () != '\0' && peek () != '\n') {
            ++_curPtr;
        }
        ++_curPtr;
    }
}

TokenKind // NOLINTNEXTLINE(readability-function-cognitive-complexity)
Lexer::parseNumSuffix (bool isFloat) {
    if (isspace (peek ()) != 0) {
        return isFloat ? TokenKind::F64Lit : TokenKind::IntLit;
    }

    const char *start = _curPtr;
    auto        match = [&] (const char *suffix, int len) {
        for (int i = 0; i < len; ++i) {
            if (suffix[i] != peek (i)) {
                return false;
            }
        }
        if (isalnum (peek (len)) != 0 || peek (len) == '_') {
            return false;
        }
        _curPtr += len;
        return true;
    };
    char first = peek ();

#define kind_macro(val) TokenKind::val##Lit

#define match_macro(str, kind)                                                           \
    if (match (str, sizeof (str) - 1)) {                                                 \
        return kind_macro (kind);                                                        \
    }

#define int_group(prefix)                                                                \
    match_macro ("8", prefix##8) match_macro ("16", prefix##16)                          \
        match_macro ("32", prefix##32) match_macro ("64", prefix##64)                    \
            match_macro ("128", prefix##128) match_macro ("z", prefix##Size)

    if (first == 'u') {
        if (isFloat) {
            _diag
                .Report (
                    DiagCode::EIntSuffixForFloat,
                    "integer suffix cannot be applied to a float",
                    Severity::Error)
                .AddSpan (loc (start), loc (_curPtr));
            return kind_macro (F64);
        }
        ++_curPtr;
        int_group (U);
    } else if (first == 'i') {
        if (isFloat) {
            _diag
                .Report (
                    DiagCode::EIntSuffixForFloat,
                    "integer suffix cannot be applied to a float",
                    Severity::Error)
                .AddSpan (loc (start), loc (_curPtr));
            return kind_macro (F64);
        }
        ++_curPtr;
        int_group (I);
    } else if (first == 'f') {
        ++_curPtr;
        match_macro ("32", F32) match_macro ("64", F64)
    }
#undef int_group
#undef match_macro
#undef kind_macro

    _curPtr = start;
    return isFloat ? TokenKind::F64Lit : TokenKind::IntLit;
}

char
Lexer::peek (int relPos) {
    if (_curPtr + relPos < _bufStart || _curPtr + relPos > _bufEnd) {
        return '\0';
    }
    return *(_curPtr + relPos);
}

#undef loc

}
