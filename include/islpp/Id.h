#ifndef ISLPP_ID_H
#define ISLPP_ID_H

#include "islpp_common.h"
#include "Obj.h" // baseclass of Id
#include "llvm\Support\raw_ostream.h"

#pragma region Forward declarations
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
#pragma endregion


namespace isl {
  class Id : public Obj<Id, isl_id> {
    friend struct llvm::DenseMapInfo<isl::Id>;

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() ;
    StructTy *addref() const;

  public:
    Id() { }

    /* implicit */ Id(const ObjTy &that) : Obj(that) { }
    /* implicit */ Id(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const;
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region Creational
  public:
    static Id create(Ctx *ctx, const char *name, void *user = nullptr);
#pragma endregion


#pragma region Properties
    ISLPP_EXSITU_ATTRS const char *getName() ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS void *getUser() ISLPP_EXSITU_FUNCTION;
    template<typename T> ISLPP_EXSITU_ATTRS T getUser() ISLPP_EXSITU_FUNCTION { return static_cast<T>(getUser()); }

    /// Set the function to be executued when the last user frees this Id
    /// NOTE: Id's are interned and the freefunc does not change the Id's identity, therefore the freefunc applies to all id's with this identity (name + user)
    Id setFreeUser(void (*freefunc)(void *)) ISLPP_EXSITU_FUNCTION;
    void setFreeUser_inplace(void (*freefunc)(void *)) ISLPP_INPLACE_FUNCTION;
    Id setFreeUser_consume(void (*freefunc)(void *)) ISLPP_CONSUME_FUNCTION;
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Id setFreeUser(void (*freefunc)(void *)) &&;
#endif
#pragma endregion
  }; // class Id


  static inline Id enwrap(__isl_take isl_id *id) { return Id::enwrap(id); }
  static inline Id enwrapCopy(__isl_take isl_id *id) { return Id::enwrapCopy(id); }

  Id setFreeUser(Id id, void (*freefunc)(void *));
  Id setFreeUser(Id &&id, void (*freefunc)(void *));

  static inline bool operator==(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()==rhs.keepOrNull(); }
  static inline bool operator!=(const Id &lhs, const Id &rhs) { return lhs.keepOrNull()!=rhs.keepOrNull(); }

  static inline void swap(Id &lhs, Id &rhs) { isl::Id::swap(lhs, lhs); }

  static inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Id &id) {
    id.print(os);
    return os;
  }

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
    static unsigned getHashValue(const isl::Id& val);
    static bool isEqual(const isl::Id &LHS, const isl::Id &RHS) {
      return LHS.keepOrNull() == RHS.keepOrNull();
    }
  }; // class DenseMapInfo<isl::Id>

} // namespace llvm
#endif /* ISLPP_ID_H */
