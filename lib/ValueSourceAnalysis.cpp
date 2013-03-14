#include "ValueSourceAnalysis.h"

#include <llvm/IR/Instruction.h>
#include <llvm/IR/User.h>
#include <llvm/InstVisitor.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>

using namespace llvm;
using namespace molly;
using namespace std;





class ValueSourceVisitor : public InstVisitor<ValueSourceVisitor> {
  SmallVectorImpl<llvm::Value*> &sources;
  SmallVectorImpl<User*> &worklist;
  DenseSet <Value*> &donelist;

public:
  ValueSourceVisitor(SmallVectorImpl<User*> &worklist, DenseSet<Value*> &donelist, SmallVectorImpl<llvm::Value*> &sources) : worklist(worklist), donelist(donelist), sources(sources) {
  }

  void visitLoad(LoadInst &I) {
    sources.push_back(&I);
  }

  void visitInstruction(Instruction &I) {
    // Other instruction; follow
    worklist.push_back(&I);
  }

}; // class ValueSourceVisitor




void ValueSourceAnalysis::findSources(llvm::Instruction *value, llvm::SmallVectorImpl<llvm::Value*> &sources) {
  //TODO: We may cache results for later use
  SmallVector<User*, 8> worklist;
  DenseSet<Value*> donelist;
  ValueSourceVisitor visitor(worklist, donelist, sources);

  auto bb = value->getParent();

  worklist.push_back(value);
  while (true) {
    auto user = worklist.pop_back_val();
    for (auto it = user->op_begin(), end = user->op_end(); it!=end; ++it) {
      auto *use = it;
      assert(user == use->getUser());
      auto val = use->get();

      if (donelist.count(val))
        continue;
      donelist.insert(val);

      if (auto instr = dyn_cast<Instruction>(val)) {
        if (instr->getParent() != bb)
          continue; // Support only one BB at the moment!

        if (!donelist.count(instr)) {
          visitor.visit(instr);
        }
      } else if (auto valAsUser = dyn_cast<User>(val)) {
        worklist.push_back(valAsUser);
      } else {
        // Simple value
        sources.push_back(val);
      }
    }
  }
}


void ValueSourceAnalysis::findResults(llvm::Value *value, llvm::SmallVectorImpl<llvm::User*> &results) {
}



char ValueSourceAnalysis::ID = 0;
static RegisterPass<ValueSourceAnalysis> ValueSourceAnalysisRegistration("molly-valuesource", "Molly - Value source analysis", false, true);
