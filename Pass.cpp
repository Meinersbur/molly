#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraphSCCPass.h"

#include <gmp.h>
#include <isl/ctx.h>
#include <stdarg.h>

#define CLOOG_INT_GMP
#include <cloog/cloog.h>


// Required by ISL
extern "C"
int snprintf(char *str,size_t size,const char *fmt,...) {
   int ret;
   va_list ap;
   
   va_start(ap,fmt); 
   ret = vsnprintf(str,size,fmt,ap);
   // Whatever happen in vsnprintf, what i'll do is just to null terminate it 
   // and everything else s mynickmynick's problem
   str[size-1] = '\0';       
	va_end(ap);    
   return ret;    
}


using namespace llvm;

namespace llvm {
	
struct MollyFunc : public FunctionPass {
	static char ID;
		MollyFunc() : FunctionPass(ID) {}

		virtual ~MollyFunc(){
			errs() << "Destruct! ";
		}

	virtual bool runOnFunction(Function &F) {
	  errs() << "Hello: ";
	  errs().write_escaped(F.getName()) << "\n";
	  return false;
	}
};  // end of struct Hello


class P: public CallGraphSCCPass {
	virtual bool doInitialization(CallGraph &CG) {
		return true;
	}
};

void initializeMollyFuncPass(PassRegistry &reg);

}  // end of anonymous namespace

char MollyFunc::ID = 0;



//INITIALIZE_PASS(MollyFunc, "molly",  "Molly Pass 2", false, false);

//,<MollyFunc> X("molly", "Molly Pass",
//							 false /* Only looks at CFG */,
//							 false /* Analysis Pass */);
			   
extern "C" {
int teststatic() {
	// GMP
	mpz_t a;
	mpz_init(a);

	// ISL
	auto ctx = isl_ctx_alloc(); 

	// CLOOG
	auto c = new_clast_name("Name");

	return 0;
}
}

static int x = teststatic();

