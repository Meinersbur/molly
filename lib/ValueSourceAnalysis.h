#ifndef MOLLY_VALUESOURCEANALYSIS_H
#define MOLLY_VALUESOURCEANALYSIS_H

//#include "FieldAccess.h"
#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>



namespace llvm {
  class Value;
  class Instruction;
  class User;
} // namespace llvm

namespace molly {
} // namespace molly


namespace molly {

class ValueSourceAnalysis : public llvm::FunctionPass {
public:
  static char ID;
  ValueSourceAnalysis() : FunctionPass(ID) {
   }

  virtual bool runOnFunction(llvm::Function &F) { return false; }


  void findSources(llvm::Instruction/*llvm::User*/ *value, llvm::SmallVectorImpl<llvm::Value*> &sources);
  void findResults(llvm::Value *value, llvm::SmallVectorImpl<llvm::User*> &results);

  
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

private:


}; // class ValueSourceAnalysis
} // namespace molly
#endif /* MOLLY_VALUESOURCEANALYSIS_H */