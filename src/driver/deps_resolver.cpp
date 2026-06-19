#include <driver/deps_resolver.h>

namespace veo::driver {

#define loc(ptr) llvm::SMLoc::getFromPointer (ptr)

std::vector<std::string>
DepsResolver::Deps () {
    std::vector<std::string> deps;

    while (true) {
        const auto &tok = nextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }

        if (tok.Kind == TokenKind::Id && tok.Val == "import") {
            // parsing imports
            std::string res;

            const auto &maybeId = nextToken ();
            if (maybeId.Kind != TokenKind::Id) {
                break;
            }
            res += maybeId.Val;

            bool ok = true;
            while (nextToken ().Kind == TokenKind::Dot) {
                const auto &maybeId = nextToken ();
                if (maybeId.Kind != TokenKind::Id) {
                    ok = false;
                    break;
                }
                res += '.' + maybeId.Val;
            }
            if (ok) {
                deps.push_back (std::move (res));
            }
        }
    }

    return std::move (deps);
}

Token
DepsResolver::nextToken () {
    if (peek () == '\0') {
        return { TokenKind::Eof, "", loc (_curPtr), loc (_curPtr) };
    }
    if (peek () == '.') {
        ++_curPtr;
        return { TokenKind::Dot, ".", loc (_curPtr - 1), loc (_curPtr - 1) };
    }
    if ((isalpha (peek ()) != 0) || peek () == '_') {
        return tokenizeId (_curPtr);
    }
    ++_curPtr;
    return nextToken ();
}

Token
DepsResolver::tokenizeId (const char *tokStart) {
    while ((isalnum (peek ()) != 0) || peek () == '_') {
        ++_curPtr;
    }
    std::string val (tokStart, _curPtr - tokStart);

    return { TokenKind::Id, val, loc (tokStart), loc (_curPtr) };
}

char
DepsResolver::peek (int relPos) {
    if (_curPtr + relPos < _bufStart || _curPtr + relPos > _bufEnd) {
        return '\0';
    }
    return *(_curPtr + relPos);
}

}
