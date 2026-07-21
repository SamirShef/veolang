#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, IfElseIfChain) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_if_else_if_chain.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "IfElseStmt:\n"
                                  "  BinaryExpr: ==\n"
                                  "    VarExpr: x\n"
                                  "    LiteralExpr: 1\n"
                                  "  Then:\n"
                                  "    Return:\n"
                                  "      LiteralExpr: 10\n"
                                  "  Else:\n"
                                  "    IfElseStmt:\n"
                                  "      BinaryExpr: ==\n"
                                  "        VarExpr: x\n"
                                  "        LiteralExpr: 2\n"
                                  "      Then:\n"
                                  "        Return:\n"
                                  "          LiteralExpr: 20\n"
                                  "      Else:\n"
                                  "        Return:\n"
                                  "          LiteralExpr: 0\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
