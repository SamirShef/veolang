#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, VarNoInit) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_var_no_init.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let buffer_size: u64\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
