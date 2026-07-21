#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ForLoopFull) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_for_loop_full.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ForLoopStmt:\n"
                                  "  Cond:\n"
                                  "    BinaryExpr: <\n"
                                  "      VarExpr: i\n"
                                  "      LiteralExpr: 10\n"
                                  "  Indexator:\n"
                                  "    VarDef: priv let i: i32\n"
                                  "      LiteralExpr: 0\n"
                                  "  Iteration:\n"
                                  "    ExprStmt:\n"
                                  "      AsgnExpr: +=\n"
                                  "        VarExpr: i\n"
                                  "        LiteralExpr: 1\n"
                                  "  Body:\n"
                                  "    BreakContinue: Continue\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
