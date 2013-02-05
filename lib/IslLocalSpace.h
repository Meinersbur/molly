#pragma once

#include <isl/local_space.h>

class IslLocalSpace {
private:
  isl_local_space *space;

public:
  IslLocalSpace() {
  }

  ~IslLocalSpace() {
  }

#if 0
   #include <isl/local_space.h>
        __isl_give isl_local_space *isl_local_space_from_space(
                __isl_take isl_space *space);

            isl_ctx *isl_local_space_get_ctx(
                __isl_keep isl_local_space *ls);
        int isl_local_space_is_set(__isl_keep isl_local_space *ls);
        int isl_local_space_dim(__isl_keep isl_local_space *ls,
                enum isl_dim_type type);
        int isl_local_space_has_dim_id(
                __isl_keep isl_local_space *ls,
                enum isl_dim_type type, unsigned pos);
        __isl_give isl_id *isl_local_space_get_dim_id(
                __isl_keep isl_local_space *ls,
                enum isl_dim_type type, unsigned pos);
        int isl_local_space_has_dim_name(
                __isl_keep isl_local_space *ls,
                enum isl_dim_type type, unsigned pos)
        const char *isl_local_space_get_dim_name(
                __isl_keep isl_local_space *ls,
                enum isl_dim_type type, unsigned pos);
        __isl_give isl_local_space *isl_local_space_set_dim_name(
                __isl_take isl_local_space *ls,
                enum isl_dim_type type, unsigned pos, const char *s);
        __isl_give isl_local_space *isl_local_space_set_dim_id(
                __isl_take isl_local_space *ls,
                enum isl_dim_type type, unsigned pos,
                __isl_take isl_id *id);
        __isl_give isl_space *isl_local_space_get_space(
                __isl_keep isl_local_space *ls);
        __isl_give isl_aff *isl_local_space_get_div(
                __isl_keep isl_local_space *ls, int pos);
        __isl_give isl_local_space *isl_local_space_copy(
                __isl_keep isl_local_space *ls);
        void *isl_local_space_free(__isl_take isl_local_space *ls);

     int isl_local_space_is_equal(__isl_keep isl_local_space *ls1,
                __isl_keep isl_local_space *ls2);

             __isl_give isl_local_space *isl_local_space_domain(
                __isl_take isl_local_space *ls);
        __isl_give isl_local_space *isl_local_space_range(
                __isl_take isl_local_space *ls);
        __isl_give isl_local_space *isl_local_space_from_domain(
                __isl_take isl_local_space *ls);
        __isl_give isl_local_space *isl_local_space_intersect(
                __isl_take isl_local_space *ls1,
                __isl_take isl_local_space *ls2);
        __isl_give isl_local_space *isl_local_space_add_dims(
                __isl_take isl_local_space *ls,
                enum isl_dim_type type, unsigned n);
        __isl_give isl_local_space *isl_local_space_insert_dims(
                __isl_take isl_local_space *ls,
                enum isl_dim_type type, unsigned first, unsigned n);
        __isl_give isl_local_space *isl_local_space_drop_dims(
                __isl_take isl_local_space *ls,
                enum isl_dim_type type, unsigned first, unsigned n);
#endif
};
