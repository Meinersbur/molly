#include "cstdiofile.h"

#include <clang/Driver/Compilation.h>
#include <clang/Driver/Action.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/DriverDiagnostic.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/ToolChain.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Option/ArgList.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Twine.h>
#include <vector>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <string>
//#include <stdio.h>

#ifdef CSTDIOFILE_LLVM_CREATETEMPORARYFILE
//#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#endif


#ifdef CSTDIOFILE_HAS_TD
#define CSTDIOFILE_FOPEN_TD "TD"
#else
#define CSTDIOFILE_FOPEN_TD
#endif

using namespace molly;
using namespace std;

CstdioFile::CstdioFile() {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  // Unix version
  buf = nullptr;
  writtensize = 0;
  fd = open_memstream(&buf, &writtensize);
  if (!fd) {
    llvm::errs() << "ERROR open_memstream: writtensize=" << writtensize << " &buf=" << &buf << " errno=" << errno << "\n";
    llvm::errs().flush();
  }
  assert(fd);
#endif /* CSTDIOFILE_OPEN_MEMSTREAM */

#ifdef CSTDIOFILE_FOPEN_S
  // _tempnam, fopen_s only available in MSVCRT
  this->tmpfilename = _tempnam(NULL, "cstdiofile");
  assert(tmpfilename);
  auto retval = fopen_s(&fd, tmpfilename, "w+b" CSTDIOFILE_FOPEN_TD); // T=Specifies a file as temporary. If possible, it is not flushed to disk; D=The file is deleted when the last file pointer is closed.
  assert(retval == NULL);
  //_get_errno( &err );
  //printf( _strerror(NULL) );
#ifndef CSTDIOFILE_HAS_TD
  free(tmpfilename);
  tmpfilename = nullptr;
#endif
#endif /* CSTDIOFILE_FOPEN_S */

#ifdef CSTDIOFILE_FOPEN
  errno=0;
  auto fname = tmpnam(this->tmpfilename);
  assert(fname);
  fd = fopen(fname, "w+" CSTDIOFILE_FOPEN_TD);
  if (!fd) {
    llvm::errs() << "ERROR fopen: fname=" << fname << " fd=" << fd << " errno=" << errno << "\n";
    llvm::errs().flush();
  }
  assert(fd);
#endif /* CSTDIOFILE_FOPEN */

#ifdef CSTDIOFILE_TMPFILE
  // Deleted on fclose
  errno=0;
  fd = tmpfile();
  if (!fd) {
    llvm::errs() << "ERROR tmpfile: fd=" << fd << " errno=" << errno << "\n";
    llvm::errs().flush();
  }
  assert(fd);
#endif /* CSTDIOFILE_TMPFILE */

#ifdef CSTDIOFILE_LLVM_CREATETEMPORARYFILE
  // LLVM per-platform implementation
  int fdno=-1;
  llvm::sys::fs::createTemporaryFile("cstdiofile", "", fdno, this->tmppath);
  assert(fdno!=-1);
  fd = fdopen(fdno, "w+b" CSTDIOFILE_FOPEN_TD);
  assert(fd);
#endif /* CSTDIOFILE_LLVM_CREATETEMPORARYFILE */
}


CstdioFile::~CstdioFile() {
  close();

#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  free(buf); 
  buf = nullptr;
#endif /* CSTDIOFILE_OPEN_MEMSTREAM */
}


void CstdioFile::close() {
  if (!fd)
    return;

  fclose(fd);
  fd = nullptr;

#if defined(CSTDIOFILE_FOPEN_S) && !defined(CSTDIOFILE_HAS_TD)
  unlink(tmpfilename);
  free(tmpfilename);
  tmpfilename = nullptr;
#endif

#if defined(CSTDIOFILE_FOPEN) && !defined(CSTDIOFILE_HAS_TD)
  unlink(&tmpfilename);
  tmpfilename[0] = '\0';
#endif

#if defined(CSTDIOFILE_LLVM_CREATETEMPORARYFILE) && !defined(CSTDIOFILE_HAS_TD)
  llvm::sys::fs::remove(llvm::Twine(this->tmppath), false);
  this->tmppath.clear();
#endif
}


std::string CstdioFile::readAsStringAndClose() {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  close();
  return std::string(buf, writtensize);
#else /* CSTDIOFILE_OPEN_MEMSTREAM */
  // Determine file size
  fseek(fd, 0, SEEK_END);
  size_t size = ftell(fd);

  rewind(fd);
  std::vector<char> bytes(size); // Temporary buffer
  fread(&bytes[0], sizeof(char), size, fd);
  close();
  return std::string(&bytes[0], size);
#endif /* CSTDIOFILE_OPEN_MEMSTREAM */
}


llvm::raw_ostream &molly::operator<<(llvm::raw_ostream &OS, CstdioFile &tmp) {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  OS.write(tmp.buf, tmp.writtensize);
#else /* CSTDIOFILE_OPEN_MEMSTREAM */
  fpos_t savepos;
  auto retval = fgetpos(tmp.fd, &savepos);
  if (retval) {
    llvm::errs() << "ERROR fgetpos: retval=" << retval << " tmp.td=" << tmp.fd << " errno=" << errno << "\n";
    llvm::errs().flush();
  }
  assert(retval == 0);
  rewind(tmp.fd);

  while (!feof(tmp.fd)) {
    char buf[1024];
    auto readSize = fread(&buf[0], sizeof(buf[0]), sizeof(buf) / sizeof(buf[0]), tmp.fd);
    if (ferror(tmp.fd)) {
      llvm::errs() << "Error reading temporary file";
    }

    OS.write(&buf[0], readSize);
  }

  fsetpos(tmp.fd, &savepos);
#endif /* CSTDIOFILE_OPEN_MEMSTREAM */

  return OS;
}
