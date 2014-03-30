#ifndef ISLPP_SETSPACELIKE_H
#define ISLPP_SETSPACELIKE_H

#include "islpp_common.h"

namespace isl {

  template <typename D>
  class SetSpacelike {
      typedef D SpaceTy;

    private:
      SpaceTy *getDerived() { return static_cast<D*>(this); }
      const SpaceTy *getDerived() const { return static_cast<const D*>(this); }
      SpaceTy &self() { return *static_cast<D*>(this); }
      const SpaceTy &self() const { return *static_cast<const D*>(this); }

    public:
    }; // class SetSpacelike

  } // namespace molly

#endif /* ISLPP_SETSPACELIKE_H */
