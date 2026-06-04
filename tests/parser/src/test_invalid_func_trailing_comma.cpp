#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FuncTrailingComma) {
    ParserContext context;
    auto          res = context.ParseFile ("veo/test_invalid_func_trailing_comma.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, true);
    const auto &expectedSnaphot = "FuncDef: priv test (a: i32): noth\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    assert_eq (context.Diag.Builders ().size (), 1);
    assert_diag (
        context.Diag.Builders ()[0],
        diagnostic::DiagCode::EUnexpectedToken,
        "expected identifier, found ')'",
        diagnostic::Severity::Error);
    return true;
}
