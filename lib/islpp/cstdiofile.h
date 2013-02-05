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

    CstdioFile(const CstdioFile &) LLVM_DELETED_FUNCTION;
    const CstdioFile &operator=(const CstdioFile &) LLVM_DELETED_FUNCTION;


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

    void close() {
      if (fd)
        fclose(fd);
      fd = NULL;
    }

    std::string readAsStringAndClose(); // Deprecated; use operator<<
     

  }; // class TmpFile
} // namespace molly
#endif /* MOLLY_CSTDIOFILE */
