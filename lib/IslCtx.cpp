
#include "IslCtx.h"

#include "IslSpace.h"

#include <isl/ctx.h>
#include <isl/space.h>

using namespace molly;


    IslCtx::IslCtx() {
      ctx = isl_ctx_alloc();
    }

        IslCtx::~IslCtx() {
      isl_ctx_free(ctx);
    }

    IslSpace IslCtx::createSpace(unsigned nparam, unsigned dim) {
      isl_space *space = isl_space_set_alloc(ctx, nparam, dim);
      return IslSpace::wrap(space);
    }
