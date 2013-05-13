#ifndef ISLPP_MULTI_H
#define ISLPP_MULTI_H

namespace isl {
  class Aff;
} // namespace isl


namespace isl {
  template<typename T>
  class Multi;

  template<>
  class Multi<Aff>;
  typedef Multi<Aff> MultiAff;

} // namespace isl
#endif /* ISLPP_MULTI_H */
