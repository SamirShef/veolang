#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, TestTernary) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_ternary.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let x\n"
                                  "  TernaryExpr:\n"
                                  "    Cond:\n"
                                  "      BinaryExpr: ==\n"
                                  "        LiteralExpr: 10\n"
                                  "        BinaryExpr: +\n"
                                  "          LiteralExpr: 1\n"
                                  "          LiteralExpr: 9\n"
                                  "    TrueVal:\n"
                                  "      LiteralExpr: 2\n"
                                  "    FalseVal:\n"
                                  "      LiteralExpr: 20\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
