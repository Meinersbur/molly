#ifndef MOLLY_MOLLYSCOPSTMTPROCESSOR_H
#define MOLLY_MOLLYSCOPSTMTPROCESSOR_H

#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"


namespace molly {

  class MollyScopStmtProcessor {
  public:
    virtual void applyWhere() = 0;

  public:
    static MollyScopStmtProcessor *create(MollyPassManager *pm, polly::ScopStmt *scop);
  }; // class MollyScopStmtProcessor

} // namespace molly
#endif /* MOLLY_MOLLYSCOPSTMTPROCESSOR_H */
