#ifndef ISLPP_PARAMSPACELIKE_H
#define ISLPP_PARAMSPACELIKE_H

#include "islpp_common.h"

namespace isl {

  template <typename D>
  class ParamSpacelike {
      typedef D SpaceTy;

    private:
      SpaceTy *getDerived() { return static_cast<D*>(this); }
      const SpaceTy *getDerived() const { return static_cast<const D*>(this); }
      SpaceTy &self() { return *static_cast<D*>(this); }
      const SpaceTy &self() const { return *static_cast<const D*>(this); }

    public:
  }; // class ParamSpacelike

} // namespace molly

#endif /* ISLPP_PARAMSPACELIKE_H */
