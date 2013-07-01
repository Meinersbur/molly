#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include "islpp_common.h"
#include "Obj.h" // baseclass of Id

// #include <isl/id.h>
struct isl_id;

namespace llvm {
  // #include <llvm/Support/raw_ostream.h>
  class raw_ostream;
} // namespace llvm

namespace isl {
  // #include "Ctx.h"
  class Ctx;
} // namespace isl


namespace isl {

  class Id : public Obj3<Id, isl_id> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() ;

  public:
    Id() { }
    static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    /* implicit */ Id(const ObjTy &that) : Obj3(that) { }
    /* implicit */ Id(ObjTy &&that) : Obj3(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

  public:
    StructTy *takeCopyOrNull() const;

    Ctx *getCtx() const;
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region Creational
  public:
    static Id create(Ctx *ctx, const char *name, void *user = nullptr) ;
#pragma endregion


#pragma region Properties
    const char *getName() const;
    void *getUser() const ;

    void setFreeUser_inline(void (*freefunc)(void *));
    Id setFreeUser(void (*freefunc)(void *));
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Id setFreeUser(void (*freefunc)(void *)) && ;
#endif
#pragma endregion
  }; // class Id


  static inline Id enwrap(__isl_take isl_id *id) { return Id::enwrap(id); }

  Id setFreeUser(const Id &id, void (*freefunc)(void *)) ;
  Id setFreeUser(Id &&id, void (*freefunc)(void *)) ;

  static inline bool operator==(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()==rhs.keepOrNull(); }
  static inline bool operator!=(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()!=rhs.keepOrNull(); }

  static inline void swap(isl::Id &lhs, isl::Id &rhs) { isl::Id::swap(lhs, lhs); }
} // namespace isl
#endif /* ISLPP_ID_H */
