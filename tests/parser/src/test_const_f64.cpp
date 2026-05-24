#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ConstF64) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_const_f64.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv const pi: f64\n"
                                  "  LiteralExpr: 3.14159\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
