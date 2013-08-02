#ifndef ISLPP_ISLFWD_H
#define ISLPP_ISLFWD_H

#pragma region Forward declarations for original isl

// #include <isl/id.h>
struct isl_id;

#pragma endregion

#pragma region Forward declarations for Islpp
namespace isl {

  // Ctx.h
  class Ctx;

  // Int.h
  class Int;

  // Val.h
  class Val;

  // Id.h
  class Id;

  // Space.h
  class Space;

  // LocalSpace.h
  class LocalSpace;

  // Constraint.h
  class Constraint;

  // Aff.h
  class Aff;

  // BasicSet.h
  class BasicSet;

  // Set.h
  class Set;

  // BasicMap.h
  class BasicMap;

  // Map.h
  class Map;

  // Printer.h
  class Printer;

  // AstBuild.h
  class AstBuild;

  // AstExpr.h
  class AstExpr;

  // AstNode.h
  class AstNode;

  // Vec.h
  class Vec;

  // Point.h
  class Point;

  // Pw.h
  template<typename T> class Pw;

  // PwAff.h
  template<> class Pw<Aff>; 
  typedef Pw<Aff> PwAff;

  // List.h
  template<typename T> class List;

  // SetList.h
  template<> class List<Set>;
  typedef List<Set> SetList;

  // AffList.h
  template<> class List<Aff>;
  typedef List<Aff> AffList;

  // PwAffList.h
  template<> class List<PwAff>;
  typedef List<PwAff> PwAffList;

  // Multi.h
  template<typename T> class Multi;

  // MultiAff.h
  template<> class Multi<Aff>;
  typedef Multi<Aff> MultiAff;

  // MultiPwAff.h
  template<> class Multi<PwAff>;
  typedef Multi<PwAff> MultiPwAff;

  // MultiVal.h
  template<> class Multi<Val>;
  typedef Multi<Val> MultiVal;

  // Union.h
  template<typename T> class Union;

  // UnionSet.h
  template<> class Union<Set>;
  typedef Union<Set> UnionSet;

  // UnionMap.h
  template<> class Union<Map>;
  typedef Union<Map> UnionMap;

  // PwMultiAff.h
  template<> class Pw<MultiAff>;
  typedef Pw<MultiAff> PwMultiAff;

  // #include <UnionPwMultiAff.h>
  class UnionPwMultiAff;

  // #include <islpp/QPolynomial.h>
  class QPolynomial;

  // #include <islpp/PwQPolynomial.h>
  class PwQPolynomial;

  // #include <islpp/UnionPwQPolynomial.h>
  class UnionPwQPolynomial;

  // #include <islpp/PwQPolynomialFold.h>
  class PwQPolynomialFold;

  // #include <islpp/UnionPwQPolynomialFold.h>
  class UnionPwQPolynomialFold;

} // namespace isl
#pragma endregion

#endif /* ISLPP_ISLFWD_H */
