#ifndef ISLPP_PW_H
#define ISLPP_PW_H

namespace isl {
  // Aff.h
  class Aff;

  // Multi.h
  template<typename>
  class Multi;

  // MultiAff.h
  template<>
  class Multi<Aff>;
  typedef Multi<Aff> MultiAff;
} // namespace isl


namespace isl {
  template<typename T>
  class Pw;

  // PwAff.h
  template<>
  class Pw<Aff>;
  typedef Pw<Aff> PwAff;

  // PwMultiAff.h
  template<>
  class Pw<MultiAff>;
  typedef Pw<MultiAff> PwMultiAff;
} // namepsace isl

#endif /* ISLPP_PW_H */
