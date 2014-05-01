#ifndef ISLPP_SET_H
#define ISLPP_SET_H

#include "islpp_common.h"
#include "Pw.h"
#include "Multi.h"
#include "Int.h"
#include "Obj.h"
#include "Space.h"
#include "Spacelike.h"
#include "Ctx.h"
#include "BasicSet.h"
#include "SetSpacelike.h"
#include "SetSpace.h" // class SetSpace
#include "ParamSpace.h"

#include <isl/space.h> // enum isl_dim_type;
#include <isl/lp.h> // enum isl_lp_result;
#include <isl/set.h>

#include <llvm/Support/Compiler.h>
#include <llvm/ADT/ArrayRef.h>

#include <cassert>
#include <string>
#include <functional>
#include <vector>


struct isl_set;
struct isl_basic_set;
struct isl_point;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Space;
  class Constraint;
  class BasicSet;
  class Ctx;
  class Id;
  class Aff;
  class Map;
  class Point;
  class PwQPolynomialFold;
  class Dim;
} // namespace isl


namespace isl {

  typedef int(*BasicSetCallback)(isl_basic_set *bset, void *user);
  typedef int(*PointCallback)(isl_point *pnt, void *user);


  // or Pw<BasicSet>
  class Set : public Obj<Set, isl_set>, public Spacelike<Set>, public SetSpacelike<Set> {
#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_set_free(takeOrNull()); }
    StructTy *addref() const { return isl_set_copy(keepOrNull()); }

  public:
    Set() { }

