#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FieldAccessAndAssignment) {
    ParserContext context;
    auto          res = context.ParseFile ("veo/test_field_access_and_assignment.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ExprStmt:\n"
                                  "  AsgnExpr: =\n"
                                  "    FieldExpr: x\n"
                                  "      From:\n"
                                  "        FieldExpr: position\n"
                                  "          From:\n"
                                  "            VarExpr: player\n"
                                  "    LiteralExpr: 5.0\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
