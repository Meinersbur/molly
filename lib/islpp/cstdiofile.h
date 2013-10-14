#ifndef MOLLY_CSTDIOFILE
#define MOLLY_CSTDIOFILE

#include <llvm/Support/Compiler.h>
#include <string>

namespace llvm {
  class raw_ostream;
} // namepspace llvm


#if defined(__MINGW32__)
// Neither open_memstream nor fopen_s (with "TD" options) available
#define CSTDIOFILE_TMPFILE
#elif defined(_WIN32)
// open_memstream not available, use temporary file that is not written to disk instead
#define CSTDIOFILE_FOPEN_S 1
#else
#define CSTDIOFILE_OPEN_MEMSTREAM 1
#endif

namespace molly {

  //TODO: Rename to something that has "Temp" in it
  class CstdioFile { 
    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, CstdioFile &);

  private:
    FILE *fd;
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
    char *buf;
    size_t writtensize;
#endif

#ifdef CSTDIOFILE_FOPEN
    char *tmpfilename;
#endif

    CstdioFile(CstdioFile &&that) {
      this->fd = that.fd;
      that.fd = NULL;

#ifdef CSTDIOFILE_OPEN_MEMSTREAM
     this->buf = that.buf;
     that.buf = nullptr;
     this->writtensize = that.writtensize;
#endif
    }
    const CstdioFile &operator=(const CstdioFile &) LLVM_DELETED_FUNCTION;
    CstdioFile(const CstdioFile &) LLVM_DELETED_FUNCTION;

  public:
    CstdioFile();

    ~CstdioFile() {
      close();

#ifdef CSTDIOFILE_OPEN_MEMSTREAM
      free(buf); buf = nullptr;
#endif
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
