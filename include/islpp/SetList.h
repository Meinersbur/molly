#ifndef ISLPP_SETLIST_H
#define ISLPP_SETLIST_H

#include "List.h"

struct isl_set_list;

namespace isl {
  class Set;
} // namespace isl

namespace isl {

  template<>
  class List<Set> final {
#pragma region Low-level
  private:
    isl_set_list *list;

  public:
    isl_set_list *keep() const { return list; }

#pragma endregion

  }; // class List<Set>
} // namespace isl
#endif /* ISLPP_SETLIST_H */
