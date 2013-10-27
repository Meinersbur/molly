#ifndef MOLLY_REGISTER_PASSES_H
#define MOLLY_REGISTER_PASSES_H
namespace llvm {
  class PassManagerBase;
}

namespace molly {
  void forceLinkPassRegistration();
}

namespace {
  struct MollyStaticInitializer {

    MollyStaticInitializer() {
      molly::forceLinkPassRegistration();
    }

  } MollyStaticInitializer; // MollyStaticInitializer
} // namespace

#endif /* MOLLY_REGISTER_PASSES_H */
