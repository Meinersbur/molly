#ifndef MOLLY_CSTDIOFILE
#define MOLLY_CSTDIOFILE

#include <llvm/Support/Compiler.h>
#include <string>

namespace llvm {
  class raw_ostream;
} // namespace llvm


#if defined(__MINGW32__)
// Neither open_memstream nor fopen_s are available
//#define CSTDIOFILE_HAS_TD 1 // MinGW runtime is msvcrt-based, therefore supports the TD option
//#define CSTDIOFILE_TMPFILE 1
//#define CSTDIOFILE_FOPEN 1
//#define CSTDIOFILE_FOPEN_S 1
//#define CSTDIOFILE_OPEN_MEMSTREAM 1
#define CSTDIOFILE_LLVM_CREATETEMPORARYFILE 1
#elif defined(_MSC_VER)
// open_memstream not available, use temporary file that is not written to disk instead
#define CSTDIOFILE_HAS_TD 1
#define CSTDIOFILE_FOPEN_S 1
//#define CSTDIOFILE_LLVM_CREATETEMPORARYFILE 1
#else
// POSIX, best solution; pure in-memory without syscalls
#define CSTDIOFILE_OPEN_MEMSTREAM 1
#endif


#ifdef CSTDIOFILE_FOPEN
#include <cstdio> // L_tmpnam
#endif /* CSTDIOFILE_FOPEN */

#ifdef CSTDIOFILE_LLVM_CREATETEMPORARYFILE
#include <llvm/ADT/SmallString.h>
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

#ifdef CSTDIOFILE_FOPEN_S
    char *tmpfilename;
#endif

#ifdef CSTDIOFILE_FOPEN
    char tmpfilename[L_tmpnam];
#endif

#ifdef CSTDIOFILE_LLVM_CREATETEMPORARYFILE
    llvm::SmallString<128> tmppath;
#endif

    CstdioFile(CstdioFile &&that) {
      this->fd = that.fd;
      that.fd = NULL;

#ifdef CSTDIOFILE_OPEN_MEMSTREAM
     this->buf = that.buf;
     that.buf = nullptr;
     this->writtensize = that.writtensize;
#endif

#ifdef CSTDIOFILE_FOPEN_S
     this->tmpfilename = that.tmpfilename;
#endif

#ifdef CSTDIOFILE_FOPEN
     memcpy(&this->tmpfilename, that.tmpfilename, sizeof(tmpfilename));
#endif

#ifdef CSTDIOFILE_LLVM_CREATETEMPORARYFILE
    this-> tmppath = std::move(that.tmppath);
    that.tmppath.clear();
#endif
    }
    const CstdioFile &operator=(const CstdioFile &) LLVM_DELETED_FUNCTION;
    CstdioFile(const CstdioFile &) LLVM_DELETED_FUNCTION;

  public:
    CstdioFile();
    ~CstdioFile();

    static CstdioFile create() {
      return CstdioFile();
    }


    FILE *getFileDescriptor() {
      return fd;
    }

    void close();

    std::string readAsStringAndClose(); // Deprecated; use operator<<
  }; // class TmpFile

  llvm::raw_ostream &operator<<(llvm::raw_ostream &, CstdioFile &);
} // namespace molly
#endif /* MOLLY_CSTDIOFILE */
