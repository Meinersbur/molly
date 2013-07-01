#ifndef ISLPP_DIM_H
#define ISLPP_DIM_H

#include "islpp_common.h"
//#include <cassert>
#include <isl/space.h> // enum isl_dim_type
#include "Id.h" // class Id (member of isl::Dim)
#include <llvm/Support/ErrorHandling.h>
#include <isl/space.h>
#include <isl/local_space.h>

struct isl_space;
struct isl_local_space;
struct isl_map;
struct isl_basic_map;
struct isl_set;
struct isl_basic_set;

namespace isl {
  class Spacelike;
  class Id;
  class LocalSpace;
  class Space;
} // namespace isl


namespace isl {

  namespace DimTypeFlags {
    enum DimTypeFlags {
      Cst = (1 << isl_dim_cst),
      Param = (1 << isl_dim_param),
      In = (1 << isl_dim_in),
      Out = (1 << isl_dim_out),
      Set = (1 << isl_dim_set),
      Div = (1 << isl_dim_div),
      All = (Cst | Param | In | Out | Set | Div)
    }; // enum DimTypeFlags
  } // namespace DimTypeFlags
  typedef enum DimTypeFlags::DimTypeFlags DimType;
#if 0
  namespace DimType {
    static const DimTypeFlags Cst = DimTypeCst;
    static const DimTypeFlags Param = DimTypeParam;
    static const DimTypeFlags Out = DimTypeOut;
    static const DimTypeFlags Set = DimTypeSet;
    static const DimTypeFlags Div = DimTypeDiv;
    static const DimTypeFlags All = DimTypeAll;
  }
#endif


  /// Encapsulates a (isl_dim_type,pos)-pair or and isl_id
  /// Also knows what tuple it is part of
  class Dim {
  private:
    isl_dim_type type;
    unsigned pos;

#if 1
  private:
    //TODO: llvm::PointerUnion
    isl_space *space;
    isl_local_space *localspace;

  protected:
    Dim(isl_space *space, isl_dim_type type, unsigned pos) : space(space), localspace(nullptr), type(type), pos(pos) {
      assert(space || localspace);
    }
    Dim(isl_local_space *localspace, isl_dim_type type, unsigned pos) : space(nullptr), localspace(localspace), type(type), pos(pos) {
       assert(space || localspace);
    }

  public:
    bool hasId() const {  
      if (space) 
        return isl_space_has_dim_id(space, type, pos);
      if (localspace)
        return isl_local_space_has_dim_id(localspace, type, pos);
      llvm_unreachable("Null dim");
    }

    Id getId() const {
      if (space)
        return Id::enwrap(isl_space_get_dim_id(space, type, pos));
      if (localspace) 
        return Id::enwrap(isl_local_space_get_dim_id(localspace, type, pos));
      llvm_unreachable("Null dim");
    }


    const char *getName() const { 
      if (space)
        return isl_space_get_dim_name(space, type, pos);
      if (localspace) 
        return isl_local_space_get_dim_name(localspace, type, pos);
      llvm_unreachable("Null dim");
    } 

    unsigned getTupleDims() const { 
      if (space)
        return isl_space_dim(space, type);
      if (localspace) 
        return isl_local_space_dim(localspace, type);
      llvm_unreachable("Null dim");
    }
#else
  private:
    Id dimId;

    unsigned tupleDims;
    Id tupleId;

  protected:
    Dim(isl_dim_type type, unsigned dimPos, const Id dimId, unsigned tupleDims, const Id &tupleId) : type(type), dimPos(dimPos), dimId(dimId), tupleDims(tupleDims), tupleId(tupleId) {
      assert(dimPos < tupleDims);
    }

  public:
    bool hasId() const { return dimId.isValid(); }
    Id getId() const { return dimId; }
    const char *getName() const { return dimId.getName(); } 
    unsigned getTupleDims() const { return tupleDims; }
#endif



  public:
    Dim(): type(isl_dim_cst/*Constant dim not valid*/) {}

    static Dim enwrap(Space space, isl_dim_type type, unsigned pos) ;
    static Dim enwrap(LocalSpace localspace, isl_dim_type type, unsigned pos);


  public:
    isl_dim_type getType() const { return type; }
    unsigned getPos() const { return pos; }

    bool isNull() const { return type==isl_dim_cst; }
    bool isValid() const { return type!=isl_dim_cst; }

    bool isConstantDim() const { return type == isl_dim_cst; }
    bool isParamDim() const { return type == isl_dim_param; }
    bool isInDim() const { return type == isl_dim_in; }
    bool isOutDim() const { return type == isl_dim_out; }
    bool isSetDim() const { return type == isl_dim_set; }
    bool isDivDim() const { return type == isl_dim_div; }
  }; // class Dim

} // namespace isl
#endif /* ISLPP_DIM_H */
