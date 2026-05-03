#pragma once
#include "boilerplate.h"

#include <string>

using namespace veo;
namespace fs = std::filesystem;

inline void
TestComments (const fs::path &root) {
    init (root, "comments.veo");
    std::vector<Token> tokens;
    while (true) {
        const Token tok = lexer.NextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }
        tokens.emplace_back (tok);
    }
    assert_tokens_count (tokens, 0);
}
