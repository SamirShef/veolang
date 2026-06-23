#include <basic/symbols/module.h>
#include <basic/symbols/variable.h>

namespace veo::symbols {

bool
Variable::operator== (const Variable &other) const {
    if (this == &other) {
        return true;
    }

    return Name.Val == other.Name.Val && *Type == *other.Type && Val == other.Val
           && IsConst == other.IsConst && IsGlobal == other.IsGlobal
           && *Parent == *other.Parent && MangleKind == other.MangleKind;
}

}
