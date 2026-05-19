#pragma once
#include <hir/func.h>
#include <hir/struct_def.h>
#include <hir/var_def.h>
#include <llvm/Support/Allocator.h>
#include <vector>

namespace veo::hir {

class Context {
    llvm::BumpPtrAllocator   _allocator;
    std::vector<Function *>  _funcs;
    std::vector<VarDef *>    _globals;
    std::vector<StructDef *> _structs;

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

    std::vector<StructDef *> &
    Structs () {
        return _structs;
    }

    void
    AddFunction (Function *func) {
        _funcs.emplace_back (func);
    }

    void
    AddGlobal (VarDef *var) {
        _globals.emplace_back (var);
    }

    void
    AddStruct (StructDef *s) {
        _structs.emplace_back (s);
    }
};

}
