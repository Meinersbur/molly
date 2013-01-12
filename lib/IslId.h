#pragma once

#include "IslCtx.h"

#include <isl/id.h>

#include <assert.h>



class IslId {
private:
	isl_id *id;

public:
	IslId(IslCtx &ctx, const char *name, void *user) {
		assert(name);
		assert(user);
		this->id = isl_id_alloc(ctx, name, user);
	}

#if 0
	        __isl_give isl_id *isl_id_set_free_user(
                __isl_take isl_id *id,
                __isl_give void (*free_user)(void *user));
        __isl_give isl_id *isl_id_copy(isl_id *id);
   
        isl_ctx *isl_id_get_ctx(__isl_keep isl_id *id);
        void *isl_id_get_user(__isl_keep isl_id *id);
        __isl_keep const char *isl_id_get_name(__isl_keep isl_id *id);
        __isl_give isl_printer *isl_printer_print_id(
                __isl_take isl_printer *p, __isl_keep isl_id *id);
#endif


	~IslId() {
		isl_id_free(id);
	}
};
