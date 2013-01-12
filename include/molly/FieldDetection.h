

#ifndef MOLLY_FIELD_DETECTION_H
#define MOLLY_FIELD_DETECTION_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
//#include "llvm\Analysis\Loo

using namespace llvm;

namespace molly {

	class FieldDetectionAnalysis : public ModulePass {
	public:
		static char ID;
		FieldDetectionAnalysis() : ModulePass(ID) {
		}

		//virtual Pass *createPrinterPass(raw_ostream &O,
		//                        const std::string &Banner) {
		//}

		virtual bool runOnModule(Module &M) {
			return false;
		}

	private:
		Pass *createFieldDetectionAnalysisPass();

	};

}

namespace llvm {
	class PassRegistry;
	void initializeScopDetectionPass(llvm::PassRegistry&);
}


#endif /* MOLLY_FIELD_DETECTION_H */
