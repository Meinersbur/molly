#ifndef ISLPP_MULTI_H
#define ISLPP_MULTI_H

#pragma region Forward declarations
namespace isl {
  // Aff.h
  class Aff;

  // Val.h
  class Val;

  // Pw.h
  template<typename> 
  class Pw;
  
  // PwAff.h
  template<> 
  class Pw<Aff>;
  typedef Pw<Aff> PwAff;
} // namespace isl
#pragma endregion


namespace isl {
  template<typename T>
  class Multi;

  template<> 
  class Multi<Aff>;
  typedef Multi<Aff> MultiAff;

  template<> 
  class Multi<PwAff>;
  typedef Multi<PwAff> MultiPwAff;

  template<> 
  class Multi<Val>;
  typedef Multi<Val> MultiVal;
} // namespace isl
#endif /* ISLPP_MULTI_H */
