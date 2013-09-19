#ifndef ISLPP_ISLPP_COMMON_H
#define ISLPP_ISLPP_COMMON_H

#include <llvm/Support/Compiler.h> // LLVM_FINAL, LLVM_OVERRIDE
#include <type_traits> // std::remove_reference
//#include <llvm/Support/CommandLine.h>
#include <cassert>

namespace llvm {
  template<typename> class ArrayRef;
} // namespace llvm

#ifdef __GNUC__
// Ignore #pragma region
//TODO:  #pragma GCC diagnostic pop
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif

#if __has_extension(cxx_reference_qualified_functions) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 8) || (_GNUC__ == 4 && __GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ >= 1)
// Supported since GCC 8.4.1 (http://gcc.gnu.org/projects/cxx0x.html)
#define ISLPP_LVALUE_THIS_QUALIFIER &
#define ISLPP_RVALUE_THIS_QUALIFIER &&
#define ISLPP_HAS_LVALUE_THIS_QUALIFIER 1
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 1
#else
#define ISLPP_LVALUE_THIS_QUALIFIER 
#define ISLPP_RVALUE_THIS_QUALIFIER 
#define ISLPP_HAS_LVALUE_THIS_QUALIFIER 0
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 0
#endif

#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
#define ISLPP_INLINE_DECLARATION extern inline
#define ISLPP_INLINE_DEFINITION inline
#else
#define ISLPP_INLINE_DECLARATION inline
#define ISLPP_INLINE_DEFINITION extern inline
#endif


#if defined(_MSC_VER) && _MSC_VER>=1500 /*VS2008 or later*/
#define ISLPP_WARN_UNUSED_RESULT _Check_return_ /* Source Annotation Language (SAL) attribute syntax -- MS PREfast */
#elif defined(_MSC_VER)
#define ISLPP_WARN_UNUSED_RESULT __checkReturn /* Source Annotation Language (SAL) declspec syntax -- MS PREfast */
#else
#define ISLPP_WARN_UNUSED_RESULT LLVM_ATTRIBUTE_UNUSED_RESULT
#endif


#define ISLPP_EXSITU_PREFIX ISLPP_WARN_UNUSED_RESULT
#define ISLPP_EXSITU_QUALIFIER const

#define ISLPP_INPLACE_PREFIX
#define ISLPP_INPLACE_QUALIFIER ISLPP_LVALUE_THIS_QUALIFIER

#define ISLPP_CONSUME_PREFIX ISLPP_WARN_UNUSED_RESULT
#define ISLPP_CONSUME_QUALIFIER 


#include <utility> // std::move, std::forward
namespace isl {
  using std::move;
  
  // ISL uses mostly unsigned int for anything that cannot be negative, although not consistently
  // This poses problems when sometimes negative values are returned, e.g. in case of an error or just because of inconsistency. 
  // Some compilers warn (rightfully) about mixing signed and unsigned types
  // Therefore keep in mind that only value range 0..MAX_INT work reliable, you won't use more dimensions anyways
  // For number of dimensions (count_t), size_t would probably have made more sense
  typedef unsigned int pos_t;
  typedef unsigned int count_t;


  template<typename T>
  T copy(const T &obj) { return obj.copy(); }

  /// union always means piecewise union
  /// here, called unite because union is a C++ keyword
  template<typename T, typename U, typename V>
  T unite(T &&t, U &&u, V &&v) {
    auto imm = isl::unite(std::forward<T>(t), std::forward<U>(u));
    return isl::unite(std::move(imm), std::forward<V>(v));
  }


  /// isl returns int for anything boolean (is_empty, is_equal, etc.). On 
  /// error, it also returns -1. To avoid that this is gets silently converted 
  /// to true, insert this function
  /// In future version, we could also raise an exception
  static inline bool checkBool(int val) {
    assert(val == 0 || val == 1);
    return val;
  }


  /// Some isl functions do not use unsigned int, this is a check to not miss anything
  static inline count_t to_count_t(int n) { assert(n>=0); return n; } 
  static inline count_t to_count_t(count_t n) { return n; }
 
  

} // namespace isl
#endif /* ISLPP_ISLPP_COMMON_H */
