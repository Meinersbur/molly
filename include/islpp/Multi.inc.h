#ifndef ISLPP_EL
#define ISLPP_EL int
#define ISLPP_ELPP Int
#endif






#include <cassert>
#include <isl/aff.h>
#include <isl/multi.h>

#include "islpp/Multi.h"
#include "islpp/Aff.h"
#include "islpp/Space.h"
#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Vec.h"
#include "islpp/Int.h"
#include "islpp/Set.h"
#include "islpp/LocalSpace.h"
#include "islpp/Spacelike.h"

#include <llvm/Support/ErrorHandling.h>

struct ISLPP_STRUCT;

namespace llvm {
} // namespace llvm

namespace isl {
  class Aff;
} // namespace isl

// Forgotten declarations
//__isl_give isl_multi_aff *isl_multi_aff_alloc(__isl_take isl_set *set, __isl_take isl_multi_aff *maff);


namespace isl {
  template<>
  class Multi<ISLPP_ELPP> : public Spacelike {
#include "Multi_members.inc.h"
  }; 

#include "Multi_func.inc.h"

} // namespace isl

#undef ISLPP_STRUCT
