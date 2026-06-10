#include <basic/symbols/module.h>
#include <basic/symbols/struct.h>

namespace veo::symbols {

bool
Struct::operator== (const Struct &other) const {
    if (this == &other) {
        return true;
    }

    return Name.Val == other.Name.Val && Fields == other.Fields
           && Methods == other.Methods && TraitsImplements == other.TraitsImplements
           && *Parent == *other.Parent;
}

}
