#ifndef ISLPP_DIM_H
#define ISLPP_DIM_H

#include "islpp_common.h"
#include <cassert>
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

#if 0
  class Dim {
  private:
    // Identifying the dimension
    isl_dim_type type;
    unsigned pos;
    Id dimId;

    // About the space it is contained in
    unsigned count;
    Id tupleId;

    Dim(isl_dim_type type, unsigned pos, Id &&dimId, unsigned count, Id &&tupleId)
      : type(type), pos(pos), dimId(std::move(dimId)), count(count), tupleId(std::move(tupleId)) {
        assert(pos < count);
    }

  public:
    Dim(): type(isl_dim_cst/*Constant dim is reserved for internal use*/) {}

    static Dim enwrap(const Space &space, isl_dim_type type, unsigned pos);
    static Dim enwrap(const LocalSpace &localspace, isl_dim_type type, unsigned pos);

    isl_dim_type getType() const { return type; }
    unsigned getPos() const { return pos; }
    bool hasId() const { return dimId.isValid(); }
    Id getId() const { assert(hasId()); return dimId; }
    Id getIdOrNull() const { return dimId; }

    bool isNull() const { return type==isl_dim_cst; }
    bool isValid() const { return type!=isl_dim_cst; }

    bool isConstantDim() const { return type == isl_dim_cst; }
    bool isParamDim() const { return type == isl_dim_param; }
    bool isInDim() const { return type == isl_dim_in; }
    bool isOutDim() const { return type == isl_dim_out; }
    bool isSetDim() const { return type == isl_dim_set; }
    bool isDivDim() const { return type == isl_dim_div; }

    const char *getName() const { return dimId.getName(); }
    unsigned getTupleDims() const { return count; }
  }; // class Dim

#else
  /// Encapsulates a (isl_dim_type,pos)-pair or and isl_id
  /// Also knows what tuple it is part of
  /// May also have a LocalDim type
  class Dim {
  private:
    isl_dim_type type;
    unsigned pos;
    isl_space *space;

    Dim(isl_dim_type type, unsigned pos, __isl_take isl_space *space) : type(type), pos(pos), space(space) {
      assert(pos < isl_space_dim(space, type));
    }

  public:
    Dim(): type(isl_dim_cst/*Constant dim not valid*/), space(nullptr) {}
    /* implicit */ Dim(const Dim &that) : type(that.type), pos(that.pos), space(isl_space_copy(that.space)) { }
    /* implicit */ Dim(Dim &&that) : type(that.type), pos(that.pos), space(that.space) { that.space = nullptr; }
    ~Dim() { isl_space_free(space); space = NULL; }

    const Dim &operator=(const Dim &that) { this->type = that.type; this->pos = that.pos; auto old = this->space; this->space = isl_space_copy(that.space); isl_space_free(old); return *this; }
    const Dim &operator=(Dim &&that) { this->type = that.type; this->pos = that.pos; std::swap(this->space, that.space); return *this; }

    static Dim enwrap(isl_dim_type type, unsigned pos, Space space);
    static Dim enwrap(isl_dim_type type, unsigned pos, isl_space *space);

  public:
    bool isNull() const { return type==isl_dim_cst; }
    bool isValid() const { return type!=isl_dim_cst; }

    bool hasId() const { return isl_space_has_dim_id(space, type, pos); }
    Id getId() const { return Id::enwrap(isl_space_get_dim_id(space, type, pos)); }
    const char *getName() const { return isl_space_get_dim_name(space, type, pos);  } 
    unsigned getTupleDims() const { return isl_space_dim(space, type); }

    isl_dim_type getType() const { return type; }
    unsigned getPos() const { return pos; }

    bool isConstantDim() const { return type == isl_dim_cst; }
    bool isParamDim() const { return type == isl_dim_param; }
    bool isInDim() const { return type == isl_dim_in; }
    bool isOutDim() const { return type == isl_dim_out; }
    bool isSetDim() const { return type == isl_dim_set; }
    bool isDivDim() const { return type == isl_dim_div; }
  }; // class Dim
#endif

} // namespace isl
#endif /* ISLPP_DIM_H */
