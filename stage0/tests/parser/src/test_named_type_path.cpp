#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, NamedTypePath) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_named_type_path.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let matrix: std.math.Matrix\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
