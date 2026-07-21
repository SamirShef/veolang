#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, StructWithStaticConst) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_struct_with_static_const.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "StructDef: priv Limits\n"
                                  "  Field: pub static const MIN: i32\n"
                                  "  Field: pub static const MAX: i32\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
