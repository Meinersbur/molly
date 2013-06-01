#ifndef ISLPP_ISLPP_COMMON_H
#define ISLPP_ISLPP_COMMON_H

#include <type_traits> // std::remove_reference
//#include <llvm/Support/CommandLine.h>

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

#if __has_extension(cxx_reference_qualified_functions) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 8) || (_GNUC__ == 4 && __GNUC_MINOR__ > 8 && __GNUC_PATCHLEVEL__ >= 1)
// Supported since GCC 8.4.1 (http://gcc.gnu.org/projects/cxx0x.html)
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 1
#define ISLPP_INPLACE_QUALIFIER &
#else
#define ISLPP_HAS_RVALUE_THIS_QUALIFIER 0
#define ISLPP_INPLACE_QUALIFIER 
#endif

namespace isl {
  using std::move;
  
  template<typename T>
  T copy(const T &obj) { return obj.copy(); }
} // namespace isl

#endif /* ISLPP_ISLPP_COMMON_H */
