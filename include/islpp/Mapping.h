#ifndef ISLPP_MAPPING_H
#define ISLPP_MAPPING_H
#include "islpp_common.h"

#include "Obj.h"

#include <isl/map.h>
#include <isl/aff.h>
#include <isl/multi.h>


namespace isl {

  enum class mapping_type {
    NONE,
    //BASICMAP, represented as Map with one element
    MAP,
    //AFF, represented as MultiAff with one range dimension
    //MULTIAFF, represented as PwMultiAff with one piece on universe
    //PWAFF, represented as MultiPwAff with one range dimension
    PWMULTIAFF,
    MULTIPWAFF,
    //UNIONMAP,
    //UNIONPWMULTIAFF
  };

  /// Polymorphic for everything that maps something, without the need for virtual functions
  // TODO: Idea is to hide whether it is a PwMultiAff or MultiPwAff, removing unnecessary (and costly, since exponential time) conversions between them
  // isl::Map is therefore something else, really needed? 
  class Mapping /*: Obj<void*,Mapping>*/ {
  private:
    void *obj;

  protected:
    mapping_type getType() { return static_cast<mapping_type>(reinterpret_cast<uintptr_t>(obj)& 0x07u); }
    void *getObj() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj)& 0x07u); }
    isl_map *getMap() { assert(getType() == mapping_type::MAP); return static_cast<isl_map*>(getObj()); }
    isl_pw_multi_aff *getPwMultiAff() { assert(getType() == mapping_type::PWMULTIAFF); return static_cast<isl_pw_multi_aff*>(getObj()); }
    isl_multi_pw_aff *getMultiPwAff() { assert(getType() == mapping_type::MULTIPWAFF); return static_cast<isl_multi_pw_aff*>(getObj()); }
    //isl_union_map *getMultiPwAff() { assert(getType() == mapping_type::MULTIPWAFF); return static_cast<isl_multi_pw_aff*>(getObj()); }

    void reset() {

    }
    void reset(isl_basic_map *bmap) {

    }

    Mapping() : obj(nullptr) {}

  protected:
    void release() { 
      switch (getType()) {
      case mapping_type::MAP:
        isl_map_free(getMap());
        break;
      case mapping_type::PWMULTIAFF:
        isl_pw_multi_aff_free(getPwMultiAff());
        break;
      case mapping_type::MULTIPWAFF:
        isl_multi_pw_aff_free(getMultiPwAff());
        break;
      default:
        assert(false);
      }
    }
    //StructTy *addref() const { return isl_map_copy(keepOrNull()); }

  public:
    static Mapping enwrap(isl_basic_map *bmap) {
      Mapping result;
      result.reset(bmap);
      return result;
    }

  }; // class Mapping

} // namespace isl

#endif /* ISLPP_MAPPING_H */
