#include <basic/symbols/module.h>
#include <basic/symbols/struct.h>

namespace veo::symbols {

bool
Struct::operator== (const Struct &other) const {
    return Name.Val == other.Name.Val && Fields == other.Fields
           && *Parent == *other.Parent;
}

}
