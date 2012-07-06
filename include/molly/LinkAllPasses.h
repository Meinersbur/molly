#ifndef MOLLY_LINKALLPASSES_H
#define MOLLY_LINKALLPASSES_H


namespace molly {
	void initializeMollyPasses();
}

using namespace molly;


namespace {
  struct MollyForcePassLinking {
	MollyForcePassLinking() {
	  // We must reference the passes in such a way that compilers will not
	  // delete it all as dead code, even with whole program optimization,
	  // yet is effectively a NO-OP. As the compiler isn't smart enough
	  // to know that getenv() never returns -1, this will do the job.
	  if (std::getenv("bar") != (char*) -1)
		return;

	   //createAffSCEVItTesterPass();

	}
  } MollyForcePassLinking; // Force link by creating a global definition.
}



#endif /* MOLLY_LINKALLPASSES_H */