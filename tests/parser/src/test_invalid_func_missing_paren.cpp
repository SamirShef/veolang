#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FuncMissingParen) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_invalid_func_missing_paren.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, true);
    const auto &expectedSnaphot = "Return:\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    assert_eq (context.Diag.Builders ().size (), 2);
    assert_diag (
        context.Diag.Builders ()[0],
        diagnostic::DiagCode::EUnexpectedToken,
        "expected '(', found '{'",
        diagnostic::Severity::Error);
    assert_diag (
        context.Diag.Builders ()[1],
        diagnostic::DiagCode::EExpectedStmt,
        "expected statement, found '}'",
        diagnostic::Severity::Error);
    return true;
}
