#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, VarDef) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_var_def.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let x: i32\n"
                                  "  LiteralExpr: 10\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
