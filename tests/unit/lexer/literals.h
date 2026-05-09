#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestLiterals (const fs::path &root) {
    init (root, "literals.veo");
    Lexer              lexer (diag, mgr, bufferId);
    std::vector<Token> tokens;
    while (true) {
        const Token tok = lexer.NextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }
        tokens.emplace_back (tok);
    }
    assert_tokens_count (tokens, 10);
    assert_tok (tokens[0], IntLit, "1000000");
    assert_tok (tokens[1], F64Lit, "2000000.2");
    assert_tok (tokens[2], U8Lit, "3");
    assert_tok (tokens[3], I16Lit, "4");
    assert_tok (tokens[4], USizeLit, "5");
    assert_tok (tokens[5], F32Lit, "6.7");
    assert_tok (tokens[6], F32Lit, "7.2");
    assert_tok (tokens[7], IntLit, "8");
    assert_tok (tokens[8], Dot, ".");
    assert_tok (tokens[9], Id, "Field");
}
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
