#include <basic/symbols/module.h>
#include <basic/types/struct_type.h>
#include <sstream>

namespace veo::basic {

std::string
StructType::ToString () const {
    std::ostringstream oss;
    if (_base->Parent != nullptr) {
        oss << _base->Parent->ToString () << '.';
    }
    oss << _base->Name.Val;
    return oss.str ();
}

}
