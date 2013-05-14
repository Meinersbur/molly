
#define ISLPP_STRUCT NAMEUS(isl,multi,ISLPP_EL)

extern "C" {
__isl_give ISLPP_STRUCT *NAMEUS(isl_multi,ISLPP_EL,set_tuple_id)(__isl_take ISLPP_STRUCT *map, enum isl_dim_type type, __isl_take isl_id *id);
__isl_give ISLPP_STRUCT *NAMEUS(isl_multi,ISLPP_EL,drop_dims)(__isl_take ISLPP_STRUCT *aff, enum isl_dim_type type, unsigned first, unsigned n);
}

#undef ISLPP_STRUCT
