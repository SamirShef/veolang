#pragma once
#include <hir/builder.h>
#include <hir/context.h>

namespace veo {

class HIRLineanizer {
    hir::Context &_ctx;     // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    hir::Builder &_builder; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

public:
    HIRLineanizer (hir::Context &ctx, hir::Builder &builder)
        : _ctx (ctx), _builder (builder) {}
};
}
