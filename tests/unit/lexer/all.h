#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestAll (const fs::path &root) {
    init (root, "all.veo");
    Lexer              lexer (diag, mgr, bufferId);
    std::vector<Token> tokens;
    while (true) {
        const Token tok = lexer.NextToken ();
        if (tok.Kind == TokenKind::Eof) {
            break;
        }
        tokens.emplace_back (tok);
    }

    assert_tokens_count (tokens, 58);

    // Literals
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

    // Identifiers and keywords
    assert_tok (tokens[10], Id, "hello");
    assert_tok (tokens[11], Id, "_hello2");
    assert_tok (tokens[12], Id, "hello_world");
    assert_tok (tokens[13], Id, "my_var");
    assert_tok (tokens[14], Id, "one");
    assert_tok (tokens[15], Id, "two");
    assert_tok (tokens[16], Let, "let");
    assert_tok (tokens[17], Id, "x");
    assert_tok (tokens[18], I32, "i32");
    assert_tok (tokens[19], Func, "func");
    assert_tok (tokens[20], Id, "main");

    // Operators
    assert_tok (tokens[21], Bang, "!");
    assert_tok (tokens[22], BangEq, "!=");
    assert_tok (tokens[23], Eq, "=");
    assert_tok (tokens[24], EqEq, "==");
    assert_tok (tokens[25], Lt, "<");
    assert_tok (tokens[26], LtEq, "<=");
    assert_tok (tokens[27], Gt, ">");
    assert_tok (tokens[28], GtEq, ">=");
    assert_tok (tokens[29], Plus, "+");
    assert_tok (tokens[30], PlusEq, "+=");
    assert_tok (tokens[31], Minus, "-");
    assert_tok (tokens[32], MinusEq, "-=");
    assert_tok (tokens[33], Star, "*");
    assert_tok (tokens[34], StarEq, "*=");
    assert_tok (tokens[35], Slash, "/");
    assert_tok (tokens[36], SlashEq, "/=");
    assert_tok (tokens[37], Percent, "%");
    assert_tok (tokens[38], PercentEq, "%=");
    assert_tok (tokens[39], BitAnd, "&");
    assert_tok (tokens[40], LogAnd, "&&");
    assert_tok (tokens[41], BitOr, "|");
    assert_tok (tokens[42], LogOr, "||");
    assert_tok (tokens[43], Semi, ";");
    assert_tok (tokens[44], Comma, ",");
    assert_tok (tokens[45], Dot, ".");
    assert_tok (tokens[46], LParen, "(");
    assert_tok (tokens[47], RParen, ")");
    assert_tok (tokens[48], LBrace, "{");
    assert_tok (tokens[49], RBrace, "}");
    assert_tok (tokens[50], LBracket, "[");
    assert_tok (tokens[51], RBracket, "]");
    assert_tok (tokens[52], At, "@");
    assert_tok (tokens[53], Tilde, "~");
    assert_tok (tokens[54], Question, "?");
    assert_tok (tokens[55], Colon, ":");
    assert_tok (tokens[56], Dollar, "$");
    assert_tok (tokens[57], Carret, "^");
}
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
