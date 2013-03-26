#ifndef ISLPP_UNION_H
#define ISLPP_UNION_H

namespace isl {
  class Map;
  class Set;
} // namespace isl


namespace isl {
  template<typename T>
  class Union;

  template<> class Union<Set>;
  template<> class Union<Map>;
} // namespace isl
#endif /* ISLPP_UNION_H */
