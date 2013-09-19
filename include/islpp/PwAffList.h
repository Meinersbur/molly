#ifndef ISLPP_PWAFFLIST_H
#define ISLPP_PWAFFLIST_H

#include "islpp_common.h"
#include "List.h"
#include "Obj.h"
#include <isl/list.h>
#include <isl/aff_type.h>
#include "Ctx.h"
#include "PwAff.h"


namespace isl {

  template<>
  class List<PwAff> : public Obj<PwAffList, isl_pw_aff_list> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_pw_aff_list_free(takeOrNull()); }
    StructTy *addref() const { return isl_pw_aff_list_copy(keepOrNull()); }

  public:
    List() { }

    /* implicit */ List(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ List(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_pw_aff_list_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_pw_aff_list_dump(keep()); }
#pragma endregion

    static PwAffList alloc(Ctx *ctx, int n) { return  PwAffList::enwrap(isl_pw_aff_list_alloc(ctx->keep(), n) ); }
    static PwAffList fromPwAff(const PwAff &paff) { return PwAffList::enwrap(isl_pw_aff_list_from_pw_aff(paff.takeCopy())); }

    void add_inplace(const PwAff &paff) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_list_add(take(), paff.takeCopy())); }
    PwAffList add(const PwAff &paff) const { return PwAffList::enwrap(isl_pw_aff_list_add(takeCopy(), paff.takeCopy())); }

    void insert_inplace(pos_t pos, const PwAff &paff) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_list_insert(take(), pos, paff.takeCopy())); }
    PwAffList insert(pos_t pos, const PwAff &paff) const { return PwAffList::enwrap(isl_pw_aff_list_insert(takeCopy(), pos, paff.takeCopy())); }

    void drop_inplace(unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_list_drop(take(), first, count)); }
    PwAffList drop(unsigned first, unsigned count) const { return PwAffList::enwrap(isl_pw_aff_list_drop(takeCopy(), first, count)); }

    void concat_inplace(const PwAffList &rhs) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_list_concat(take(), rhs.takeCopy())); }
    PwAffList concat(const PwAffList &rhs) const { return PwAffList::enwrap(isl_pw_aff_list_concat(takeCopy(), rhs.takeCopy())); }

    PwAff getPwAff(pos_t index) const { return PwAff::enwrap(isl_pw_aff_list_get_pw_aff(keep(), index)); }

    void setPwAff_inplace(pos_t index, PwAff &paff) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_list_set_pw_aff(take(), index, paff.takeCopy())); }
    PwAffList setPwAff(pos_t index, PwAff &paff) const { return PwAffList::enwrap(isl_pw_aff_list_set_pw_aff(takeCopy(), index, paff.takeCopy())); }

    bool foreachPwAff(std::function<bool(PwAff)> func) const;
    std::vector<PwAff> getPwAffs() const;

    void sort_inplace(const std::function<int(const PwAff&, const PwAff&)> &func) ISLPP_INPLACE_QUALIFIER;
    PwAffList sort(const std::function<int(const PwAff&, const PwAff&)> &func) const { auto result = copy(); result.sort_inplace(func); return result; }

    bool foreachScc(const std::function<bool(const PwAff &, const PwAff &)> &followsFunc, const std::function<bool(PwAffList)> &func) const;
  }; // class List<PwAff> 


  static inline PwAffList enwrap(isl_pw_aff_list *obj) { return PwAffList::enwrap(obj); }

  static inline PwAffList concat(const PwAffList &lhs, const PwAffList &rhs) ISLPP_INPLACE_QUALIFIER { return PwAffList::enwrap(isl_pw_aff_list_concat(lhs.takeCopy(), rhs.takeCopy())); }

} // namespace isl
#endif /* ISLPP_PWAFFLIST_H */
