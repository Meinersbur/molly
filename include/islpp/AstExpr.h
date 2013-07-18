#ifndef ISLPP_ASTEXPR_H
#define ISLPP_ASTEXPR_H

#include "Obj.h"
#include <isl/ast.h>
#include "Ctx.h"


namespace isl {
  class AstExpr : public Obj3<AstExpr, isl_ast_expr> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_ast_expr_free(takeOrNull()); }

  public:
    AstExpr() { }

    /* implicit */ AstExpr(ObjTy &&that) : Obj3(std::move(that)) { }
    /* implicit */ AstExpr(const ObjTy &that) : Obj3(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

  public:
    StructTy *takeCopyOrNull() const { return isl_ast_expr_copy(keepOrNull()); }

    Ctx *getCtx() const { return Ctx::wrap(isl_ast_expr_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_ast_expr_dump(keep()); }
#pragma endregion

  }; // class AstExpr
} // namespace isl
#endif /* ISLPP_ASTEXPR_H */
