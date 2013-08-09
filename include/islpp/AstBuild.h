#ifndef ISLPP_ASTBUILD_H
#define ISLPP_ASTBUILD_H

#include "Obj.h"
#include <isl/ast_build.h>
#include "Ctx.h"
#include "Set.h"
#include "Space.h"
#include "UnionMap.h"

extern "C" {
  // From private header
  void isl_ast_build_dump(__isl_keep isl_ast_build *build);
  __isl_give isl_ast_expr *isl_ast_expr_from_aff(__isl_take isl_aff *aff, __isl_keep isl_ast_build *build);
} // extern "C"

namespace isl {
  class AstNode;
  class AstExpr;
} // namespace isl


namespace isl {
  class AstBuild : public Obj<AstBuild, isl_ast_build> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_ast_build_free(takeOrNull()); }
    StructTy *addref() const { return isl_ast_build_copy(keepOrNull()); }

  public:
    AstBuild() { }

    /* implicit */ AstBuild(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ AstBuild(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_ast_build_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_ast_build_dump(keep()); }
#pragma endregion


#pragma region Creational
    static AstBuild createFromContext(const Set &domain) { return AstBuild::enwrap(isl_ast_build_from_context(domain.takeCopy())); } 
#pragma endregion


#pragma region Properties
    //Set getDomain() const { return Set::enwrap(isl_ast_build_set_domain ); }

    Space getScheduleSpace() const { return Space::enwrap(isl_ast_build_get_schedule_space(keep())); }
    UnionMap getSchedule() const { return UnionMap::enwrap(isl_ast_build_get_schedule(keep())); }
#pragma endregion

    AstExpr exprFromAff(const Aff &aff) const;
    AstExpr exprFromPwAff(const PwAff &paff) const;
    AstExpr callFromPwMultiAff(const PwMultiAff &pmaff) const;
    AstNode astFromSchedule(const UnionMap &schedule) const;

  }; // class AstBuild
} // namespace isl
#endif /* ISLPP_ASTBUILD_H */
