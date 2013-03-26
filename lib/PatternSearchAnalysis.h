#ifndef MOLLY_PATTERNSEAHRCH_H
#define MOLLY_PATTERNSEAHRCH_H

#include <llvm/Pass.h> // FunctionPass (base of PatternSearchAnalysis)
#include <vector> 

namespace polly {
  class ScopInfo;
  class Scop;
} // namespace polly


namespace molly {
  class Pattern {
  }; // class Pattern

  class StencilPattern : public Pattern {
  }; // class StencilPattern

  class ReductionPattern : public Pattern {
  }; // class ReductionPattern

  class MasterPattern: public Pattern {
  }; // class MasterPattern


  class PatternSearchAnalysis : public llvm::FunctionPass {
  private:

  public:
    static char ID;
    PatternSearchAnalysis() : FunctionPass(ID) {
    }

    std::vector<Pattern*> Patterns;

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual bool runOnFunction(llvm::Function &);

  private:
    void runOnScop(polly::Scop*);
  }; // class PatternSearchAnalysis
} // namespace molly

#endif /* MOLLY_PATTERNSEAHRCH_H */
