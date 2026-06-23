#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, StructDeclaration) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_struct_declaration.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "StructDef: priv Point\n"
                                  "  Field: pub x: f64\n"
                                  "  Field: pub y: f64\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
