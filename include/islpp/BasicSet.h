#ifndef ISLPP_BASICSET_H
#define ISLPP_BASICSET_H

#include "islpp_common.h"

#include <isl/space.h> // enum isl_dim_type;
#include "Spacelike.h" // class Obj<,>
#include <isl/set.h>
#include <llvm/Support/ErrorHandling.h>
#include "Ctx.h"
#include "Id.h"
#include "Space.h"
#include "LocalSpace.h"
#include "Constraint.h"
#include <string>
#include <functional>
#include "Islfwd.h"
#include <isl/deprecated/set_int.h>

struct isl_basic_set;
struct isl_constraint;

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {
  typedef int (*ConstraintCallback)(isl_constraint *c, void *user);

  class BasicSet : public Obj<BasicSet,isl_basic_set>, public Spacelike<BasicSet> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_basic_set_free(takeOrNull()); }
    StructTy *addref() const { return isl_basic_set_copy(keepOrNull()); }

  public:
    BasicSet() { }
    static ObjTy wrap(StructTy *obj) { return BasicSet::enwrap(obj); }// obsolete

    /* implicit */ BasicSet(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ BasicSet(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }


    Ctx *getCtx() const { return Ctx::enwrap(isl_basic_set_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION { return Space::enwrap(isl_basic_set_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION { return getLocalSpace(); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION { return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION { return true; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION { return false; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_basic_set_dim(keep(), type); }
    //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_basic_set_find_dim_by_id(keep(), type, id.keep()); }

    //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { assert(type==isl_dim_set); return isl_basic_set_get_tuple_name(keep()); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { assert(type==isl_dim_set); give(isl_basic_set_set_tuple_name(take(), s)); }
    //ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_tuple_id(keep(), type)); }
    //ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_basic_set_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION { assert(type==isl_dim_set); give(isl_basic_set_set_tuple_id(take(), id.take())); }
    //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_reset_tuple_id(take(), type)); }

    //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return isl_basic_set_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_set_dim_name(take(), type, pos, s)); }
    //ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_basic_set_get_dim_id(keep(), type, pos)); }
    //ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_set_dim_id(take(), type, pos, id.take())); }
    //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_remove_dims(take(), type, first, count)); }
#pragma endregion


      ISLPP_PROJECTION_ATTRS LocalSpace getLocalSpace() ISLPP_PROJECTION_FUNCTION { return LocalSpace::enwrap(isl_basic_set_get_local_space(keep())); }

    unsigned getDimCount() const { return isl_basic_set_dim(keep(), isl_dim_set); }
    bool hasDimId(unsigned pos) const { return Spacelike<BasicSet>::hasDimId(isl_dim_set, pos); }
    Id getDimId(unsigned pos) const { return Id::enwrap(isl_basic_set_get_dim_id(keep(), isl_dim_set, pos)); }

    isl::Id getTupleId() const { return Spacelike<BasicSet>::getTupleId(isl_dim_set); }
    bool hasTupleId() const { return Spacelike<BasicSet>::hasTupleId(isl_dim_set); }
    BasicSet setTupleId(const Id &id) const  { return Spacelike<BasicSet>::setTupleId(isl_dim_set, id); }
    ISLPP_INPLACE_ATTRS void setTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION { Spacelike<BasicSet>::setSetTupleId_inplace(std::move(id)); }

#pragma region Creational
    static BasicSet create(Ctx *ctx, count_t nparam, count_t dim, count_t extra, count_t n_eq, count_t n_ineq) { return BasicSet::wrap(isl_basic_set_alloc(ctx->keep(), nparam, dim, extra, n_eq, n_ineq)); }
    static BasicSet createEmpty(Space space) { return BasicSet::wrap(isl_basic_set_empty(space.take())); }
    static BasicSet createUniverse(Space space) { return BasicSet::wrap(isl_basic_set_universe(space.take())); }
    static BasicSet createNatUniverse(Space &&space);
    static BasicSet createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4);
    static BasicSet createFromPoint(Point &&pnt);
    static BasicSet createBoxFromPoints(Point &&pnt1, Point &&pnt2);
    static BasicSet createFromBasicMap(BasicMap &&bmap);

    static BasicSet readFromFile(Ctx *ctx, FILE *input);
    static BasicSet readFromStr(Ctx *ctx, const char *str);
#pragma endregion


#pragma region Conversion
    Set toSet() const;
#pragma endregion


#pragma region Constraints
    ISLPP_EXSITU_ATTRS  BasicSet addConstraint(Constraint constraint) ISLPP_EXSITU_FUNCTION { return BasicSet::enwrap(isl_basic_set_add_constraint(takeCopy(), constraint.take())); }
    ISLPP_INPLACE_ATTRS  void addConstraint_inplace(Constraint constraint) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_add_constraint(take(), constraint.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    BasicSet addConstraint(Constraint constraint) && { return BasicSet::enwrap(isl_basic_set_add_constraint(take(), constraint.take())); }
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

    void fix_inplace(isl_dim_type type, unsigned pos, const Int &value) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_fix(take(), type, pos, value.keep())); }
    BasicSet fix(isl_dim_type type, unsigned pos, const Int &value) const { return  BasicSet::enwrap(isl_basic_set_fix(takeCopy(), type, pos, value.keep())); }
    void fix_inplace(isl_dim_type type, unsigned pos, int value) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_fix_si(take(), type, pos, value)); }
    BasicSet fix(isl_dim_type type, unsigned pos, int value) const { return BasicSet::enwrap(isl_basic_set_fix_si(takeCopy(), type, pos, value)); }

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

        const char *getTupleName() const;
        void setTupleName(const char *s);

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

    //void addDims(isl_dim_type type, unsigned n);
    //void insertDims(isl_dim_type type, unsigned pos, unsigned n);
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos,  unsigned n);

    void apply_inplace(BasicMap &&bmap) ISLPP_INPLACE_FUNCTION;

    BasicSet apply(const BasicMap &bmap) const;
    Set apply(const Map &map) const;

    void preimage(MultiAff &&ma);

    void gist(BasicSet &&context);
    Vertices computeVertices() const;

    void equate_inplace(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) ISLPP_INPLACE_FUNCTION {
      auto result = take();
      auto c = isl_equality_alloc(isl_basic_set_get_local_space(result));
      c = isl_constraint_set_coefficient_si(c, type1, pos1, 1);
      c = isl_constraint_set_coefficient_si(c, type2, pos2, -1);
      result = isl_basic_set_add_constraint(result, c);
      give(result);
    }
    BasicSet equate(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) const { auto result = copy(); result.equate_inplace(type1, pos1, type2, pos2); return result; }
    void equate_inplace(Dim dim1, Dim dim2) ISLPP_INPLACE_FUNCTION { equate_inplace(dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos() ); }
    BasicSet equate(Dim dim1, Dim dim2) const { return equate(dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos()); }

    void alignParams_inplace(Space &&model) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_align_params(take(), model.take())); }
    void alignParams_inplace(const Space &model) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_align_params(take(), model.takeCopy())); }
    BasicSet alignParams(Space &&model) const { return BasicSet::enwrap(isl_basic_set_align_params(takeCopy(), model.take())); }
    BasicSet alignParams(const Space &model) const { return BasicSet::enwrap(isl_basic_set_align_params(takeCopy(), model.takeCopy())); }

    /// Returns a somewhat strange aff without a domain (i.e. getSpace() returns a set space)
    ISLPP_EXSITU_ATTRS Aff dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS BasicSet cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION { obj_give(cast(std::move(space))); }
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
