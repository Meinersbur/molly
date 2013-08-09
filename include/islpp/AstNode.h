#ifndef ISLPP_ASTNODE_H
#define ISLPP_ASTNODE_H

#include "Obj.h"
#include <isl/ast.h>
#include "Ctx.h"


namespace isl {
  class AstNode : public Obj<AstNode, isl_ast_node> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_ast_node_free(takeOrNull()); }
    StructTy *addref() const { return isl_ast_node_copy(keepOrNull()); }

  public:
    AstNode() { }

    /* implicit */ AstNode(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ AstNode(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_ast_node_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_ast_node_dump(keep()); }
#pragma endregion

  }; // class AstNode
} // namespace isl
#endif /* ISLPP_ASTNODE_H */
