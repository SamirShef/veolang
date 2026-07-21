#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <sstream>
#include <vector>

namespace veo::basic {

class NamedType : public Type {
    std::vector<NameObj> _path;

public:
    explicit NamedType (std::vector<NameObj> path)
        : _path (std::move (path)), Type (TypeKind::Named) {}

    type_classof (Named);

    std::vector<NameObj>
    Path () const {
        return _path;
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
        return oss.str ();
    }
};

}
