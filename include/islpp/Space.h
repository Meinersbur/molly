#ifndef ISLPP_SPACE_H
#define ISLPP_SPACE_H

#include <llvm/Support/Compiler.h>
#include <assert.h>

struct isl_space;

namespace isl {
} // namespace isl


namespace isl {
  /// Whenever a new set, relation or similiar object is created from scratch, the space in which it lives needs to be specified using an isl_space. Each space involves zero or more parameters and zero, one or two tuples of set or input/output dimensions. The parameters and dimensions are identified by an isl_dim_type and a position. The type isl_dim_param refers to parameters, the type isl_dim_set refers to set dimensions (for spaces with a single tuple of dimensions) and the types isl_dim_in and isl_dim_out refer to input and output dimensions (for spaces with two tuples of dimensions). Local spaces (see Local Spaces) also contain dimensions of type isl_dim_div. Note that parameters are only identified by their position within a given object. Across different objects, parameters are (usually) identified by their names or identifiers. Only unnamed parameters are identified by their positions across objects. The use of unnamed parameters is discouraged.
  class Space {
    friend class Ctx;
    friend class LocalSpace;
    friend class BasicSet;

  private:
    isl_space *space;

    explicit Space(isl_space *space);

  public:
    isl_space *take() { assert(space); isl_space *result = space; space = nullptr; return result; }
    isl_space *keep() const { return space; }
    void give(isl_space *set) { assert(!this->space); this->space = space; }

  public:
    Space(Space &&that) { this->space = that.take(); }
    ~Space();

    static Space wrap(isl_space *space) { return Space(space); }

    /* implicit */ Space(const Space &);
    const Space &operator=(const Space &);

    Space copy() const;

  }; // class Sapce
} // namespace molly
#endif /* ISLPP_SPACE_H */
