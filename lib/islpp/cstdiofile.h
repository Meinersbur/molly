#ifndef MOLLY_CSTDIOFILE
#define MOLLY_CSTDIOFILE

#include <llvm/Support/Compiler.h>
#include <string>

namespace llvm {
  class raw_ostream;
} // namepspace llvm


namespace molly {

  class CstdioFile {
    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, CstdioFile &);

  private:
    FILE *fd;

    CstdioFile(CstdioFile &&that) {
    if (this==&that)
      return;
    this->fd = that.fd;
    that.fd = NULL;
    }
    const CstdioFile &operator=(const CstdioFile &) LLVM_DELETED_FUNCTION;
    CstdioFile(const CstdioFile &) LLVM_DELETED_FUNCTION;

  public:
    CstdioFile();

    ~CstdioFile() {
      close();
    }

    static CstdioFile create() {
      return CstdioFile();
    }


    FILE *getFileDescriptor() {
      return fd;
    }

    void close() ;

    std::string readAsStringAndClose(); // Deprecated; use operator<<
     

  }; // class TmpFile

  llvm::raw_ostream &operator<<(llvm::raw_ostream &, CstdioFile &);
} // namespace molly
#endif /* MOLLY_CSTDIOFILE */
