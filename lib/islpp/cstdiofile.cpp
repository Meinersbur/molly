#include "cstdiofile.h"

#include <llvm/Support/raw_ostream.h>
#include <vector>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fstream>

using namespace molly;




CstdioFile::CstdioFile() { 
//#ifdef WIN32
 
#ifdef WIN32
  //TODO: createTemporaryFileOnDisk
   auto tmpfilename = _tempnam( NULL, "tmp" );
#else
 auto tmpfilename = tmpnam("tmp");
#endif
  assert(tmpfilename);
  #ifdef WIN32
  auto retval = fopen_s(&fd, tmpfilename, "w+bTD");  
   assert(retval == NULL);
#else
   fd = fopen(tmpfilename, "w+");//TODO: Use open_memstream() on unices
#endif
 
  //_get_errno( &err );
  //printf( _strerror(NULL) );
  free(tmpfilename);
//#else
  //fd = tmpfile();
  //open_memstream();
//#endif
}


void CstdioFile::close() {
      if (fd)
        fclose(fd);
      fd = NULL;
}


std::string CstdioFile::readAsStringAndClose() {
  // Determine file size
  fseek(fd, 0, SEEK_END);
  size_t size = ftell(fd);

  rewind(fd);
  std::vector<char> bytes(size);
  fread(&bytes[0], sizeof(char), size, fd);
  close();
  return std::string(&bytes[0], size);
}


llvm::raw_ostream &molly::operator<<(llvm::raw_ostream &OS, CstdioFile &tmp) {
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
}
