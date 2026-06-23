#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, IfStatement) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_if_statement.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "IfElseStmt:\n"
                                  "  VarExpr: isValid\n"
                                  "  Then:\n"
                                  "    VarDef: priv let x\n"
                                  "      LiteralExpr: 1\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
