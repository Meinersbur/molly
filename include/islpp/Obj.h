#ifndef ISLPP_OBJ_H
#define ISLPP_OBJ_H

namespace isl {
  class Ctx;
} // namespace isl

namespace isl {
  class Obj {
  public:
    virtual Ctx *getCtx() const = 0;
  }; // class Obj
} // namepspace isl
#endif /* ISLPP_OBJ_H */
