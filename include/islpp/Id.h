#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include "islpp_common.h"
#include "Obj.h" // baseclass of Id

// #include <isl/id.h>
struct isl_id;

namespace llvm {
  // #include <llvm/Support/raw_ostream.h>
  class raw_ostream;
  template<typename> struct DenseMapInfo;
} // namespace llvm

namespace isl {
  // #include "Ctx.h"
  class Ctx;
  class Id;
} // namespace isl

namespace llvm {
  template<> class DenseMapInfo<isl::Id>;
} // namespace llvm


namespace isl {

  class Id : public Obj3<Id, isl_id> {
    friend struct llvm::DenseMapInfo<isl::Id>;

    //private:
    //  Id(isl_id *obj, std::string &&printed) : Obj3(obj, std::move(printed)) {}

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


namespace llvm {
  // So you can put an isl::Id into a DenseMap



  template<>
  struct DenseMapInfo<isl::Id> {
  private:
      class KeyInitializer {
  private:
       isl::Ctx *ctx;

  public:
    KeyInitializer();
    ~KeyInitializer();

     isl::Id empty;
     isl::Id tombstone;
  }; // KeyInitializer
    static KeyInitializer keys;

  public:
    static inline isl::Id getEmptyKey() {
      return keys.empty;
    }
    static inline isl::Id getTombstoneKey() {
     return keys.tombstone;
    }
    static unsigned getHashValue(const isl::Id& val) {
      return reinterpret_cast<unsigned>(val.keepOrNull());
    }
    static bool isEqual(const isl::Id &LHS, const isl::Id &RHS) {
      return LHS.keepOrNull() == RHS.keepOrNull();
    }
  }; // class DenseMapInfo<isl::Id>
} // namespace llvm


#endif /* ISLPP_ID_H */