    /* implicit */ Set(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Set(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_set_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
    friend class isl::SetSpacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS SetSpace getSpace() ISLPP_PROJECTION_FUNCTION{ return SetSpace::enwrap(isl_set_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION{ return getSpace(); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return true; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return false; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_set_dim(keep(), type); }
    ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION{ return isl_set_find_dim_by_id(keep(), type, id.keep()); }

    ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ assert(type == isl_dim_set); return checkBool(isl_set_has_tuple_name(keep())); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ assert(type == isl_dim_set); return isl_set_get_tuple_name(keep()); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION{ assert(type == isl_dim_set); give(isl_set_set_tuple_name(take(), s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ assert(type == isl_dim_set); return checkBool(isl_set_has_tuple_id(keep())); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ assert(type == isl_dim_set); return Id::enwrap(isl_set_get_tuple_id(keep())); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ assert(type == isl_dim_set); give(isl_set_set_tuple_id(take(), id.take())); }
    ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{ assert(type == isl_dim_set); give(isl_set_reset_tuple_id(take())); }

    ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_set_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_set_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_set_set_dim_name(take(), type, pos, s)); }
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_set_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_set_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_set_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_set_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_set_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_set_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_set_remove_dims(take(), type, first, count)); }
#pragma endregion


    ParamSpace getParamsSpace() const { return getSpace().getParamsSpace(); }

    bool hasTupleId() const { return isl_set_has_tuple_id(keep()); }
    const char *getTupleName() const { return isl_set_get_tuple_name(keep()); }
    Id getTupleId() const { return Id::enwrap(isl_set_get_tuple_id(keep())); }

    void setTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ give(isl_set_set_tuple_id(take(), id.take())); }
    Set setTupleId(Id id) const { return Set::enwrap(isl_set_set_tuple_id(takeCopy(), id.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Set setTupleId(Id id) && { return Set::enwrap(isl_set_set_tuple_id(take(), id.take())); }
#endif

    Set setDimId(unsigned pos, const Id &id) const { return Set::enwrap(isl_set_set_dim_id(takeCopy(), isl_dim_set, pos, id.takeCopy())); }
    void setDimId_inplace(unsigned pos, const Id &id) ISLPP_INPLACE_FUNCTION{ give(isl_set_set_dim_id(take(), isl_dim_set, pos, id.takeCopy())); }

    unsigned getDimCount() const { return isl_set_dim(keep(), isl_dim_set); }
    bool hasDimId(unsigned pos) const { return checkBool(isl_set_has_dim_id(keep(), isl_dim_set, pos)); }
    Id getDimId(unsigned pos) const { return Id::enwrap(isl_set_get_dim_id(keep(), isl_dim_set, pos)); }

#pragma region Conversion
    // From BasicSet
    /* implicit */ Set(BasicSet set) : Obj(set.isNull() ? nullptr : isl_set_from_basic_set(set.take())) {}
    const Set &operator=(BasicSet that) { reset(that.isNull() ? nullptr : isl_set_from_basic_set(that.take())); return *this; }
#pragma endregion


#pragma region Creational
    static Set createEmpty(Space &&space);
    /// @brief Contains all integers
    static Set createUniverse(Space &&space);
    /// @brief Contains all non-negative integers
    static Set createNatUniverse(Space &&space);
    /// @brief A zero-dimensional (basic) set can be constructed on a given parameter domain using the following function.
    static Set createFromParams(Set &&set);
    static Set createFromPwAff(PwAff &&);
    static Set createFromPwMultiAff(PwMultiAff &&aff);
    static Set createFromPoint(Point &&);
    static Set createBoxFromPoints(Point &&pnt1, Point &&pnt2);

    static Set readFrom(Ctx *, FILE *);
    static Set readFrom(Ctx *, const char *);
#pragma endregion


#pragma region Printing
    void printPovray(llvm::raw_ostream &out) const;
#pragma endregion

    void addConstraint_inplace(const Constraint &c) ISLPP_INPLACE_FUNCTION{ give(isl_set_add_constraint(take(), c.takeCopy())); }
    Set addContraint(const Constraint &c) const { return Set::enwrap(isl_set_add_constraint(takeCopy(), c.takeCopy())); }

    Set complement() const;
    Set projectOut(isl_dim_type type, unsigned first, unsigned n) const;
    Set params() const;

    /// Return false on success of true in case of any error
    /// The callback function fn should return 0 if successful and -1 if an error occurs. In the latter case, or if any other error occurs, the above functions will return -1.
    bool foreachBasicSet(BasicSetCallback, void *user) const;
    bool foreachBasicSet(std::function<bool(BasicSet/*rvalue ref?*/)>) const;
    bool foreachPoint(PointCallback, void *user) const;
    bool foreachPoint(std::function<bool(Point/*rvalue ref?*/)> fn) const;
    std::vector<Point> getPoints() const;

    int getBasicSetCount() const;
    ISLPP_EXSITU_ATTRS std::vector<BasicSet> getBasicSets() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS BasicSet anyBasicSet() ISLPP_EXSITU_FUNCTION;

    int getInvolvedDims(isl_dim_type, unsigned first, unsigned n) const;
    bool dimHasAnyLowerBound(isl_dim_type, unsigned pos) const;
    bool dimHasAnyUpperBound(isl_dim_type, unsigned pos) const;
    bool dimHasLowerBound(isl_dim_type, unsigned pos) const;
    bool dimHasUpperBound(isl_dim_type, unsigned pos) const;

    bool plainIsEmpty() const;
    bool isEmpty() const;
    bool plainIsUniverse() const;
    /// @brief Check if the relation obviously lies on a hyperplane where the given dimension has a fixed value and if so, return that value in *val.
    bool plainIsFixed(isl_dim_type type, unsigned pos, Int &val) const;

    /// Eliminate the coefficients for the given dimensions from the constraints, without removing the dimensions.
    void eliminate(isl_dim_type type, unsigned first, unsigned n);

    void fix(isl_dim_type type, unsigned pos, const Int &value);
    void fix(isl_dim_type type, unsigned pos, int value);

    /// Intersect the set or relation with the hyperplane where the given dimension has the fixed given value.
    void lowerBound(isl_dim_type type, unsigned pos, const Int &value);
    void lowerBound(isl_dim_type type, unsigned pos, signed long value);
    void upperBound(isl_dim_type type, unsigned pos, const Int &value);
    void upperBound(isl_dim_type type, unsigned pos, signed long value);

    /// Intersect the set or relation with the half-space where the given dimension has a value bounded by the fixed given value.
    void equate(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2);

    /// Simplify the representation of a set or relation by trying to combine pairs of basic sets or relations into a single basic set or relation.
    ISLPP_EXSITU_ATTRS Set coalesce() ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_coalesce(takeCopy())); }
    ISLPP_INPLACE_ATTRS void coalesce_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_CONSUME_ATTRS Set coalesce_consume() ISLPP_CONSUME_FUNCTION{ return Set::enwrap(isl_set_coalesce(take())); }

      /// Simplify the representation of a set or relation by detecting implicit equalities.
    ISLPP_EXSITU_ATTRS Set detectEqualities() ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_detect_equalities(takeCopy())); }
    ISLPP_INPLACE_ATTRS void detectEqualities_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_set_detect_equalities(take())); }
    ISLPP_CONSUME_ATTRS Set detectEqualities_consume() ISLPP_CONSUME_FUNCTION{ return Set::enwrap(isl_set_detect_equalities(take())); }

      /// Removing redundant constraints
    ISLPP_EXSITU_ATTRS Set removeRedundancies() ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_remove_redundancies(takeCopy())); }
    ISLPP_INPLACE_ATTRS void removeRedundancies_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_set_remove_redundancies(take())); }
    ISLPP_CONSUME_ATTRS Set removeRedundancies_consume() ISLPP_CONSUME_FUNCTION{ return Set::enwrap(isl_set_remove_redundancies(take())); }

    ISLPP_EXSITU_ATTRS Set makeDisjoint() ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_make_disjoint(takeCopy())); }
    ISLPP_INPLACE_ATTRS void makeDisjoint_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_set_make_disjoint(take())); }
    ISLPP_CONSUME_ATTRS Set makeDisjoint_consume() ISLPP_CONSUME_FUNCTION{ return Set::enwrap(isl_set_make_disjoint(take())); }


      /// These functions drop any constraints (not) involving the specified dimensions. Note that the result depends on the representation of the input.
    void dropContraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n);

    /// Remove any internal structure of domain (and range) of the given set or relation. If there is any such internal structure in the input, then the name of the space is also removed.
    void flatten();

    /// Lift the input set to a space with extra dimensions corresponding to the existentially quantified variables in the input. In particular, the result lives in a wrapped map where the domain is the original space and the range corresponds to the original existentially quantified variables.
    void lift();

    /// Returns a somewhat strange aff without a domain (i.e. Aff::getSpace() returns a set space)
    PwAff dimMin(int pos) const;
    PwAff dimMin(const Dim &dim) const;
    PwAff dimMax(int pos) const;
    PwAff dimMax(const Dim &dim) const;

    void unite_inplace(Set &&that) ISLPP_INPLACE_FUNCTION{ give(isl_set_union(take(), that.take())); }
    void unite_inplace(const Set &that) ISLPP_INPLACE_FUNCTION{ give(isl_set_union(take(), that.takeCopy())); }

    void apply_inplace(Map &&map) ISLPP_INPLACE_FUNCTION;
    void apply_inplace(const Map &map) ISLPP_INPLACE_FUNCTION;
    Set apply(Map &&map) const;
    Set apply(const Map &map) const;
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Set apply(Map &&map) && ;
    Set apply(const Map &map) &&;
