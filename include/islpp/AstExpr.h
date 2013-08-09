#ifndef ISLPP_ASTEXPR_H
#define ISLPP_ASTEXPR_H

#include "Obj.h"
#include <isl/ast.h>
#include "Ctx.h"


namespace isl {
  class AstExpr : public Obj<AstExpr, isl_ast_expr> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_ast_expr_free(takeOrNull()); }
    StructTy *addref() const { return isl_ast_expr_copy(keepOrNull()); }

  public:
    AstExpr() { }

    /* implicit */ AstExpr(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ AstExpr(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_ast_expr_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_ast_expr_dump(keep()); }
#pragma endregion

  }; // class AstExpr
} // namespace isl
#endif /* ISLPP_ASTEXPR_H */
