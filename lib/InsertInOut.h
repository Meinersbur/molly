#ifndef MOLLY_INSERTINOUT_H
#define MOLLY_INSERTINOUT_H

namespace polly {
  class ScopPass;
} // namespace polly


namespace molly {
  extern char &InsertInOutPassID;
  polly::ScopPass *creatInsertInOutPass(); 
} // namespace molly

#endif /* MOLLY_INSERTINOUT_H */
