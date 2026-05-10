#pragma once
#include <hir/node.h>
#include <string>
#include <vector>

namespace veo::hir {

class Function;

class BasicBlock {
    std::string         _name;
    std::vector<Node *> _nodes;
    Node               *_terminator = nullptr;
    Function           *_parent     = nullptr;

public:
    BasicBlock () = default;
    explicit BasicBlock (Function *parent, std::string name = "")
        : _name (std::move (name)), _parent (parent) {}

    void
    AddInst (Node *node) {
        if (_terminator != nullptr) {
            return;
        }
        if (node->IsTerminator ()) {
            _terminator = node;
        } else {
            _nodes.push_back (node);
        }
    }

    std::string
    Name () const {
        return _name;
    }

    std::vector<Node *> &
    Body () {
        return _nodes;
    }

    Node *
    Terminator () const {
        return _terminator;
    }

    Function *
    Parent () const {
        return _parent;
    }

    void
    SetParent (Function *parent) {
        _parent = parent;
    }
};

}
