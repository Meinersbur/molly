#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include "islpp_common.h"
#include <cassert>
#include <string>
#include <isl/ctx.h>

struct isl_id;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
} // namespace isl


namespace isl {
  class Id final {
#pragma region Low-level functions
  private:
    isl_id *id;

  public: // Public because otherwise we had to add a lot of friends
    isl_id *takeOrNull() { isl_id *result = id; id = nullptr; return result; }
    isl_id *take() { assert(id); isl_id *result = id; id = nullptr; return result; }
    isl_id *takeCopyOrNull() const;
    isl_id *takeCopy() const { assert(id); return takeCopyOrNull(); }
    isl_id *keepOrNull() const { return id; }
    isl_id *keep() const { assert(id); return id; }
  protected:
    void giveOrNull(isl_id *id = NULL);
    void give(isl_id *id) { assert(id); giveOrNull(id); }

  public:
    static Id wrap(isl_id *id) { Id result; result.giveOrNull(id); return result; }
#pragma endregion

  public:
    ~Id() ;

    Id() : id(NULL) {}
    Id(Id &&that) : id(that.takeOrNull()) { }
    Id(const Id &that) : id(that.takeCopyOrNull()) { }

    const Id &operator=(Id &&that) { giveOrNull(that.takeOrNull()); return *this; }
    const Id &operator=(const Id &that) { give(that.takeCopyOrNull()); return *this; }

    //explicit operator bool() const { return id!=NULL; }
    bool isNull() const { return id==NULL; }
    bool isValid() const { return id!=NULL; }

  public:
#pragma region Creational
    static Id create(Ctx *ctx, const char *name, void *user = NULL) ;
    static Id createAndFreeUser(Ctx *ctx, const char *name, void *user);

  private:
    template<typename T>
    static void deleteUser(void *user) {
      delete static_cast<T*>(user);
    }
  public:
    template<typename T>
    static Id createAndDeleteUser(Ctx *ctx, const char *name, T *user) {
      auto result = create(ctx, name, user);
      //isl_id *result = isl_id_alloc(ctx->keep(), name, user);
      result = isl_id_set_free_user(result->keep(), &deleteUser<T>);
      return result;
    }

    Id copy() const { return Id::wrap(takeCopy()); }
#pragma endregion

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
#pragma endregion

    Ctx *getCtx() const;
    void *getUser() const;
    const char *getName() const;

    void setFreeUser(void (*freefunc)(void *));
  }; // class Id

  static inline Id enwrap(__isl_take isl_id *id) { return Id::wrap(id); }
   
  static inline bool operator==(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()==rhs.keepOrNull(); }
  static inline bool operator!=(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()!=rhs.keepOrNull(); }

} // namespace isl
#endif /* ISLPP_ID_H */
