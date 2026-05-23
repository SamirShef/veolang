#pragma once
#include <basic/types/type.h>
#include <llvm/Support/Allocator.h>
#include <vector>

namespace veo::basic {

class TypePool {
    llvm::BumpPtrAllocator _allocator;
    std::vector<Type *>    _types;

public:
    template <typename T, typename... Args>
    T *
    GetOrCreate (Args &&...args) {
        void *mem = _allocator.Allocate (sizeof (T), alignof (T));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto *obj = new (mem) T (std::forward<Args> (args)...);
        for (Type *existing : _types) {
            if (*existing == *obj) {
                obj->~T ();
                return llvm::cast<T> (existing);
            }
        }
        _types.emplace_back (obj);
        return obj;
    }
};

}
