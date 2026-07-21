#include <basic/symbols/function.h>
#include <basic/symbols/module.h>
namespace veo::symbols {

bool
Function::operator== (const Function &other) const {
    if (this == &other) {
        return true;
    }

    return Name.Val == other.Name.Val && *RetType == *other.RetType && Args == other.Args
           && IsGeneric == other.IsGeneric
           && IsExternDeclaration == other.IsExternDeclaration && *Parent == *other.Parent
           && Access == other.Access && MangleKind == other.MangleKind;
}

}
