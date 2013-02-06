#ifndef ISLPP_SET_H
#define ISLPP_SET_H

#include <llvm/Support/Compiler.h>
#include <cassert>
#include <string>

#include "islpp/Int.h"


struct isl_set;
struct isl_basic_set;
enum isl_dim_type;
enum isl_lp_result;
struct isl_point;


namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Space;
  class Constraint;
  class BasicSet;
  class Ctx;
  class PwAff;
  class PwMultiAff;
  class Id;
  class Aff;
  class Map;
  class MultiAff;
  class Point;
  class MultiPwAff;
  class PwQPolynomialFold;
} // namespace isl


namespace isl {

  typedef int (*BasicSetCallback)(isl_basic_set *bset, void *user);
  typedef int (*PointCallback)(isl_point *pnt, void *user);

  class Set {
#pragma region Low-level functions
  private:
    isl_set *set;

  public: // Public because otherwise we had to add a lot of friends
    isl_set *take() { assert(set); isl_set *result = set; set = nullptr; return result; }
    isl_set *takeCopy() const;
    isl_set *keep() const { return set; }
    isl_set **change() { return &set; }
  protected:
    void give(isl_set *set);

    explicit Set(isl_set *set) : set(set) { }
  public:
    static Set wrap(isl_set *set) { return Set(set); }
#pragma endregion

  public:
    Set(void) : set(nullptr) {}
    Set(const Set &that) : set(that.takeCopy()) {}
    Set(Set &&that) : set(that.take()) { }
    ~Set(void);

    const Set &operator=(const Set &that) { give(that.takeCopy()); return *this; }
    const Set &operator=(Set &&that) { give(that.take()); return *this; }

    /* implicit */ Set(BasicSet &&set);
    const Set &operator=(const BasicSet &that);

    static Set createEmpty(Space &&space);
    /// @brief Contains all integers
    static Set createUniverse(Space &&space);
    /// @brief Contains all non-negative integers
    static Set createNatUniverse(Space &&space);
    /// @brief A zero-dimensional (basic) set can be constructed on a given parameter domain using the following function.
    static Set createFromParams(Set &&set);
    static Set createFromPwAff(PwAff &&);
    static Set createFromPwMultiAff(PwMultiAff &&);
    static Set createFromPoint(Point &&);
    static Set createBocFromPoints(Point &&pnt1, Point &&pnt2);

    static Set readFrom(const Ctx &, FILE *);
    static Set readFrom(const Ctx &, const char *);

    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;

    Space getSpace() const;

    Set copy() const { return Set::wrap(takeCopy()); }

    Set addContraint(Constraint &&) const;

    Set complement() const;
    Set projectOut(isl_dim_type type, unsigned first, unsigned n) const;
    Set params() const;

    /// Return false on success of true in case of any error
    /// The callback function fn should return 0 if successful and -1 if an error occurs. In the latter case, or if any other error occurs, the above functions will return -1.
    bool foreachBasicSet(BasicSetCallback, void *user) const;
    bool foreachPoint(PointCallback, void *user) const;

    int getBasicSetCount() const;

    unsigned getDim(isl_dim_type) const;
    int getInvolvedDims(isl_dim_type,unsigned first, unsigned n) const;
    bool dimHasAnyLowerBound(isl_dim_type, unsigned pos) const;
    bool dimHasAnyUpperBound(isl_dim_type, unsigned pos) const;
    bool dimHasLowerBound(isl_dim_type, unsigned pos) const;
    bool dimHasUpperBound(isl_dim_type, unsigned pos) const;

    void setTupleId(Id&&);
    void resetTupleId();
    bool hasTupleId() const;
    Id getTupleId() const;
    bool hasTupleName() const;
    const char *getTupleName() const;

    void setDimId(isl_dim_type type, unsigned pos, Id &&id);
    bool hasDimId(isl_dim_type type, unsigned pos) const;
    Id getDimId(isl_dim_type type, unsigned pos) const;
    int findDimById(isl_dim_type type, const Id &id) const;
    int findDimByName(isl_dim_type type, const char *name) const;
    bool dimHasName(isl_dim_type type, unsigned pos) const;
    const char *getDimName(enum isl_dim_type type, unsigned pos) const;

    bool plainIsEmpty() const;
    bool isEmpty() const;
    bool plainIsUniverse() const;
    /// @brief Check if the relation obviously lies on a hyperplane where the given dimension has a fixed value and if so, return that value in *val.
    bool plainIsFixed(isl_dim_type type, unsigned pos, Int &val) const;

    /// Eliminate the coefficients for the given dimensions from the constraints, without removing the dimensions.
    void eliminate( isl_dim_type type, unsigned first, unsigned n);

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
  }; // class Set


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
  Set union_(Set &&set1, Set &&set2);
  Set subtract(Set &&set1, Set &&set2);
  Set apply(Set &&set, Map &&map);
  Set preimage(Set &&set, MultiAff &&ma);
  Set preimage(Set &&set, PwMultiAff &&ma);
  /// @brief Cartesian product
  /// The above functions compute the cross product of the given sets or relations. The domains and ranges of the results are wrapped maps between domains and ranges of the inputs.
  Set product(Set &&set1, Set &&set2);
  Set flatProduct(Set &&set1,Set &&set2);
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

} // namespace isl
#endif /* ISLPP_SET_H */