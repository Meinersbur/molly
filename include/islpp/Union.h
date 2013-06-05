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
  typedef Union<Set> UnionSet;

  template<> class Union<Map>;
  typedef Union<Map> UnionMap;
} // namespace isl
#endif /* ISLPP_UNION_H */
