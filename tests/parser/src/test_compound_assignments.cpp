#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, CompoundAssignments) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_compound_assignments.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ExprStmt:\n"
                                  "  AsgnExpr: +=\n"
                                  "    VarExpr: score\n"
                                  "    LiteralExpr: 10\n"
                                  "\n"
                                  "ExprStmt:\n"
                                  "  AsgnExpr: *=\n"
                                  "    VarExpr: factor\n"
                                  "    LiteralExpr: 2\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
