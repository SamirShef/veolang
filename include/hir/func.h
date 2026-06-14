#pragma once
#include <ast/stmts/func_def.h>
#include <basic/name.h>
#include <basic/symbols/function.h>
#include <basic/types/type.h>
#include <hir/basic_block.h>
#include <hir/mangle_kind.h>
#include <hir/node.h>
#include <hir/var_def.h>

namespace veo::hir {

class Function : public Node {
    basic::NameObj            _name;
    basic::Type              *_retType;
    std::vector<VarDef *>     _args;
    std::vector<BasicBlock *> _body;
    symbols::Function        *_base;
    MangleKind                _mangleKind;
    basic::Type              *_methodBaseType;
    bool                      _isStatic;

public:
    Function (
        basic::NameObj        name,
        basic::Type          *retType,
        std::vector<VarDef *> args,
        llvm::SMLoc           start,
        llvm::SMLoc           end,
        symbols::Function    *base,
        MangleKind            mangleKind     = MangleKind::Veo,
        basic::Type          *methodBaseType = nullptr,
        bool                  isStatic       = false)
        : _name (std::move (name)),
          _retType (retType),
          _args (std::move (args)),
          _base (base),
          _mangleKind (mangleKind),
          _methodBaseType (methodBaseType),
          _isStatic (isStatic),
          Node (NodeKind::Func, start, end) {}

    hir_classof (Func);

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *
    RetType () const {
        return _retType;
    }

    std::vector<VarDef *> &
    Args () {
        return _args;
    }

    std::vector<BasicBlock *> &
    Body () {
        return _body;
    }

    symbols::Function *
    BaseSymbol () const {
        return _base;
    }

    MangleKind
    GetMangleKind () const {
        return _mangleKind;
    }

    basic::Type *
    MethodBaseType () const {
        return _methodBaseType;
    }

    bool
    IsStatic () const {
        return _isStatic;
    }
};

}
