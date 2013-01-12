#pragma once

#include "IslCtx.h"

#include <isl/space.h>


class IslSpace {
private:
	isl_space *space;

public:
	IslSpace(const IslCtx &ctx, unsigned nparam, unsigned n_in, unsigned n_out) {
		this->space = isl_space_alloc(ctx, nparam, n_in, n_out);
	}

#if 0
			__isl_give isl_space *isl_space_params_alloc(isl_ctx *ctx,
				unsigned nparam);
		__isl_give isl_space *isl_space_set_alloc(isl_ctx *ctx,
				unsigned nparam, unsigned dim);
		__isl_give isl_space *isl_space_copy(__isl_keep isl_space *space);
		unsigned isl_space_dim(__isl_keep isl_space *space,
				enum isl_dim_type type);

		        __isl_give isl_space *isl_space_domain(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_from_domain(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_range(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_from_range(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_params(
                __isl_take isl_space *space);
        __isl_give isl_space *isl_space_set_from_params(
                __isl_take isl_space *space);
        __isl_give isl_space *isl_space_reverse(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_join(__isl_take isl_space *left,
                __isl_take isl_space *right);
        __isl_give isl_space *isl_space_align_params(
                __isl_take isl_space *space1, __isl_take isl_space *space2)
        __isl_give isl_space *isl_space_insert_dims(__isl_take isl_space *space,
                enum isl_dim_type type, unsigned pos, unsigned n);
        __isl_give isl_space *isl_space_add_dims(__isl_take isl_space *space,
                enum isl_dim_type type, unsigned n);
        __isl_give isl_space *isl_space_drop_dims(__isl_take isl_space *space,
                enum isl_dim_type type, unsigned first, unsigned n);
        __isl_give isl_space *isl_space_move_dims(__isl_take isl_space *space,
                enum isl_dim_type dst_type, unsigned dst_pos,
                enum isl_dim_type src_type, unsigned src_pos,
                unsigned n);
        __isl_give isl_space *isl_space_map_from_set(
                __isl_take isl_space *space);
        __isl_give isl_space *isl_space_map_from_domain_and_range(
                __isl_take isl_space *domain,
                __isl_take isl_space *range);
        __isl_give isl_space *isl_space_zip(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_curry(
                __isl_take isl_space *space);
        __isl_give isl_space *isl_space_uncurry(
                __isl_take isl_space *space);



				int isl_space_is_params(__isl_keep isl_space *space);
		int isl_space_is_set(__isl_keep isl_space *space);
		int isl_space_is_map(__isl_keep isl_space *space);

				int isl_space_is_equal(__isl_keep isl_space *space1,
				__isl_keep isl_space *space2);
		int isl_space_is_domain(__isl_keep isl_space *space1,
				__isl_keep isl_space *space2);
		int isl_space_is_range(__isl_keep isl_space *space1,
				__isl_keep isl_space *space2);

		        __isl_give isl_space *isl_space_set_dim_id(
                __isl_take isl_space *space,
                enum isl_dim_type type, unsigned pos,
                __isl_take isl_id *id);
        int isl_space_has_dim_id(__isl_keep isl_space *space,
                enum isl_dim_type type, unsigned pos);
        __isl_give isl_id *isl_space_get_dim_id(
                __isl_keep isl_space *space,
                enum isl_dim_type type, unsigned pos);
        __isl_give isl_space *isl_space_set_dim_name(
                __isl_take isl_space *space,
                 enum isl_dim_type type, unsigned pos,
                 __isl_keep const char *name);
        int isl_space_has_dim_name(__isl_keep isl_space *space,
                enum isl_dim_type type, unsigned pos);
        __isl_keep const char *isl_space_get_dim_name(
                __isl_keep isl_space *space,
                enum isl_dim_type type, unsigned pos);

		        int isl_space_find_dim_by_id(__isl_keep isl_space *space,
                enum isl_dim_type type, __isl_keep isl_id *id);
        int isl_space_find_dim_by_name(__isl_keep isl_space *space,
                enum isl_dim_type type, const char *name);

		        __isl_give isl_space *isl_space_set_tuple_id(
                __isl_take isl_space *space,
                enum isl_dim_type type, __isl_take isl_id *id);
        __isl_give isl_space *isl_space_reset_tuple_id(
                __isl_take isl_space *space, enum isl_dim_type type);
        int isl_space_has_tuple_id(__isl_keep isl_space *space,
                enum isl_dim_type type);
        __isl_give isl_id *isl_space_get_tuple_id(
                __isl_keep isl_space *space, enum isl_dim_type type);
        __isl_give isl_space *isl_space_set_tuple_name(
                __isl_take isl_space *space,
                enum isl_dim_type type, const char *s);
        int isl_space_has_tuple_name(__isl_keep isl_space *space,
                enum isl_dim_type type);
        const char *isl_space_get_tuple_name(__isl_keep isl_space *space,
                enum isl_dim_type type);

		        int isl_space_is_wrapping(__isl_keep isl_space *space);
        __isl_give isl_space *isl_space_wrap(__isl_take isl_space *space);
        __isl_give isl_space *isl_space_unwrap(__isl_take isl_space *space);
#endif 

	~IslSpace() {
		isl_space_free(space);
	}

};
