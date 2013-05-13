#ifndef ISLPP_LIST_H
#define ISLPP_LIST_H

namespace isl {
  class Set;
  class Aff;
} // namespace isl


namespace isl {
  template<typename T>
  class List;

  template<>
  class List<Set>;

  template<>
  class List<Aff>;
} // namespace isl
#endif /* ISLPP_LIST_H */
