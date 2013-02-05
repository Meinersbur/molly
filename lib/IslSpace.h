#ifndef MOLLY_ISLSPACE_H
#define MOLLY_ISLSPACE_H 1

#include <llvm/Support/Compiler.h>

struct isl_space;



namespace molly {
  

  class IslSpace {
    IslSpace(const IslSpace &) LLVM_DELETED_FUNCTION;
    const IslSpace &operator=(const IslSpace &) LLVM_DELETED_FUNCTION;

    friend class IslCtx;

  private:
    isl_space *space;

    IslSpace(isl_space *space);

  protected:
    static IslSpace wrap(isl_space *space);

  public:
    ~IslSpace();

  };

}

#endif /* MOLLY_ISLSPACE_H */
