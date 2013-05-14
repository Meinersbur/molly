#ifndef ISLPP_MULTIPWAFF_H
#define ISLPP_MULTIPWAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include <cassert>
#include <llvm/Support/ErrorHandling.h>

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
#include "PwAff.h"


#define ISLPP_EL pw_aff
#define ISLPP_ELPP PwAff

#include "Multi_decls.inc.h"

namespace isl {
  template<>
  class Multi<PwAff> : public Spacelike {
#include "Multi_members.inc.h"
  }; // class PwAff

#include "Multi_funcs.inc.h"

} // namespace isl

#undef ISLPP_EL
#undef ISLPP_ELPP



#if 0
struct isl_multi_pw_aff;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  template<>
  class Multi<PwAff> {
#pragma region Low-level
  private:
    isl_multi_pw_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_multi_pw_aff *take() { assert(aff); isl_multi_pw_aff *result = aff; aff = nullptr; return result; }
    isl_multi_pw_aff *takeCopy() const;
    isl_multi_pw_aff *keep() const { return aff; }
  protected:
    void give(isl_multi_pw_aff *aff);

    explicit Multi(isl_multi_pw_aff *aff) : aff(aff) { }
  public:
    static MultiPwAff wrap(isl_multi_pw_aff *aff) { return MultiPwAff(aff); }
#pragma endregion

  public:
    Multi(void) : aff(nullptr) {}
    /* implicit */ Multi(const MultiPwAff &that) : aff(that.takeCopy()) {}
    /* implicit */ Multi(MultiPwAff &&that) : aff(that.take()) { }
    ~Multi(void);

    const MultiPwAff &operator=(const MultiPwAff &that) { give(that.takeCopy()); return *this; }
    const MultiPwAff &operator=(MultiPwAff &&that) { give(that.take()); return *this; }

  }; // class MultiPwAff
} // namespace isl
#endif

#endif /* ISLPP_MULTIPWAFF_H */
