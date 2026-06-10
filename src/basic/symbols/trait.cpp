#include <basic/symbols/module.h>
#include <basic/symbols/trait.h>

namespace veo::symbols {

bool
Trait::operator== (const Trait &other) const {
    if (this == &other) {
        return true;
    }

    return Name.Val == other.Name.Val && Methods == other.Methods
           && *Parent == *other.Parent;
}

}
