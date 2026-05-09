#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestOperators (const fs::path &root) {
    init (root, "operators.veo");
    Lexer              lexer (diag, mgr, bufferId);
    std::vector<Token> tokens;
    while (true) {
        const Token tok = lexer.NextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }
        tokens.emplace_back (tok);
    }

    assert_tokens_count (tokens, 37);

    assert_tok (tokens[0], Bang, "!");
    assert_tok (tokens[1], BangEq, "!=");
    assert_tok (tokens[2], Eq, "=");
    assert_tok (tokens[3], EqEq, "==");
    assert_tok (tokens[4], Lt, "<");
    assert_tok (tokens[5], LtEq, "<=");
    assert_tok (tokens[6], Gt, ">");
    assert_tok (tokens[7], GtEq, ">=");
    assert_tok (tokens[8], Plus, "+");
    assert_tok (tokens[9], PlusEq, "+=");
    assert_tok (tokens[10], Minus, "-");
    assert_tok (tokens[11], MinusEq, "-=");
    assert_tok (tokens[12], Star, "*");
    assert_tok (tokens[13], StarEq, "*=");
    assert_tok (tokens[14], Slash, "/");
    assert_tok (tokens[15], SlashEq, "/=");
    assert_tok (tokens[16], Percent, "%");
    assert_tok (tokens[17], PercentEq, "%=");
    assert_tok (tokens[18], BitAnd, "&");
    assert_tok (tokens[19], LogAnd, "&&");
    assert_tok (tokens[20], BitOr, "|");
    assert_tok (tokens[21], LogOr, "||");
    assert_tok (tokens[22], Semi, ";");
    assert_tok (tokens[23], Comma, ",");
    assert_tok (tokens[24], Dot, ".");
    assert_tok (tokens[25], LParen, "(");
    assert_tok (tokens[26], RParen, ")");
    assert_tok (tokens[27], LBrace, "{");
    assert_tok (tokens[28], RBrace, "}");
    assert_tok (tokens[29], LBracket, "[");
    assert_tok (tokens[30], RBracket, "]");
    assert_tok (tokens[31], At, "@");
    assert_tok (tokens[32], Tilde, "~");
    assert_tok (tokens[33], Question, "?");
    assert_tok (tokens[34], Colon, ":");
    assert_tok (tokens[35], Dollar, "$");
    assert_tok (tokens[36], Carret, "^");
}
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
