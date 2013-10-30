#define __MOLLYRT
#include "molly.h"

using namespace molly;

int _debugindention;

DebugFunctionScope::DebugFunctionScope(const char *funcname, const char *file, int line) : funcname(funcname) {
  //if (!__molly_isMaster())
  //  return;
  std::cerr << __molly_cluster_mympirank() << ")";
  for (int i = _debugindention; i > 0; i-=1) {
    //fprintf(stderr,"  ");
    std::cerr << ' ' << ' ';
  }
  //fprintf(stderr,"ENTER %s (%s:%d)\n", funcname, extractFilename(file), line);
  std::cerr << "ENTER " << funcname << " (" << extractFilename(file) << ":" << line << ")" << std::endl;
  _debugindention += 1;
}


DebugFunctionScope::~DebugFunctionScope() {
  //if (!__molly_isMaster())
  //  return;
  _debugindention -= 1;
  std::cerr << __molly_cluster_mympirank() << ")";
  for (int i = _debugindention; i > 0; i-=1) {
    //fprintf(stderr,"  ");
    std::cerr << ' ' << ' ';
  }
  //fprintf(stderr,"EXIT\n");
  std::cerr << "EXIT  " << funcname << std::endl;
}
