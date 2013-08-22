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

#ifdef __GNUC__
    #define WARN_IF_UNUSED __attribute__((warn_unused_result))
#else
    #define WARN_IF_UNUSED
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
