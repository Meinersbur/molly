#include "AffineMapping.h"

#include <llvm/ADT/ArrayRef.h>
#include "IslExprBuilder.h"
#include <llvm/IR/Module.h>
#include "islpp/Set.h"
#include "islpp/PwAff.h"

using namespace llvm;
using namespace molly;

#if 0
  llvm::Function *AffineMapping::getMappingFunc(llvm::Module *module) {
    auto &llvmContext = module->getContext();
    auto nDims = mapping.getInDimCount();
    auto intTy = Type::getInt32Ty(llvmContext);

    SmallVector<Type *,4> tys(nDims, intTy);
    auto funcTy = FunctionType::get(intTy, tys , false);
    auto func = Function::Create(funcTy, GlobalValue::PrivateLinkage, "affine_mapping", module);

    
       return func;
  }
#endif


  isl::Set AffineMapping:: getIndexset() const {
    return mapping.getDomain();
  }


 llvm::Value *AffineMapping::codegen(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value*> coords) {
   // FIXME: coords bot used
   // TODO: This is piecewise, can make SCopStmts instead
  return buildIslAff(builder, mapping, params);
 }
