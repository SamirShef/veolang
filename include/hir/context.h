#pragma once
#include <hir/func.h>
#include <hir/var_def.h>
#include <llvm/Support/Allocator.h>
#include <vector>

namespace veo::hir {

class Context {
    llvm::BumpPtrAllocator  _allocator;
    std::vector<Function *> _funcs;
    std::vector<VarDef *>   _globals;

public:
    Context ()                = default;
    ~Context ()               = default;
    Context (Context &&)      = delete;
    Context (const Context &) = delete;

    Context &
    operator= (Context &&) = delete;

    Context &
    operator= (const Context &) = delete;

    template <typename T, typename... Args>
    T *
    CreateNode (Args &&...args) {
        void *mem = _allocator.Allocate (sizeof (T), alignof (T));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto *obj = new (mem) T (std::forward<Args> (args)...);
        return obj;
    }

    std::vector<Function *> &
    Functions () {
        return _funcs;
    }

    std::vector<VarDef *> &
    Globals () {
        return _globals;
    }

    void
    AddFunction (Function *func) {
        _funcs.push_back (func);
    }

    void
    AddGlobal (VarDef *var) {
        _globals.push_back (var);
    }
};

}
