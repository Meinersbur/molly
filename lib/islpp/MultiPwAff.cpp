#include "islpp/MultiPwAff.h"

using namespace isl;

#define ISLPP_EL pw_aff
#define ISLPP_ELPP PwAff
#include "Multi_impl.inc.h"
#undef ISLPP_EL
#undef ISLPP_ELPP
