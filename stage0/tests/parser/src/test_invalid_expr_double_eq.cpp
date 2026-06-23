#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ExprDoubleEq) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_invalid_expr_double_eq.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, true);
    const auto &expectedSnaphot = "";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    assert_eq (context.Diag.Builders ().size (), 1);
    assert_diag (
        context.Diag.Builders ()[0],
        diagnostic::DiagCode::EExpectedExpr,
        "expected expression, found '='",
        diagnostic::Severity::Error);
    return true;
}
