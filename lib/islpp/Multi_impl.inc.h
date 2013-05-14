
#define ISLPP_STRUCT NAMEUS(isl,multi,ISLPP_EL)

static Multi<ISLPP_ELPP> Multi<ISLPP_ELPP>::CONCAT(from,ISLPP_ELPP)(ISLPP_ELPP &&el) { 
  return wrap(NAMEUS(isl_multi,ISLPP_EL,from,ISLPP_EL)(el.take())); 
}

#undef ISLPP_STRUCT
