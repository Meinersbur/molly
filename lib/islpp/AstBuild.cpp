#include "islpp_impl_common.h"
#include "islpp/AstBuild.h"

#include "islpp/AstExpr.h"
#include "islpp/AstNode.h"
#include "islpp/PwMultiAff.h"
#include "islpp/UnionMap.h"


void  AstBuild::print(llvm::raw_ostream &out) const { 
  out << "AstBuild";
}


AstExpr AstBuild::exprFromAff(const Aff &aff) const {
  return AstExpr::enwrap(isl_ast_expr_from_aff(aff.takeCopy(), keep()));
}


AstExpr AstBuild::exprFromPwAff(const PwAff &paff) const {
  return AstExpr::enwrap(isl_ast_build_expr_from_pw_aff(keep(), paff.takeCopy()));
}


AstExpr AstBuild::callFromPwMultiAff(const PwMultiAff &pmaff) const { 
  return AstExpr::enwrap(isl_ast_build_call_from_pw_multi_aff(keep(), pmaff.takeCopy()));
}


AstNode AstBuild::astFromSchedule(const UnionMap &schedule) const {
  return AstNode::enwrap(isl_ast_build_ast_from_schedule(keep(), schedule.takeCopy()));
}
