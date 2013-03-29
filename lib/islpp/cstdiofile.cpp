#include "cstdiofile.h"

#include <llvm/Support/raw_ostream.h>
#include <vector>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <stdio.h>

using namespace molly;
using namespace std;

CstdioFile::CstdioFile() {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  // Unix version
  fd = open_memstream(&buf, &writtensize);
#endif

#ifdef CSTDIOFILE_FOPEN_S
  // _tempnam, fopen_s only available in MSVCRT
  auto tmpfilename = _tempnam( NULL, "tmp" );
  assert(tmpfilename);
  auto retval = fopen_s(&fd, tmpfilename, "w+bTD"); // T=Specifies a file as temporary. If possible, it is not flushed to disk; D=Specifies a file as temporary. It is deleted when the last file pointer is closed.
  assert(retval == NULL);
  //_get_errno( &err );
  //printf( _strerror(NULL) );
  free(tmpfilename);
#endif

#ifdef CSTDIOFILE_FOPEN
  tmpfilename = tmpnam("tmp");
  assert(tmpfilename);
  fd = fopen(tmpfilename, "w+");
#endif

#ifdef CSTDIOFILE_TMPFILE
  // Deleted on fclose
  fd = tmpfile();
#endif

#ifdef CSTDIOFILE_CREATETEMPORARYFILEONDISK
  // LLVM platform-dependent implementation
  llvm::Path::createTemporaryFileOnDisk
#endif
}


void CstdioFile::close() {
  if (fd)
    fclose(fd);
  fd = NULL;

#ifdef CSTDIOFILE_FOPEN
  unlink(tmpfilename);
  free(tmpfilename);
  tmpfilename = NULL;
#endif
}


std::string CstdioFile::readAsStringAndClose() {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  return std::string(buf, writtensize);
#else
  // Determine file size
  fseek(fd, 0, SEEK_END);
  size_t size = ftell(fd);

  rewind(fd);
  std::vector<char> bytes(size); // Temporary buffer
  fread(&bytes[0], sizeof(char), size, fd);
  close();
  return std::string(&bytes[0], size);
#endif
}


llvm::raw_ostream &molly::operator<<(llvm::raw_ostream &OS, CstdioFile &tmp) {
#ifdef CSTDIOFILE_OPEN_MEMSTREAM
  OS.write(buf, writtensize);
#else
  fpos_t savepos;
  auto retval = fgetpos(tmp.fd, &savepos);
  assert(retval==0);
  rewind(tmp.fd);

  while (!feof(tmp.fd)) {
    char buf[1024];
    auto readSize = fread(&buf[0], sizeof(buf[0]), sizeof(buf)/sizeof(buf[0]), tmp.fd);
    if ( ferror(tmp.fd) ) {
      llvm::errs() << "Error reading temporary file";
    }

    OS.write(&buf[0], readSize);
  }

  fsetpos(tmp.fd, &savepos);
  return OS;
#endif
}
