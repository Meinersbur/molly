#ifndef ISLPP_LOCALSPACE_H
#define ISLPP_LOCALSPACE_H

#include <llvm/Support/Compiler.h>

#include <cassert>

struct isl_local_space;

namespace isl {
  class Space;
}


namespace isl {
  /// A local space is essentially a space with zero or more existentially quantified variables.
  class LocalSpace {
    friend class Constraint;

  private:
    isl_local_space *space;

  protected:
    explicit LocalSpace(isl_local_space *);

    static LocalSpace wrap(isl_local_space *space) { return LocalSpace(space); }

    isl_local_space *take() { assert(space); isl_local_space *result = space; space = nullptr; return result; }
    isl_local_space *keep() { return space; }
    void give(isl_local_space *set) { assert(!this->space); this->space = space; }

  public:
    ~LocalSpace();

    /* implicit */ LocalSpace(const LocalSpace &);
    /* implicit */ LocalSpace(LocalSpace &&that) { this->space = that.take(); }
    const LocalSpace &operator=(const LocalSpace &);
    const LocalSpace &operator=(LocalSpace &&that) { assert(!this->space); this->space = that.take(); return *this; }

    /* implicit */ LocalSpace(const Space &);
    /* implicit */ LocalSpace(Space &&);
    const LocalSpace &operator=(const Space &that);
    const LocalSpace &operator=(Space &&);

    LocalSpace clone();
  };

} // namepsace isl
#endif /* ISLPP_LOCALSPACE_H */