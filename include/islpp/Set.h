#ifndef ISLPP_SET_H
#define ISLPP_SET_H

#include "islpp_common.h"
#include "Pw.h"
#include "Multi.h"
#include "Int.h"
#include <llvm/Support/Compiler.h>
#include <cassert>
#include <string>
#include <functional>
#include <vector>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/lp.h> // enum isl_lp_result;
#include <isl/set.h>
#include "Obj.h"
#include "Space.h"
#include "Spacelike.h"
#include "Ctx.h"
#include "BasicSet.h"
#include <llvm/ADT/ArrayRef.h>

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

  typedef int (*BasicSetCallback)(isl_basic_set *bset, void *user);
  typedef int (*PointCallback)(isl_point *pnt, void *user);


  class Set : public Obj<Set, isl_set>, public Spacelike<Set> {


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
    void dump() const { isl_set_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_set_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { assert(type==isl_dim_set); give(isl_set_set_tuple_id(take(), id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_set_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_set_insert_dims(take(), type, pos, count)); }
    void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_set_remove_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_set_dim(keep(), type); }
    int findDimById(isl_dim_type type, const Id &id) const { return isl_set_find_dim_by_id(keep(), type, id.keep()); }

    bool hasTupleId(isl_dim_type type) const { assert(type==isl_dim_set); return isl_set_has_tuple_id(keep()); }
    const char *getTupleName(isl_dim_type type) const { assert(type==isl_dim_set); return isl_set_get_tuple_name(keep()); }
    Id getTupleId(isl_dim_type type) const { assert(type==isl_dim_set); return Id::enwrap(isl_set_get_tuple_id(keep())); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { assert(type==isl_dim_set); give(isl_set_set_tuple_name(take(), s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_set_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_set_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_set_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_set_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_set_add_dims(take(), type, count)); }
#pragma endregion


    bool hasTupleId() const { return isl_set_has_tuple_id(keep()); }
    const char *getTupleName() const {  return isl_set_get_tuple_name(keep()); }
    Id getTupleId() const { return Id::enwrap(isl_set_get_tuple_id(keep())); }

    void setTupleId_inplace(Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_set_set_tuple_id(take(), id.take())); }
    void setTupleId_inplace(const Id &id) ISLPP_INPLACE_QUALIFIER { give(isl_set_set_tuple_id(take(), id.takeCopy())); }
    Set setTupleId(Id &&id) const { return Set::enwrap(isl_set_set_tuple_id(takeCopy(), id.take())); }
    Set setTupleId(const Id &id) const { return Set::enwrap(isl_set_set_tuple_id(takeCopy(), id.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Set setTupleId(Id &&id) && { return Set::enwrap(isl_set_set_tuple_id(take(), id.take())); }
    Set setTupleId(const Id &id) && { return Set::enwrap(isl_set_set_tuple_id(take(), id.takeCopy())); }
#endif

    unsigned getDimCount() const { return isl_set_dim(keep(), isl_dim_set); }
    bool hasDimId(unsigned pos) const { return checkBool(isl_set_has_dim_id(keep(), isl_dim_set, pos)); }
    Id getDimId(unsigned pos) const { return Id::enwrap(isl_set_get_dim_id(keep(), isl_dim_set, pos)); }

#pragma region Conversion
    // From BasicSet
    /* implicit */ Set(BasicSet &&set) : Obj(isl_set_from_basic_set(set.take())) {}
    /* implicit */ Set(const BasicSet &set) : Obj(isl_set_from_basic_set(set.takeCopy())) {}
    const Set &operator=(BasicSet &&that) { give(isl_set_from_basic_set(that.take())); return *this; }
    const Set &operator=(const BasicSet &that) { give(isl_set_from_basic_set(that.takeCopy())); return *this; }
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
    static Set createBocFromPoints(Point &&pnt1, Point &&pnt2);

    static Set readFrom(Ctx *, FILE *);
    static Set readFrom(Ctx *, const char *);

    //Set copy() const { return Set::enwrap(takeCopy()); }
    //Set &&move() { return std::move(*this); }
#pragma endregion


#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    void printPovray(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
#pragma endregion

    void addConstraint_inplace(const Constraint &c) ISLPP_INPLACE_QUALIFIER { give(isl_set_add_constraint(take(), c.takeCopy())); }
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

    //unsigned dim(isl_dim_type) const;
    //unsigned getSetDimCount() const;
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
    void coalesce();

    /// Simplify the representation of a set or relation by detecting implicit equalities.
    void detectEqualities();

    /// Removing redundant constraints
    void removeRedundancies();

    /// These functions drop any constraints (not) involving the specified dimensions. Note that the result depends on the representation of the input.
    void dropContraintsInvolvingDims(isl_dim_type type,  unsigned first, unsigned n);

    /// Remove any internal structure of domain (and range) of the given set or relation. If there is any such internal structure in the input, then the name of the space is also removed.
    void flatten();

    /// Lift the input set to a space with extra dimensions corresponding to the existentially quantified variables in the input. In particular, the result lives in a wrapped map where the domain is the original space and the range corresponds to the original existentially quantified variables.
    void lift();

    PwAff dimMin(int pos) const;
    PwAff dimMin(const Dim &dim) const;
    PwAff dimMax(int pos) const;
    PwAff dimMax(const Dim &dim) const;

    void unite_inplace(Set &&that) ISLPP_INPLACE_QUALIFIER { give(isl_set_union(take(), that.take())); }
    void unite_inplace(const Set &that) ISLPP_INPLACE_QUALIFIER { give(isl_set_union(take(), that.takeCopy())); }

    void apply_inplace(Map &&map) ISLPP_INPLACE_QUALIFIER; 
    void apply_inplace(const Map &map) ISLPP_INPLACE_QUALIFIER; 
    Set apply(Map &&map) const ;
    Set apply(const Map &map) const;
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Set apply(Map &&map) && ;
    Set apply(const Map &map) &&;
#endif

    Set lexmin() const { return Set::enwrap(isl_set_lexmin(takeCopy())); }
    Set lexmax() const { return Set::enwrap(isl_set_lexmax(takeCopy())); }

    PwMultiAff lexminPwMultiAff() const;
    PwMultiAff lexmaxPwMultiAff() const;

    Set subtract(const Set &that) const { return Set::enwrap(isl_set_subtract(takeCopy(), that.takeCopy())); }

    Map unwrap() const;

    // { A } and { A' -> B } to { A*A' -> B }
    Map chain(const Map &map) const;


    void permuteDims_inplace(llvm::ArrayRef<unsigned> order) ISLPP_INPLACE_QUALIFIER;
    Set permuteDims(llvm::ArrayRef<unsigned> order) const { auto result = copy(); result.permuteDims_inplace(order); return result; }

    bool isSubsetOf(const Set &&that) const { return isl_set_is_subset(keep(), that.keep()); }
    bool isSupersetOf(const Set &&that) const { return isl_set_is_subset(that.keep(), keep()); }
  }; // class Set


  static inline Set enwrap(isl_set *obj) { return Set::enwrap(obj); }
  static inline Set enwrapCopy(isl_set *obj) { return Set::enwrapCopy(obj); }

  /// @brief Convex hull
  /// If the input set or relation has any existentially quantified variables, then the result of these operations is currently undefined.
  BasicSet convexHull(Set&&);

  /// These functions compute a single basic set or relation that contains the whole input set or relation. In particular, the output is described by translates of the constraints describing the basic sets or relations in the input. In case of isl_set_unshifted_simple_hull, only the original constraints are used, without any translation.
  BasicSet unshiftedSimpleHull(Set&&);
  BasicSet simpleHull(Set&&);

  /// @brief Affine hull
  /// In case of union sets and relations, the affine hull is computed per space.
  BasicSet affineHull(Set &&);

  /// These functions compute a single basic set or relation not involving any existentially quantified variables that contains the whole input set or relation. In case of union sets and relations, the polyhedral hull is computed per space.
  BasicSet polyhedralHull(Set &&);

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

  Set addDims(Set &&,isl_dim_type type, unsigned n);
  Set insertDims(Set &&set, isl_dim_type type, unsigned pos, unsigned n);
  Set moveDims(Set &&set, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos,  unsigned n);

  Set intersectParams(Set &&set, Set &&params);
  Set intersect(Set &&set1, Set &&set2);

  static inline Set unite(Set &&set1, Set &&set2) { return Set::enwrap(isl_set_union(set1.take(), set2.take())); }
  static inline Set unite(Set &&set1, const Set &set2) { return Set::enwrap(isl_set_union(set1.take(), set2.takeCopy())); }
  static inline Set unite(const Set &set1, Set &&set2) { return Set::enwrap(isl_set_union(set1.takeCopy(), set2.take())); }
  static inline Set unite(const Set &set1, const Set &set2) { return Set::enwrap(isl_set_union(set1.takeCopy(), set2.takeCopy())); }

  Set subtract(Set &&set1, Set &&set2);

  Set apply(Set &&set, Map &&map);
  Set apply(const Set &set, Map &&map);
  Set apply(Set &&set, const Map &map);
  Set apply(const Set &set, const Map &map);

  Set preimage(Set &&set, MultiAff &&ma);
  Set preimage(Set &&set, PwMultiAff &&ma);
  /// @brief Cartesian product
  /// The above functions compute the cross product of the given sets or relations. The domains and ranges of the results are wrapped maps between domains and ranges of the inputs.
  Set product(Set &&set1, Set &&set2);
 static inline Set product(const Set &set1, Set &set2) { return Set::enwrap(isl_set_product(set1.takeCopy(), set2.takeCopy())); }

  Set flatProduct(Set &&set1,Set &&set2);
   static inline Set flatProduct(const Set &set1,const Set &set2) { return Set::enwrap(isl_set_flat_product(set1.takeCopy(), set2.takeCopy())); }

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

} // namespace isl
#endif /* ISLPP_SET_H */
