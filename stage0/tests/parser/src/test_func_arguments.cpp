#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FuncArguments) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_func_arguments.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "FuncDef: priv sum (a: i32, b: i32): i32\n"
                                  "  Return:\n"
                                  "    BinaryExpr: +\n"
                                  "      VarExpr: a\n"
                                  "      VarExpr: b\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
