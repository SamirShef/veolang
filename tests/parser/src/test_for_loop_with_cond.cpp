#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ForLoopWithCond) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_for_loop_with_cond.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ForLoopStmt:\n"
                                  "  Cond:\n"
                                  "    BinaryExpr: <\n"
                                  "      VarExpr: x\n"
                                  "      LiteralExpr: 100\n"
                                  "  Body:\n"
                                  "    ExprStmt:\n"
                                  "      AsgnExpr: +=\n"
                                  "        VarExpr: x\n"
                                  "        LiteralExpr: 1\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
