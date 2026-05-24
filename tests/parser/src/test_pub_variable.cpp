#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, PubVariable) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_pub_variable.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: pub const MAX_CONNECTIONS: u32\n"
                                  "  LiteralExpr: 1024\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
