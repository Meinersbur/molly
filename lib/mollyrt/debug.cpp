#define __MOLLYRT
#include "molly.h"

using namespace molly;

#ifndef NTRACE
int _debugindention;


DebugFunctionScope::DebugFunctionScope(const char *funcname, const char *file, int line) : funcname(funcname) {
  auto myrank = __molly_cluster_mympirank();
  if (PRINTRANK >= 0 && myrank != PRINTRANK)
    return;
  std::cerr << myrank << ")";
  for (int i = _debugindention; i > 0; i-=1) {
    //fprintf(stderr,"  ");
    std::cerr << "  ";
  }
  //fprintf(stderr,"ENTER %s (%s:%d)\n", funcname, extractFilename(file), line);
  std::cerr << "ENTER " << funcname << " (" << extractFilename(file) << ":" << line << ")" << std::endl;
  _debugindention += 1;
}
 

DebugFunctionScope::~DebugFunctionScope() {
  auto myrank = __molly_cluster_mympirank();
  if (PRINTRANK >= 0 && myrank != PRINTRANK)
    return;
  _debugindention -= 1;
  std::cerr << myrank << ")";
  for (int i = _debugindention; i > 0; i-=1) {
    //fprintf(stderr,"  ");
    std::cerr << "  ";
  }
  //fprintf(stderr,"EXIT\n");
  std::cerr << "EXIT  " << funcname << std::endl;
}
#endif