#endif

    Set lexmin() const { return Set::enwrap(isl_set_lexmin(takeCopy())); }
    Set lexmax() const { return Set::enwrap(isl_set_lexmax(takeCopy())); }

    PwMultiAff lexminPwMultiAff() const;
    PwMultiAff lexmaxPwMultiAff() const;

    Set subtract(const Set &that) const { return Set::enwrap(isl_set_subtract(takeCopy(), that.takeCopy())); }

    Map unwrap() const;

    // { A } and { A' -> B } to { A*A' -> B } (where * means intersection)
    Map chain(const Map &map) ISLPP_EXSITU_FUNCTION;

    // { (A -> B -> C ) } and { B' -> D } to { (A -> B*B' -> C) -> D }
    //Map chainNested(const Map &map) const; // deprecated; use chainSubspace
    Map chainSubspace(const Map &map) ISLPP_EXSITU_FUNCTION;
    Map chainSubspace_consume(const Map &map) ISLPP_CONSUME_FUNCTION;
    Map chainNested(const Map &map, unsigned tuplePos) const; // deprecated?

    ISLPP_EXSITU_ATTRS PwMultiAff chainSubspace(PwMultiAff pma) ISLPP_EXSITU_FUNCTION;
    ISLPP_CONSUME_ATTRS PwMultiAff chainSubspace_consume(PwMultiAff pma) ISLPP_CONSUME_FUNCTION;

    void permuteDims_inplace(llvm::ArrayRef<unsigned> order) ISLPP_INPLACE_FUNCTION;
    Set permuteDims(llvm::ArrayRef<unsigned> order) const { auto result = copy(); result.permuteDims_inplace(order); return result; }

    bool isSubsetOf(const Set &that) const { return checkBool(isl_set_is_subset(keep(), that.keep())); }
    bool isSupersetOf(const Set &that) const { return checkBool(isl_set_is_subset(that.keep(), keep())); }

    /// moves the dimensions of one nested tuple to the range of a map
    /// The tuple is identified by its position relative to the other nested spaces or by its identifier, where the first space with this id is chosen
    /// examples:
    ///   { ((A -> B) -> C) }.unwrapTuple(1) = { (A -> C) -> B }
    ///   { (A -> (B -> C)) }.unwrapTuple(B) = { (A -> C) -> B }
    //Map unwrapTuple_internal(unsigned TuplePos) ISLPP_INTERNAL_QUALIFIER;
    //Map unwrapTuple_internal(const Id &tupleId) ISLPP_INTERNAL_QUALIFIER;
    Map unwrapSubspace(const Space &subspace) const;

    Set intersect(const Set &that) ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_intersect(takeCopy(), that.takeCopy())); }
    void intersect_inplace(Set that) ISLPP_INPLACE_FUNCTION{ give(isl_set_intersect(take(), that.take())); }
    void intersect_inplace(UnionSet that) ISLPP_INPLACE_FUNCTION;

    /// Similar to Map.rangeMap() and Map.domainMap(), but allow to select the subspace to map to 
    /// { (A, B, C) }.subspspaceMap({ B }) = { (A, B, C) -> B }
    Map subspaceMap(const Space &subspace) const;
    Map subrangeMap(unsigned first, unsigned count) const;

    Set resetTupleId() const { return Set::enwrap(isl_set_reset_tuple_id(takeCopy())); }

    //Map reorganizeTuples(llvm::ArrayRef<unsigned> domainTuplePos, llvm::ArrayRef<unsigned> rangeTuplePos);
    Map reorganizeSubspaceList(llvm::ArrayRef<Space> domainTuplePos, llvm::ArrayRef<Space> rangeTuplePos);
    Map reorganizeSubspaces(const Space &domainSpace, const Space &rangeSpace, bool mustExist = false) ISLPP_EXSITU_FUNCTION;
    Set reorganizeSubspaces(const Space &setSpace, bool mustExist = false) ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS Map reorderSubspaces(const Space &domainSpace, const Space &rangeSpace) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Set reorderSubspaces(const Space &setSpace) ISLPP_EXSITU_FUNCTION{ return reorganizeSubspaces(std::move(setSpace), true); }

    ISLPP_EXSITU_ATTRS Set cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ obj_give(cast(space).move()); }

    ISLPP_EXSITU_ATTRS Set cast() ISLPP_EXSITU_FUNCTION{ return cast(getSpace().untyped()); } // TODO: Some no-op on dimension that reset the space

      //FIXME: Not part of public isl interface
    ISLPP_EXSITU_ATTRS Set resetSpace(Space dim) ISLPP_EXSITU_FUNCTION{ return Set::enwrap(isl_set_reset_space(takeCopy(), dim.take())); }
    ISLPP_INPLACE_ATTRS void resetSpace_inplace(Space dim) ISLPP_INPLACE_FUNCTION{ give(isl_set_reset_space(take(), dim.take())); }
    ISLPP_INPLACE_ATTRS Set resetSpace_consume(Space dim) ISLPP_INPLACE_FUNCTION{ return Set::enwrap(isl_set_reset_space(take(), dim.take())); }


    void printExplicit(llvm::raw_ostream &os, int maxElts = 8, bool newlines = false, bool formatted = false, bool sorted = true) const;
    void dumpExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const;
    void dumpExplicit() const; // In order do be callable without arguments from debugger
    std::string toStringExplicit(int maxElts = 8, bool newlines = false, bool formatted = false, bool srted = true);

    std::string toString()  const;

    ISLPP_PROJECTION_ATTRS bool isFixed(isl_dim_type type, unsigned pos, Int &val) ISLPP_PROJECTION_FUNCTION{
      // There is no set variant; use the fact that in the implementation, a isl_set is a isl_map in disguise
      auto result = isl_map_plain_is_fixed((isl_map*)keep(), type, pos, val.change());
      val.updated();
      return result;
    }

    ISLPP_PROJECTION_ATTRS uint64_t getComplexity() ISLPP_PROJECTION_FUNCTION;
    ISLPP_PROJECTION_ATTRS uint64_t getOpComplexity() ISLPP_PROJECTION_FUNCTION;

    ISLPP_PROJECTION_ATTRS HasBounds getDimBounds(isl_dim_type type, pos_t pos, Int &lowerBound, Int &upperBound) ISLPP_PROJECTION_FUNCTION;

    AllBounds getAllBounds() const;

    //ISLPP_EXSITU_ATTRS Set coalesceEx() ISLPP_EXSITU_FUNCTION;
    //ISLPP_INPLACE_ATTRS void coalesceEx_inplace() ISLPP_INPLACE_FUNCTION{ obj_give(coalesceEx()); }
    //ISLPP_CONSUME_ATTRS Set coalesceEx_consume() ISLPP_CONSUME_FUNCTION{ return coalesceEx(); }

    ISLPP_EXSITU_ATTRS BasicSet sample() ISLPP_EXSITU_FUNCTION { return BasicSet::enwrap(isl_set_sample(takeCopy())); }
    ISLPP_CONSUME_ATTRS BasicSet sample() ISLPP_CONSUME_FUNCTION { return BasicSet::enwrap(isl_set_sample(take())); }

  }; // class Set



  static inline Set enwrap(isl_set *obj) { return Set::enwrap(obj); }
  static inline Set enwrapCopy(isl_set *obj) { return Set::enwrapCopy(obj); }

  /// @brief Convex hull
  /// If the input set or relation has any existentially quantified variables, then the result of these operations is currently undefined.
  BasicSet convexHull(Set&&);

  /// These functions compute a single basic set or relation that contains the whole input set or relation. In particular, the output is described by translates of the constraints describing the basic sets or relations in the input. In case of isl_set_unshifted_simple_hull, only the original constraints are used, without any translation.
  BasicSet unshiftedSimpleHull(Set&&);
  static inline BasicSet simpleHull(Set set) { return BasicSet::enwrap(isl_set_simple_hull(set.take())); }

  /// @brief Affine hull
  static inline  BasicSet affineHull(Set set) { return BasicSet::enwrap(isl_set_affine_hull(set.take())); }

  /// These functions compute a single basic set or relation not involving any existentially quantified variables that contains the whole input set or relation. In case of union sets and relations, the polyhedral hull is computed per space.
  static inline BasicSet polyhedralHull(Set set) { return BasicSet::enwrap(isl_set_polyhedral_hull(set.take())); }

  /// If the input (basic) set or relation is non-empty, then return a singleton subset of the input. Otherwise, return an empty set.
  BasicSet sample(Set &&);

  /// Compute the minimum or maximum of the given set or output dimension as a function of the parameters (and input dimensions), but independently of the other set or output dimensions. For lexicographic optimization, see Lexicographic Optimization.
  PwAff dimMin(Set &&, int pos);
  PwAff dimMax(Set &&, int pos);

  /// The following function computes the set of (rational) coefficient values of valid constraints for the given set. 
  /// Internally, these two sets of functions perform essentially the same operations, except that the set of coefficients is assumed to be a cone, while the set of values may be any polyhedron. The current implementation is based on the Farkas lemma and Fourier-Motzkin elimination, but this may change or be made optional in future. In particular, future implementations may use different dualization algorithms or skip the elimination step.
  BasicSet coefficients(Set &&);
  /// The following function computes the set of (rational) values satisfying the constraints with coefficients from the given set. 
  /// Internally, these two sets of functions perform essentially the same operations, except that the set of coefficients is assumed to be a cone, while the set of values may be any polyhedron. The current implementation is based on the Farkas lemma and Fourier-Motzkin elimination, but this may change or be made optional in future. In particular, future implementations may use different dualization algorithms or skip the elimination step.
  BasicSet solutions(Set &&);


  //Map unwrap(Set &&);

  /// The function above constructs a relation that maps the input set to a flattened version of the set.
  Map flattenMap(Set &&);


  /// Change the order of the parameters of the given set or relation such that the first parameters match those of model. This may involve the introduction of extra parameters. All parameters need to be named.
  Set alignParams(Set &&set, Space &&model);

  Set addDims(Set &&, isl_dim_type type, unsigned n);
  Set insertDims(Set &&set, isl_dim_type type, unsigned pos, unsigned n);
  Set moveDims(Set &&set, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n);

  Set intersectParams(Set &&set, Set &&params);
  Set intersect(Set set1, Set set2);

  static inline Set unite(Set &&set1, Set &&set2) { return Set::enwrap(isl_set_union(set1.take(), set2.take())); }
  static inline Set unite(Set &&set1, const Set &set2) { return Set::enwrap(isl_set_union(set1.take(), set2.takeCopy())); }
  static inline Set unite(const Set &set1, Set &&set2) { return Set::enwrap(isl_set_union(set1.takeCopy(), set2.take())); }
  static inline Set unite(const Set &set1, const Set &set2) { return Set::enwrap(isl_set_union(set1.takeCopy(), set2.takeCopy())); }

  static inline Set subtract(Set set1, Set set2) { return Set::enwrap(isl_set_subtract(set1.take(), set2.take())); }
  static inline Set operator-(Set set1, Set set2) { return Set::enwrap(isl_set_subtract(set1.take(), set2.take())); }

  Set apply(Set &&set, Map &&map);
  Set apply(const Set &set, Map &&map);
  Set apply(Set &&set, const Map &map);
  Set apply(const Set &set, const Map &map);

  Set preimage(Set &&set, MultiAff &&ma);
  Set preimage(Set &&set, PwMultiAff &&ma);

  /// @brief Cartesian product
  /// The above functions compute the cross product of the given sets or relations. The domains and ranges of the results are wrapped maps between domains and ranges of the inputs.
  static inline Set product(Set set1, Set set2) { return Set::enwrap(isl_set_product(set1.take(), set2.take())); }

  static inline Set flatProduct(Set set1, Set set2) { return Set::enwrap(isl_set_flat_product(set1.take(), set2.take())); }

  Map alltoall(Set domainSet, Set rangeSet);
  Map alltoall(Space domainUniverse, Set rangeSet);

  Set gist(Set &&set, Set &&context);
  Set gistParams(Set &&set, Set &&context);
  Set partialLexmin(Set &&set, Set &&dom, Set &empty);
  Set partialLexmax(Set &&set, Set &&dom, Set &empty);
  Set lexmin(Set &&set);
  Set lexmax(Set &&set);
  PwAff indicatorFunction(Set &&set);

  Point samplePoint(Set &&set);
  PwQPolynomialFold apply(Set &&set, PwQPolynomialFold &&pwf, bool &tight);


  Set complement(Set &&set);
  Set projectOut(Set &&set, isl_dim_type type, unsigned first, unsigned n);
  Set params(Set &&set);

  Set addContraint(Set &&, Constraint &&);

  Set computeDivs(Set &&);
  Set alignDivs(Set &&);
  /// @brief The existentially quantified variables can be removed using the following functions, which compute an overapproximation.
  Set removeDivs(Set &&);
  Set removeDivsInvolvingDims(Set &&, isl_dim_type, unsigned, unsigned);
  Set removeUnknownDivs(Set &&);

  Set makeDisjoint(Set &&);

  bool plainsIsEqual(const Set &left, const Set &right);
  bool isEqual(const Set &left, const Set &right);

  bool plainIsDisjoint(const Set &lhs, const Set &rhs);
  bool isDisjoint(const Set &lhs, const Set &rhs);

  bool isSubset(const Set &lhs, const Set &rhs);
  bool isStrictSubset(const Set &lhs, const Set &rhs);

  /// This function is useful for sorting isl_sets. The order depends on the internal representation of the inputs. The order is fixed over different calls to the function (assuming the internal representation of the inputs has not changed), but may change over different versions of isl.
  int plainCmp(const Set &lhs, const Set &rhs);

#if 0
  __isl_give isl_set *isl_set_from_union_set(__isl_take isl_union_set *uset);
#endif

  // Note these are NOT total orders
  static inline bool operator<=(const Set &map1, const Set &map2) { return checkBool(isl_set_is_subset(map1.keep(), map2.keep())); }
  static inline bool operator<(const Set &map1, const Set &map2) { return checkBool(isl_set_is_strict_subset(map1.keep(), map2.keep())); }
  static inline bool operator>=(const Set &map1, const Set &map2) { return checkBool(isl_set_is_subset(map2.keep(), map1.keep())); }
  static inline bool operator>(const Set &map1, const Set &map2) { return checkBool(isl_set_is_strict_subset(map2.keep(), map1.keep())); }
  static inline bool operator==(const Set &map1, const Set &map2) { return checkBool(isl_set_is_equal(map1.keep(), map2.keep())); }

} // namespace isl
#endif /* ISLPP_SET_H */
