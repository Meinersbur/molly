#ifndef ISLPP_BASICSET_H
#define ISLPP_BASICSET_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include "Spacelike.h"
#include <llvm/Support/Compiler.h>
#include <cassert>
#include <string>
#include <functional>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/set.h>
#include <llvm/Support/ErrorHandling.h>
#include "Ctx.h"
#include "Id.h"

struct isl_basic_set;
struct isl_constraint;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Constraint;
  class Space;
  class Set;
  class LocalSpace;
  class Ctx;
  class Mat;
  class Id;
  class Int;
  class BasicMap;
  class Point;
  class Vertices;
} // namespace isl


namespace isl {
  typedef int (*ConstraintCallback)(isl_constraint *c, void *user);

  class BasicSet : public Spacelike2<BasicSet> {
#pragma region Low-level
  private:
    isl_basic_set *set;

  public: // Public because otherwise we had to add a lot of friends
    isl_basic_set *take() { assert(set); isl_basic_set *result = set; set = nullptr; return result; }
    isl_basic_set *takeCopy() const;
    isl_basic_set *keep() const { return set; }
  protected:
    void give(isl_basic_set *set);

  public:
    //TODO: Find better name, 'wrap' means something else for isl
    static BasicSet wrap(isl_basic_set *set) { BasicSet result; result.give(set); return result; }
#pragma endregion


  public:
    BasicSet(void) : set(nullptr) { }
    BasicSet(const BasicSet &that) : set(that.takeCopy()) {}
    BasicSet(BasicSet &&that) : set(that.take()) { }
    ~BasicSet(void);

    const BasicSet &operator=(const BasicSet &that) { give(that.takeCopy()); return *this; }
    const BasicSet &operator=(BasicSet &&that) { give(that.take()); return *this; }


#pragma region Creational
    static BasicSet create(const Space &space);
    static BasicSet create(Space &&space);
    static BasicSet createEmpty(const Space &space);
    static BasicSet createEmpty(Space &&space);
    static BasicSet createUniverse(const Space &space);
    static BasicSet createUniverse(Space &&space);
    static BasicSet createNatUniverse(Space &&space);
    static BasicSet createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4);
    static BasicSet createFromPoint(Point &&pnt);
    static BasicSet createBoxFromPoints(Point &&pnt1, Point &&pnt2);
    static BasicSet createFromBasicMap(BasicMap &&bmap);
    
    static BasicSet readFromFile(Ctx *ctx, FILE *input);
    static BasicSet readFromStr(Ctx *ctx, const char *str);

    BasicSet copy() const { return wrap(isl_basic_set_copy(keep())); }
    BasicSet &&move() { return std::move(*this); }
#pragma endregion


#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
#pragma endregion


    Ctx *getCtx() const { return Ctx::wrap(isl_basic_set_get_ctx(keep())); }


#pragma region Spacelike2
    unsigned dim(isl_dim_type type) const { return isl_basic_set_dim(keep(), type); }

    //bool hasTupleId(isl_dim_type type) const { return isl_basic_set_has_tuple_id(keep(), type); }
    Id getTupleId(isl_dim_type type) const { llvm_unreachable("Missing API function"); }
    void setTupleId_inplace(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER{ llvm_unreachable("Missing API function"); }
    //bool hasTupleName(isl_dim_type type) const { return isl_basic_set_has_tuple_name(keep(), type); }
    const char *getTupleName(isl_dim_type type=isl_dim_set) const { assert(type==isl_dim_set); return isl_basic_set_get_tuple_name(keep());  }
   // const char *getTupleName() const { return isl_basic_set_get_tuple_name(keep()); }
     void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { assert(type==isl_dim_set); give(isl_basic_set_set_tuple_name(take(), s)); }
     void setTupleName_inplace(const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_set_tuple_name(take(), s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_basic_set_has_dim_id(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::wrap(isl_basic_set_get_dim_id(keep(), type, pos)); }
    void setDimId_inplace(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { llvm_unreachable("Missing API function"); }
    //bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_basic_set_has_dim_name(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_basic_set_get_dim_name(keep(), type, pos); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_set_dim_name(take(), type, pos, s)); }

    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned n) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_insert_dims(take(), type, pos, n)); }
    void addDims_inplace(isl_dim_type type, unsigned n) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_add_dims(take(), type, n)); }
 void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_remove_dims(take(), type, first, n)); }
#pragma endregion


#pragma region Constraints
    void addConstraint(Constraint &&constraint);
    void dropContraint(Constraint &&constraint);

    int getCountConstraints() const;
    bool foreachConstraint(ConstraintCallback fn, void *user) const;
    bool foreachConstraint(std::function<bool(Constraint)>) const;

    Mat equalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const;
    Mat inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const;
#pragma endregion

    void projectOut(isl_dim_type type, unsigned first, unsigned n);
    void params();
    /// Eliminate the coefficients for the given dimensions from the constraints, without removing the dimensions.
    void eliminate(isl_dim_type type, unsigned first, unsigned n);
    void fix(isl_dim_type type, unsigned pos, const Int &value);
    void fix(isl_dim_type type, unsigned pos, int value);
    void detectEqualities();
    void removeRedundancies();
    void affineHull();

    Space getSpace() const;
    LocalSpace getLocalSpace() const;
    void removeDivs();
    void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n);
    void removeUnknownDivs();

    /// The number of parameters, input, output or set dimensions can be obtained using the following functions.
//    unsigned dim(isl_dim_type type) const;
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const;

//    const char *getTupleName() const;
//    void setTupleName(const char *s);

//    Id getDimId(isl_dim_type type, unsigned pos) const;
 //   const char *getDimName(isl_dim_type type, unsigned pos) const;

    bool plainIsEmpty() const;
    bool isEmpty() const;
    bool isUniverse() const;
    bool isWrapping() const;

    void dropConstraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n);
    void dropConstraintsNotInvolvingDims(isl_dim_type type, unsigned first, unsigned n);

    /// If the input (basic) set or relation is non-empty, then return a singleton subset of the input. Otherwise, return an empty set.
    void sample();
    void coefficients();
    void solutions();
    void flatten();
    void lift();
    void alignParams(Space &&model);

    void addDims(isl_dim_type type, unsigned n);
    void insertDims(isl_dim_type type, unsigned pos,  unsigned n);
    void moveDims(isl_dim_type dst_type, unsigned dst_pos,  isl_dim_type src_type, unsigned src_pos,  unsigned n);

    void apply(BasicMap &&bmap);
    void preimage(MultiAff &&ma);

    void gist(BasicSet &&context);
    Vertices computeVertices() const;
  }; // class BasicSet

  static inline BasicSet enwrap(isl_basic_set *bset) { return BasicSet::wrap(bset); }


  BasicSet params(BasicSet &&params);

  bool isSubset(const BasicSet &bset1, const BasicSet &bset2);

  BasicMap unwrap(BasicSet &&bset);

  BasicSet intersectParams(BasicSet &&bset1, BasicSet &&bset2);
  BasicSet intersect(BasicSet &&bset1, BasicSet &&bset2);
  Set union_(BasicSet &&bset1, BasicSet &&bset2);
  BasicSet flatProduct(BasicSet &&bset1, BasicSet &&bset2);

  Set partialLexmin(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  Set partialLexmax(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  PwMultiAff partialLexminPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  PwMultiAff partialLexmaxPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);

  Set lexmin(BasicSet &&bset);
  Set lexmax(BasicSet &&bset);

  Point samplePoint(BasicSet &&bset);
} // namespace isl
#endif /* ISLPP_BASICSET_H */
