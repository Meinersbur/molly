#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include <cassert>
#include <isl/id.h>
#include <islpp/Ctx.h>

struct isl_id;

namespace isl {
class Ctx;
}


namespace isl {

class Id {

  #pragma region Low-level functions
  private:
    isl_id *id;

  public: // Public because otherwise we had to add a lot of friends
    isl_id *take() { assert(id); isl_id *result = id; id = nullptr; return result; }
    isl_id *takeCopy() const;
    isl_id *keep() const { return id; }
  protected:
    void give(isl_id *id);

    explicit Id(isl_id *id) : id(id) { }
  public:
    static Id wrap(isl_id *id) { return Id(id); }
#pragma endregion

public:
  Id(Ctx &ctx, const char *name, void *user) {
    assert(name);
    assert(user);
    this->id = isl_id_alloc(ctx.keep(), name, user);
  }



  ~Id() {
    isl_id_free(id);
  }
}; // class Id
} // namespace isl
#endif /* ISLPP_ID_H */