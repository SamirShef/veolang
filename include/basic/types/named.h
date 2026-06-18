#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <sstream>
#include <vector>

namespace veo::basic {

class NamedType : public Type {
    std::vector<NameObj> _path;
    std::vector<Type *>  _genericParams;

public:
    explicit NamedType (std::vector<NameObj> path, std::vector<Type *> genericParams = {})
        : _path (std::move (path)),
          _genericParams (std::move (genericParams)),
          Type (TypeKind::Named) {}

    type_classof (Named);

    std::vector<NameObj>
    Path () const {
        return _path;
    }

    const std::vector<Type *> &
    GenericParams () const {
        return _genericParams;
    }

    bool
    IsGeneric () const {
        return !_genericParams.empty ();
    }

    std::string
    ToString () const override {
        std::ostringstream oss;
        size_t             index = 0;
        for (const auto &name : _path) {
            if (index != 0) {
                oss << ".";
            }
            oss << name.Val;
            ++index;
        }
        if (IsGeneric ()) {
            oss << "<";
            size_t i = 0;
            for (const auto &param : _genericParams) {
                if (i != 0) {
                    oss << ", ";
                }
                oss << param->ToString ();
            }
            oss << ">";
        }
        return oss.str ();
    }
};

}
