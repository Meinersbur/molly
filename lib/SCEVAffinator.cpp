// Mostly copied from Polly's ScopInfo.cpp

#include "SCEVAffinator.h"

#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <isl/ctx.h>
#include <isl/aff.h>
#include <isl/set.h>
#include <polly/ScopInfo.h>
#include <polly/Support/SCEVValidator.h>
#include <polly/Support/GICHelper.h>
#include <llvm/Analysis/LoopInfo.h>
#include "islpp/PwAff.h"

using namespace llvm;
using namespace polly;

struct isl_pw_aff;


/// Translate a SCEVExpression into an isl_pw_aff object.
struct Affinator : public SCEVVisitor<Affinator, isl_pw_aff *> {
private:
  isl_ctx *Ctx;
  int NbLoopSpaces;
  const Scop *S;
  isl_space *space;

  isl_space *getInSpace() {
    return isl_space_copy(space);
    //auto result = inStmt->getDomainSpace();
    //assert(isl_space_dim(result, isl_dim_out) == 0);
    //return result;
  }

public:
  isl_pw_aff *visit(const SCEV *Scev) {
    // In case the scev is a valid parameter, we do not further analyze this
    // expression, but create a new parameter in the isl_pw_aff. This allows us
    // to treat subexpressions that we cannot translate into an piecewise affine
    // expression, as constant parameters of the piecewise affine expression.
    if (isl_id *Id = S->getIdForParam(Scev)) {
      isl_space *Space = getInSpace();// isl_space_set_alloc(Ctx, 1, NbLoopSpaces);
      Space = isl_space_insert_dims(Space, isl_dim_param, 0, 1);
      Space = isl_space_set_dim_id(Space, isl_dim_param, 0, Id);

      isl_set *Domain = isl_set_universe(isl_space_copy(Space));
      isl_aff *Affine =
        isl_aff_zero_on_domain(isl_local_space_from_space(Space));
      Affine = isl_aff_add_coefficient_si(Affine, isl_dim_param, 0, 1);

      return isl_pw_aff_alloc(Domain, Affine);
    }

    return SCEVVisitor<Affinator, isl_pw_aff *>::visit(Scev);
  }

  Affinator(const ScopStmt *Stmt, isl_space *space)
    : Ctx(Stmt->getIslCtx()), NbLoopSpaces(Stmt->getNumIterators()),
    S(Stmt->getParent()) {
      if (space)
        this->space = isl_space_copy(space);
      else
        this->space = isl_space_set_alloc(Ctx, 0, NbLoopSpaces);
  }

  ~Affinator() {
    isl_space_free(space);
    space = nullptr;
  }

  __isl_give isl_pw_aff *visitConstant(const SCEVConstant *Constant) {
    ConstantInt *Value = Constant->getValue();
    isl_int v;
    isl_int_init(v);

    // LLVM does not define if an integer value is interpreted as a signed or
    // unsigned value. Hence, without further information, it is unknown how
    // this value needs to be converted to GMP. At the moment, we only support
    // signed operations. So we just interpret it as signed. Later, there are
    // two options:
    //
    // 1. We always interpret any value as signed and convert the values on
    //    demand.
    // 2. We pass down the signedness of the calculation and use it to interpret
    //    this constant correctly.
    MPZ_from_APInt(v, Value->getValue(), /* isSigned */ true);

    isl_space *Space = getInSpace();//isl_space_set_alloc(Ctx, 0, NbLoopSpaces);
    isl_local_space *ls = isl_local_space_from_space(isl_space_copy(Space));
    isl_aff *Affine = isl_aff_zero_on_domain(ls);
    isl_set *Domain = isl_set_universe(Space);

    Affine = isl_aff_add_constant(Affine, v);
    isl_int_clear(v);

    return isl_pw_aff_alloc(Domain, Affine);
  }

  __isl_give isl_pw_aff *visitTruncateExpr(const SCEVTruncateExpr *Expr) {
    llvm_unreachable("SCEVTruncateExpr not yet supported");
  }

  __isl_give isl_pw_aff *visitZeroExtendExpr(const SCEVZeroExtendExpr *Expr) {
    llvm_unreachable("SCEVZeroExtendExpr not yet supported");
  }

