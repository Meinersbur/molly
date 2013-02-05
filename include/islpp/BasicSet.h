#ifndef ISLPP_BASICSET_H
#define ISLPP_BASICSET_H

#include <llvm/Support/Compiler.h>
#include <cassert>

#include <isl/space.h>

struct isl_basic_set;


namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {
  class Constraint;
  class Space;
  class Set;
}


namespace isl {
  class BasicSet {
    friend class Set;
    friend BasicSet params(BasicSet &&params);


  private:
    isl_basic_set *set;

  protected:
        explicit BasicSet(isl_basic_set *set) { this->set = set; }

        isl_basic_set *take() { assert(set); isl_basic_set *result = set; set = nullptr; return result; }
        isl_basic_set *takeCopy() const;
        isl_basic_set *keep() const { return set; }
        void give(isl_basic_set *set);

  public:
    BasicSet(void) : set(NULL) { }
    BasicSet(const BasicSet &that) : set(that.takeCopy()) {}
    BasicSet(BasicSet &&that) : set(that.take()) { }
    ~BasicSet(void);

    const BasicSet &operator=(const BasicSet &that) { give(that.takeCopy()); return *this; }
    const BasicSet &operator=(BasicSet &&that) { give(that.take()); return *this; }

    static BasicSet wrap(isl_basic_set *set) { return BasicSet(set); }
    static BasicSet create(const Space &space);
    static BasicSet create(Space &&space);
    static BasicSet createEmpty(const Space &space);
    static BasicSet createEmpty(Space &&space);
    static BasicSet createUniverse(const Space &space);
    static BasicSet createUniverse(Space &&space);

    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;

    void addConstraint(Constraint &&constraint);
    void projectOut(isl_dim_type type, unsigned first, unsigned n);
    BasicSet params();

  }; // class BasicSet


  BasicSet params(BasicSet &&params);

} // namespace isl
#endif /* ISLPP_BASICSET_H */