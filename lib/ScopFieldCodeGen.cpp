#include "ScopFieldCodeGen.h"

#include <polly/ScopPass.h>

using namespace molly;
using namespace llvm;
using namespace polly;


namespace molly {
 class ScopFieldCodeGen final : public polly::ScopPass {

  public:
    static char ID;
    ScopFieldCodeGen() : ScopPass(ID) {
    }

    virtual const char *getPassName() const {
      return "ScopFieldCodeGen";
    }

    virtual bool runOnScop(Scop &S) {
      return false;
    }
  }; // class FieldCodeGen
} // namespace molly



  char ScopFieldCodeGen::ID = 0;
static RegisterPass<ScopFieldCodeGen> FieldScopCodeGenRegistration("molly-scopfieldcodegen", "Molly - Modify SCoPs", false, false);

  polly::ScopPass *molly::createScopFieldCodeGenPass() {
    return new ScopFieldCodeGen();
  }
