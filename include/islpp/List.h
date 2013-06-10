#ifndef ISLPP_LIST_H
#define ISLPP_LIST_H

namespace isl {
  class Set;
  class Aff;

  // Pw.h
  template<typename T>
  class Pw;

  // PwAff.h
  template<>
  class Pw<Aff>;
  typedef Pw<Aff> PwAff;
} // namespace isl


namespace isl {
  template<typename T>
  class List;

  // SetList.h
  template<>
  class List<Set>;
  typedef List<Set> SetList;

  // AffList.h
  template<>
  class List<Aff>;
  typedef List<Aff> AffList;

  // PwAffList.h
  template<>
  class List<PwAff>;
  typedef List<PwAff> PwAffList;
} // namespace isl
#endif /* ISLPP_LIST_H */
