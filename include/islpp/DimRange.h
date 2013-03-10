#ifndef ISLPP_DIMRANGE_H
#define ISLPP_DIMRANGE_H

#include <isl/space.h> // enum isl_dim_type

struct isl_space;
struct isl_local_space;
struct isl_map;
struct isl_basic_map;
struct isl_set;
struct isl_basic_set;


namespace isl {

    class DimRange {
  protected:
        union {
      isl_space *space;
      isl_local_space *lspace;
      isl_map *map;
      isl_basic_map *bmap;
      isl_set *set;
      isl_basic_set *bset;
    } owner;
    isl_dim_type type;
    unsigned pos;
    unsigned n;
  }; // class DimRange


} // namespace isl

#endif /* ISLPP_DIMRANGE_H */