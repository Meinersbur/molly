#ifndef ISLPP_ISLFWD_H
#define ISLPP_ISLFWD_H

#include <utility> // std::move


#pragma region Forward declarations for original isl
extern "C" {

  // #include <isl/ctx.h>
  struct isl_ctx;

  // #include <isl/id.h>
  struct isl_id;

  // #include <isl/map_type.h>
  struct isl_basic_map;
  struct isl_map;
  struct isl_basic_set;
  struct isl_set;

  // #include <isl/val.h>
  struct isl_val;
  struct isl_multi_val;
  struct isl_val_list;

} // extern "C"
#pragma endregion


#pragma region Forward declarations for Islpp
namespace isl {

  // Obj.h
  template<typename, typename> class Obj;

  // Spacelike.h
  template<typename> class Spacelike;

  // Dim.h
  class Dim;

  // DimRange.h
  class DimRange;

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

  // ParamSpace.h
  class ParamSpace;

  // SetSpace.h
  class SetSpace;

  // MapSpace.h
  class MapSpace;

  // LocalSpace.h
  class LocalSpace;

  // Constraint.h
  class Constraint;

  // Aff.h
  class Aff;

  // AffExpr.h
  class AffExpr;

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

  // ValList.h
  template<> class List<Val>;
  typedef List<Val> ValList;

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

  // #include <islpp/Mat.h>
  class Mat;

  // #include <islpp/Vertices.h>
  class Vertices;
} // namespace isl
#pragma endregion


#pragma region Variadic isl::product
namespace isl {
  static MultiAff product(MultiAff maff1, MultiAff maff2);
  static PwMultiAff product(PwMultiAff pma1, PwMultiAff pma2);

  //TODO: make a macro to use with several functions
  template<typename... T>
  class __product_impl;

  template<typename TFirst>
  class __product_impl<TFirst> {
  public:
    static TFirst product(TFirst arg) { return std::move(arg); }
  }; // class class __product_impl<TFirst>

  template<typename TFirst, typename TSecond>
  class __product_impl<TFirst, TSecond> {
  public:
    static auto product(TFirst arg1, TSecond arg2) -> decltype(::isl::product(std::move(arg1), std::move(arg2))) {
      return ::isl::product(std::move(arg1), std::move(arg2));
    }
  }; // class  __product_impl<TFirst, TSecond>

  template<typename TFirst, typename... T>
  class __product_impl<TFirst, T...> {
  public:
    static auto product(TFirst arg1, T... args) -> decltype(::isl::product(std::move(arg1), isl::product(args...))) {
      return ::isl::product(std::move(arg1), isl::product(args...)); // right-associative (easier to implement)
    }
  }; // class __product_impl<TFirst, T...> 

  template<typename U>
  static inline U product(U arg1) {
    return std::move(arg1);
  }

  // Do not provide a two-element overload, it has to be provided by the libraries; offering one risks endless recursion
  template<typename U, typename V, typename W, typename... T>
  static inline auto product(U arg1, V arg2, W arg3, T... args) ->decltype(__product_impl<U, V, W, T...>::product(std::move(arg1), std::move(arg2), std::move(arg3), std::move(args)...)) {
    return __product_impl<U, V, W, T...>::product(std::move(arg1), std::move(arg2), std::move(arg3), std::move(args)...);
  }
} // namespace isl
#pragma endregion

#endif /* ISLPP_ISLFWD_H */
