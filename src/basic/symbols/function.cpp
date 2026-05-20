#include <basic/symbols/function.h>
#include <basic/symbols/module.h>
namespace veo::symbols {

bool
Function::operator== (const Function &other) const {
    return Name.Val == other.Name.Val && *RetType == *other.RetType && Args == other.Args
           && *Parent == *other.Parent;
}

}
