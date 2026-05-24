#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, VarDeduction) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_var_deduction.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let status\n"
                                  "  LiteralExpr: true\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
