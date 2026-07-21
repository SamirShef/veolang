#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, UnaryOperations) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_unary_operations.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let inverted\n"
                                  "  UnaryExpr: !\n"
                                  "    VarExpr: flag\n"
                                  "\n"
                                  "VarDef: priv let negative\n"
                                  "  UnaryExpr: -\n"
                                  "    VarExpr: count\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
