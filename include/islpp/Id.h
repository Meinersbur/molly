#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include <cassert>
#include <string>

struct isl_id;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
} // namespace isl


namespace isl {
  class Id {
#pragma region Low-level functions
  private:
    isl_id *id;

  public: // Public because otherwise we had to add a lot of friends
    isl_id *take() { assert(id); isl_id *result = id; id = nullptr; return result; }
    isl_id *takeCopy() const;
    isl_id *keep() const { assert(id); return id; }
  protected:
    void give(isl_id *id);

  public:
    static Id wrap(isl_id *id) { Id result; result.give(id); return result; }
#pragma endregion

    ~Id() ;

    Id() : id(NULL) {}
    Id(Id &&that) : id(that.take()) { }
    Id(const Id &that) : id(that.takeCopy()) { }

    const Id &operator=(Id &&that) { give(that.take()); return *this; }
    const Id &operator=(const Id &that) { give(that.takeCopy()); return *this; }

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
    Id createAndDeleteUser(Ctx *ctx, const char *name, T *user) {
      isl_id *result = isl_id_alloc(ctx->keep(), name, user);
      result = isl_id_set_free_user(result, &deleteUser<T>);
      return Id::wrap(result);
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

  bool operator==(const Id &lhs, const Id &rhs) { return lhs.keep()==rhs.keep(); }
  bool operator!=(const Id &lhs, const Id &rhs) { return lhs.keep()!=rhs.keep(); }

} // namespace isl
#endif /* ISLPP_ID_H */