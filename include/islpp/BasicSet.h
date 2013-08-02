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
#include "Space.h"
#include "LocalSpace.h"
#include "Constraint.h"

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

  class BasicSet : public Obj3<BasicSet,isl_basic_set>, public Spacelike3<BasicSet> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_basic_set_free(takeOrNull()); }
    StructTy *addref() const { return isl_basic_set_copy(keepOrNull()); }

  public:
    BasicSet() { }
    static ObjTy wrap(StructTy *obj) { return BasicSet::enwrap(obj); }// obsolete

    /* implicit */ BasicSet(ObjTy &&that) : Obj3(std::move(that)) { }
    /* implicit */ BasicSet(const ObjTy &that) : Obj3(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }


    Ctx *getCtx() const { return Ctx::enwrap(isl_basic_set_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_basic_set_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike3
    friend class isl::Spacelike3<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_basic_set_get_space(keep())); }
    LocalSpace getLocalSpace() const { return LocalSpace::wrap(isl_basic_set_get_local_space(keep())); }
    LocalSpace getSpacelike() const { return getLocalSpace(); }

  protected:
    //void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_set_tuple_id(take(), type, id.take())); }
    //void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_insert_dims(take(), type, pos, count)); }
    void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_remove_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_basic_set_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_basic_set_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_basic_set_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { assert(type==isl_dim_set); return isl_basic_set_get_tuple_name(keep()); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_basic_set_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { assert(type==isl_dim_set); give(isl_basic_set_set_tuple_name(take(), s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_basic_set_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_basic_set_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_basic_set_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_add_dims(take(), type, count)); }
#pragma endregion


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

    //BasicSet copy() const { return wrap(isl_basic_set_copy(keep())); }
    //BasicSet &&move() { return std::move(*this); }
#pragma endregion


#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
#pragma endregion


#pragma region Constraints
    void addConstraint_inplace(Constraint &&constraint) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_add_constraint(take(), constraint.take())); }
    void addConstraint_inplace(const Constraint &constraint) ISLPP_INPLACE_QUALIFIER { give(isl_basic_set_add_constraint(take(), constraint.takeCopy())); }
    BasicSet addConstraint(Constraint &&constraint) const { return BasicSet::enwrap(isl_basic_set_add_constraint(takeCopy(), constraint.take())); }
    BasicSet addConstraint(const Constraint &constraint) const { return BasicSet::enwrap(isl_basic_set_add_constraint(takeCopy(), constraint.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    BasicSet addConstraint(Constraint &&constraint) && { return BasicSet::enwrap(isl_basic_set_add_constraint(take(), constraint.take())); }
    BasicSet addConstraint(const Constraint &constraint) && { return BasicSet::enwrap(isl_basic_set_add_constraint(take(), constraint.takeCopy())); }
#endif

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

    void fix_inplace(isl_dim_type type, unsigned pos, const Int &value) ISLPP_INPLACE_QUALIFIER {   give(isl_basic_set_fix(take(), type, pos, value.keep())); }
    BasicSet fix(isl_dim_type type, unsigned pos, const Int &value) const {      BasicSet  ::enwrap(isl_basic_set_fix(takeCopy(), type, pos, value.keep())); }
    void fix_inplace(isl_dim_type type, unsigned pos, int value) ISLPP_INPLACE_QUALIFIER {      give(isl_basic_set_fix_si(take(), type, pos, value));    }
    BasicSet fix(isl_dim_type type, unsigned pos, int value) const {    return  BasicSet  ::enwrap(isl_basic_set_fix_si(takeCopy(), type, pos, value));    }

    void detectEqualities();
    void removeRedundancies();
    void affineHull();

    //Space getSpace() const;
    //LocalSpace getLocalSpace() const;
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

    void apply_inplace(BasicMap &&bmap) ISLPP_INPLACE_QUALIFIER;

    BasicSet apply(const BasicMap &bmap) const;
    Set apply(const Map &map) const;

    void preimage(MultiAff &&ma);

    void gist(BasicSet &&context);
    Vertices computeVertices() const;

    void equate_inplace(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) ISLPP_INPLACE_QUALIFIER {
      auto result = take();
      auto c = isl_equality_alloc(isl_basic_set_get_local_space(result));
      c = isl_constraint_set_coefficient_si(c, type1, pos1, 1);
      c = isl_constraint_set_coefficient_si(c, type2, pos2, -1);
      result = isl_basic_set_add_constraint(result, c);
      give(result);
    }
    BasicSet equate(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) const { auto result = copy(); result.equate_inplace(type1, pos1, type2, pos2); return result; }
  }; // class BasicSet


  static inline BasicSet enwrap(isl_basic_set *bset) { return BasicSet::wrap(bset); }

  BasicSet params(BasicSet &&params);

  bool isSubset(const BasicSet &bset1, const BasicSet &bset2);

  BasicMap unwrap(BasicSet &&bset);

  BasicSet intersectParams(BasicSet &&bset1, BasicSet &&bset2);
  BasicSet intersect(BasicSet &&bset1, BasicSet &&bset2);
  Set unite(BasicSet &&bset1, BasicSet &&bset2);
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
