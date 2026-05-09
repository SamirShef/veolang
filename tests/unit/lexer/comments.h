#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestComments (const fs::path &root) {
    init (root, "comments.veo");
    Lexer              lexer (diag, mgr, bufferId);
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
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
