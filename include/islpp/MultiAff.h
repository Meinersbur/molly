#ifndef ISLPP_MULTIAFF_H
#define ISLPP_MULTIAFF_H

#include <cassert>
#include <isl/aff.h>

#include "islpp/Multi.h"
#include "islpp/Aff.h"
#include "islpp/Space.h"
#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Vec.h"
#include "islpp/Int.h"
#include "islpp/Set.h"
#include "islpp/LocalSpace.h"
#include "islpp/Spacelike.h"

#include <llvm/Support/ErrorHandling.h>

struct isl_multi_aff;

namespace llvm {
} // namespace llvm

namespace isl {
  class Aff;
} // namespace isl


namespace isl {
  typedef Multi<Aff> MultiAff;

  template<>
  class Multi<Aff> : public Spacelike {
#pragma region Low-level
  private:
    isl_multi_aff *maff;

  public: // Public because otherwise we had to add a lot of friends
    isl_multi_aff *take() { assert(maff); isl_multi_aff *result = maff; maff = nullptr; return result; }
    isl_multi_aff *takeCopy() const;
    isl_multi_aff *keep() const { return maff; }
  protected:
    void give(isl_multi_aff *aff);

    explicit Multi(isl_multi_aff *maff) : maff(maff) { }
  public:
    static Multi<Aff> wrap(isl_multi_aff *maff) { return Multi<Aff>(maff); }
#pragma endregion

  public:
    Multi<Aff>() : maff(nullptr) {}
    Multi(const Multi<Aff> &that) : maff(that.takeCopy()) {}
    Multi(Multi<Aff> &&that) : maff(that.take()) {}
    ~Multi();

    const Multi<Aff> &operator=(const Multi<Aff> &that) { give(that.takeCopy()); return *this; }
    const Multi<Aff> &operator=(Multi<Aff> &&that) { give(that.take()); return *this; }

    Multi<Aff> copy() const { return wrap(takeCopy()); }

    Ctx *getCtx() const { return Ctx::wrap(isl_multi_aff_get_ctx(keep())); }
    Space getSpace() const { return Space::wrap(isl_multi_aff_get_space(keep())); }
    Space getDomainSpace() const { return Space::wrap(isl_multi_aff_get_domain_space(keep())); }

#pragma region Creational
    static Multi<Aff> fromAff(Aff &&aff) { return wrap(isl_multi_aff_from_aff(aff.take())); }
    static Multi<Aff> createZero(Space &&space) { return wrap(isl_multi_aff_zero(space.take())); }
    static Multi<Aff> createIdentity(Space &&space) { return wrap(isl_multi_aff_identity(space.take())); }

    static Multi<Aff> readFromString(Ctx *ctx, const char *str) { return wrap(isl_multi_aff_read_from_str(ctx->keep(), str)); } 
#pragma endregion

#pragma region Dimensions
    unsigned dim(enum isl_dim_type type) const { return isl_multi_aff_dim(keep(), type); }

    //bool hasTupleName(isl_dim_type type) const { return isl_multi_aff_has_tuple_name(keep(), type); } 
    const char *getTupleName(isl_dim_type type) const { return isl_multi_aff_get_tuple_name(keep(), type); }
    void setTupleName(isl_dim_type type, const char *s) { give(isl_multi_aff_set_tuple_name(take(), type, s)); }

    //bool hasTupleId(isl_dim_type type) const { return isl_multi_aff_has_tuple_id(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::wrap(isl_multi_aff_get_tuple_id(keep(), type)); }
    void setTupleId(isl_dim_type type, Id &&id) { give(isl_multi_aff_set_tuple_id(take(), type, id.take())); }

    //bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_name(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_get_dim_name(keep(), type, pos); }
    void setDimName(isl_dim_type type, unsigned pos, const char *s) { give(isl_multi_aff_set_dim_name(take(), type, pos, s)); }
    //int findDimByName(isl_dim_type type, const char *name) const { return isl_multi_aff_find_dim_by_name(keep(), type, name); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_id(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::wrap(isl_multi_aff_get_dim_id(keep(), type, pos)); }
    //void setDimId(isl_dim_type type, unsigned pos, Id &&id) { give(isl_multi_aff_set_dim_id(take(), type, pos, id.take())); }
    void setDimId(isl_dim_type type, unsigned pos, Id &&id) { llvm_unreachable("API function missing"); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    void addDims(isl_dim_type type, unsigned n) { give(isl_multi_aff_add_dims(take(), type, n)); }
    void insertDims(isl_dim_type type, unsigned pos, unsigned n) { give(isl_multi_aff_insert_dims(take(), type, pos, n)); }
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { give(isl_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n)); }
    void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { llvm_unreachable("API function missing"); }
    void dropDims(isl_dim_type type, unsigned first, unsigned n) { give(isl_multi_aff_drop_dims(take(), type, first, n)); }
    
    void removeDims(isl_dim_type type, unsigned first, unsigned n) { dropDims(type, first, n); }
#pragma endregion


#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    Aff getAff(int pos) const { return Aff::wrap(isl_multi_aff_get_aff(keep(), pos)); }
#pragma endregion

    void scale(Int f) { give(isl_multi_aff_scale(take(), f.keep())); };
    void scaleVec(Vec &&v) { give(isl_multi_aff_scale_vec(take(), v.take())); }

    void alignParams(Space &&model) { give(isl_multi_aff_align_params(take(), model.take())); }
    void gistParams(Set &&context) { give(isl_multi_aff_gist_params(take(), context.take())); }
    void gist(Set &&context) { give(isl_multi_aff_gist(take(), context.take())); }
     void lift() { give(isl_multi_aff_lift(take(), nullptr)); }
     LocalSpace lift(LocalSpace &context) { 
       isl_local_space *ls = nullptr;
       give(isl_multi_aff_lift(take(), &ls));
       return LocalSpace::wrap(ls);
     }

  }; // class MultiAff

  bool plainIsEqual(const Multi<Aff> &maff1, const Multi<Aff> &maff2) { return isl_multi_aff_plain_is_equal(maff1.keep(), maff2.keep()); }
  Multi<Aff> add(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_add(maff1.take(), maff2.take())); }
  Multi<Aff> sub(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_sub(maff1.take(), maff2.take())); }
  
  Multi<Aff> rangeSplice(Multi<Aff> &&maff1, unsigned pos, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_range_splice(maff1.take(), pos, maff2.take())); }
  Multi<Aff> splice(Multi<Aff> &&maff1, unsigned in_pos, unsigned out_pos, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_splice(maff1.take(), in_pos, out_pos, maff2.take())); }

  Multi<Aff> rangeProduct(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_range_product(maff1.take(), maff2.take())); }
  Multi<Aff> flatRangeProduct(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_flat_range_product(maff1.take(), maff2.take())); }
  Multi<Aff> product(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_product(maff1.take(), maff2.take())); }
  
  Multi<Aff> pullbackMultiAff(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::wrap(isl_multi_aff_pullback_multi_aff(maff1.take(), maff2.take())); }
  
  Set lexLeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::wrap(isl_multi_aff_lex_le_set(maff1.take(), maff2.take())); }
  Set lexGeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::wrap(isl_multi_aff_lex_ge_set(maff1.take(), maff2.take())); }
} // namespace isl
#endif /* ISLPP_MULTIAFF_H */
