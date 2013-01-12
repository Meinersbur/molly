#pragma once

#include <isl/ctx.h>

class IslCtx {
private:
	isl_ctx *ctx;

public:
	IslCtx() {
		ctx = isl_ctx_alloc();
	}

	//int isl_options_set_on_error(isl_ctx *ctx, int val); // ISL_ON_ERROR_WARN, ISL_ON_ERROR_CONTINUE and ISL_ON_ERROR_ABORT
	// int isl_options_get_on_error(isl_ctx *ctx);
	//enum isl_error isl_ctx_last_error(isl_ctx *ctx);
	//void isl_ctx_reset_error(isl_ctx *ctx);

	~IslCtx() {
		isl_ctx_free(ctx);
	}
};
