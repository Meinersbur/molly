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
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 1
#define ISLPP_INPLACE_QUALIFIER &
#define ISLPP_INTERNAL_QUALIFIER &&
#else
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 0
#define ISLPP_INPLACE_QUALIFIER 
#define ISLPP_INTERNAL_QUALIFIER
#endif

#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
#define ISLPP_INLINE_DECLARATION extern inline
#define ISLPP_INLINE_DEFINITION inline
#else
#define ISLPP_INLINE_DECLARATION inline
#define ISLPP_INLINE_DEFINITION extern inline
#endif

#define ISLPP_WARN_UNUSED_RESULT LLVM_ATTRIBUTE_UNUSED_RESULT
#if defined(__clang__) || defined(__GNUC__)
#define ISLPP_WARN_UNUSED_RESULT_PREFIX
#elif defined(_MSC_VER) && _MSC_VER>=1500 /*VS2008 or later*/
#define ISLPP_WARN_UNUSED_RESULT_PREFIX _Check_return_ /* Source Annotation Language (SAL) attribute syntax -- MS PREfast */
#elif defined(_MSC_VER)
#define ISLPP_WARN_UNUSED_RESULT_PREFIX __checkReturn /* Source Annotation Language (SAL) declspec syntax -- MS PREfast */
#else
#define ISLPP_WARN_UNUSED_RESULT_PREFIX
#endif

#define ISLPP_EXSITU_QUALIFIER const ISLPP_WARN_UNUSED_RESULT
#define ISLPP_CONSUME_QUALIFIER ISLPP_WARN_UNUSED_RESULT


#include <utility> // std::move, std::forward
namespace isl {
  using std::move;
  
  template<typename T>
  T copy(const T &obj) { return obj.copy(); }

  // union always means piecewise union
  template<typename T, typename U, typename V>
  T union_(T &&t, U &&u, V &&v) {
    auto imm = isl::union_(std::forward<T>(t), std::forward<U>(u));
    return isl::union_(std::move(imm), std::forward<V>(v));
  }


  /// isl returns int for anything boolean (is_empty, is_equal, etc.). On 
  /// error, it also returns -1. To avoid that this is gets silently converted 
  /// to true, insert this function
  /// In future version, we could also raise an exception
  static inline bool checkBool(int val) {
    assert(val == 0 || val == 1);
    return val;
  }

} // namespace isl
#endif /* ISLPP_ISLPP_COMMON_H */
