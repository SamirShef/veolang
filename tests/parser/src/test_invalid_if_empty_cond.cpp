#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, IfEmptyCond) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_invalid_if_empty_cond.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, true);
    const auto &expectedSnaphot = "Return:\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    assert_eq (context.Diag.Builders ().size (), 3);
    assert_diag (
        context.Diag.Builders ()[0],
        diagnostic::DiagCode::EExpectedExpr,
        "expected expression, found '{'",
        diagnostic::Severity::Error);
    assert_diag (
        context.Diag.Builders ()[1],
        diagnostic::DiagCode::EUnexpectedToken,
        "expected '{', found 'return'",
        diagnostic::Severity::Error);
    assert_diag (
        context.Diag.Builders ()[2],
        diagnostic::DiagCode::EExpectedStmt,
        "expected statement, found '}'",
        diagnostic::Severity::Error);
    return true;
}
