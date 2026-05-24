#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, StructMissingBrace) {
    ParserContext context;
    auto          res = context.ParseFile ("veo/test_invalid_struct_missing_brace.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, true);
    const auto &expectedSnaphot = "";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    assert_eq (context.Diag.Builders ().size (), 3);
    assert_diag (
        context.Diag.Builders ()[0],
        diagnostic::DiagCode::EUnexpectedToken,
        "expected '{', found 'pub'",
        diagnostic::Severity::Error);
    assert_diag (
        context.Diag.Builders ()[1],
        diagnostic::DiagCode::EUnexpectedToken,
        "expected ';', found ':'",
        diagnostic::Severity::Error);
    assert_diag (
        context.Diag.Builders ()[2],
        diagnostic::DiagCode::EExpectedStmt,
        "expected statement, found '}'",
        diagnostic::Severity::Error);
    return true;
}
