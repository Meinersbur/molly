#ifndef ISLPP_MULTI_H
#define ISLPP_MULTI_H

namespace isl {
  // Aff.h
  class Aff;

  // Pw.h
  template<typename> 
  class Pw;
  
  // PwAff.h
  template<> 
  class Pw<Aff>;
  typedef Pw<Aff> PwAff;
} // namespace isl


namespace isl {
  template<typename T>
  class Multi;

  template<> 
  class Multi<Aff>;
  typedef Multi<Aff> MultiAff;

  template<> 
  class Multi<PwAff>;
  typedef Multi<PwAff> MultiPwAff;
} // namespace isl
#endif /* ISLPP_MULTI_H */