  __isl_give isl_pw_aff *visitSignExtendExpr(const SCEVSignExtendExpr *Expr) {
    // Assuming the value is signed, a sign extension is basically a noop.
    // TODO: Reconsider this as soon as we support unsigned values.
    return visit(Expr->getOperand());
  }

  __isl_give isl_pw_aff *visitAddExpr(const SCEVAddExpr *Expr) {
    isl_pw_aff *Sum = visit(Expr->getOperand(0));

    for (int i = 1, e = Expr->getNumOperands(); i < e; ++i) {
      isl_pw_aff *NextSummand = visit(Expr->getOperand(i));
      Sum = isl_pw_aff_add(Sum, NextSummand);
    }

    // TODO: Check for NSW and NUW.

    return Sum;
  }

  __isl_give isl_pw_aff *visitMulExpr(const SCEVMulExpr *Expr) {
    isl_pw_aff *Product = visit(Expr->getOperand(0));

    for (int i = 1, e = Expr->getNumOperands(); i < e; ++i) {
      isl_pw_aff *NextOperand = visit(Expr->getOperand(i));

      if (!isl_pw_aff_is_cst(Product) && !isl_pw_aff_is_cst(NextOperand)) {
        isl_pw_aff_free(Product);
        isl_pw_aff_free(NextOperand);
        return NULL;
      }

      Product = isl_pw_aff_mul(Product, NextOperand);
    }

    // TODO: Check for NSW and NUW.
    return Product;
  }

  __isl_give isl_pw_aff *visitUDivExpr(const SCEVUDivExpr *Expr) {
    llvm_unreachable("SCEVUDivExpr not yet supported");
  }

  int getLoopDepth(const Loop *L) {
    Loop *outerLoop =
      S->getRegion().outermostLoopInRegion(const_cast<Loop *>(L));
    assert(outerLoop && "Scop does not contain this loop");
    return L->getLoopDepth() - outerLoop->getLoopDepth();
  }

  __isl_give isl_pw_aff *visitAddRecExpr(const SCEVAddRecExpr *Expr) {
    assert(Expr->isAffine() && "Only affine AddRecurrences allowed");
    assert(S->getRegion().contains(Expr->getLoop()) &&
      "Scop does not contain the loop referenced in this AddRec");

    isl_pw_aff *Start = visit(Expr->getStart());
    isl_pw_aff *Step = visit(Expr->getOperand(1));
    isl_space *Space =  getInSpace(); //isl_space_set_alloc(Ctx, 0, NbLoopSpaces);
    isl_local_space *LocalSpace = isl_local_space_from_space(Space);

    int loopDimension = getLoopDepth(Expr->getLoop());

    isl_aff *LAff = isl_aff_set_coefficient_si(
      isl_aff_zero_on_domain(LocalSpace), isl_dim_in, loopDimension, 1);
    isl_pw_aff *LPwAff = isl_pw_aff_from_aff(LAff);

    // TODO: Do we need to check for NSW and NUW?
    return isl_pw_aff_add(Start, isl_pw_aff_mul(Step, LPwAff));
  }

  __isl_give isl_pw_aff *visitSMaxExpr(const SCEVSMaxExpr *Expr) {
    isl_pw_aff *Max = visit(Expr->getOperand(0));

    for (int i = 1, e = Expr->getNumOperands(); i < e; ++i) {
      isl_pw_aff *NextOperand = visit(Expr->getOperand(i));
      Max = isl_pw_aff_max(Max, NextOperand);
    }

    return Max;
  }

  __isl_give isl_pw_aff *visitUMaxExpr(const SCEVUMaxExpr *Expr) {
    llvm_unreachable("SCEVUMaxExpr not yet supported");
  }

  __isl_give isl_pw_aff *visitUnknown(const SCEVUnknown *Expr) {
    llvm_unreachable("Unknowns are always parameters");
  }
}; // class Affinator


isl::PwAff molly::convertScEvToAffine(ScopStmt *Stmt, const SCEV *se) {
  //auto dom = Stmt->getDomain();
  //auto iterDomain = getIterationDomain(Stmt);
  Scop *S = Stmt->getParent();
  const Region *Reg = &S->getRegion();

  S->addParams(getParamsInAffineExpr(Reg, se, *S->getSE()));

  Affinator Affinator(Stmt, Stmt->getDomainSpace());
  auto pwaff = Affinator.visit(se);
  return isl::PwAff::wrap(pwaff);
}
