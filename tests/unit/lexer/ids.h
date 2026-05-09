#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestIds (const fs::path &root) {
    init (root, "ids.veo");
    Lexer              lexer (diag, mgr, bufferId);
    std::vector<Token> tokens;
    while (true) {
        const Token tok = lexer.NextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }
        tokens.emplace_back (tok);
    }
    assert_tokens_count (tokens, 11);
    assert_tok (tokens[0], Id, "hello");
    assert_tok (tokens[1], Id, "_hello2");
    assert_tok (tokens[2], Id, "hello_world");
    assert_tok (tokens[3], Id, "my_var");
    assert_tok (tokens[4], Id, "one");
    assert_tok (tokens[5], Id, "two");
    assert_tok (tokens[6], Let, "let");
    assert_tok (tokens[7], Id, "x");
    assert_tok (tokens[8], I32, "i32");
    assert_tok (tokens[9], Func, "func");
    assert_tok (tokens[10], Id, "main");
}
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
