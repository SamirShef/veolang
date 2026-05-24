#pragma once
#include <basic/types/pool.h>
#include <llvm/Support/Allocator.h>
#include <ranges>

namespace veo::ast {

class Context {
    llvm::BumpPtrAllocator _allocator;

    struct CleanupNode {
        void (*Destroy) (void *);
        void *Obj;
    };

    std::vector<CleanupNode> _cleanups;

public:
    Context () = default;

    Context (const Context &) = delete;

    ~Context () {
        for (auto &cleanup : std::views::reverse (_cleanups)) {
            cleanup.Destroy (cleanup.Obj);
        }
    }

    Context &
    operator= (const Context &) = delete;

    template <typename T, typename... Args>
    T *
    CreateNode (Args &&...args) {
        void *mem = _allocator.Allocate (sizeof (T), alignof (T));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto *obj = new (mem) T (std::forward<Args> (args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            _cleanups.emplace_back (
                [] (void *ptr) { static_cast<T *> (ptr)->~T (); },
                obj);
        }
        return obj;
    }

    template <typename T>
    T *
    AllocateArray (size_t count) {
        return static_cast<T *> (_allocator.Allocate (sizeof (T) * count, alignof (T)));
    }
};

}
