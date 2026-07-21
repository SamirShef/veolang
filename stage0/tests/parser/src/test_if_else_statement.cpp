#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, IfElseStatement) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_if_else_statement.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "IfElseStmt:\n"
                                  "  BinaryExpr: >\n"
                                  "    VarExpr: x\n"
                                  "    VarExpr: y\n"
                                  "  Then:\n"
                                  "    Return:\n"
                                  "      VarExpr: x\n"
                                  "  Else:\n"
                                  "    Return:\n"
                                  "      VarExpr: y\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
