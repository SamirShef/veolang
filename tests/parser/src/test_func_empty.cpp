#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FuncEmpty) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_func_empty.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "FuncDef: priv foo ()\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
