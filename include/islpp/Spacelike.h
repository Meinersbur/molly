#ifndef ISLPP_SPACELIKE_H
#define ISLPP_SPACELIKE_H

#include "islpp_common.h"
#include <isl/space.h> // enum isl_dim_type
#include "Obj.h" // class Obj (base of Map)
#include "Id.h" // Id::wrap
#include "Dim.h"

namespace isl {
  class Space;
  class Id;
  class Dim;
} // namespace isl


namespace isl {

  template <typename S>
  class SpacelikeTypeDimIterator : public std::iterator<std::forward_iterator_tag, Dim> {
    typedef S SpaceTy;

  private:
    isl_dim_type curType;
    unsigned curTypeDims;
    unsigned curPos;

  public:
    explicit SpacelikeTypeDimIterator(isl_dim_type type, unsigned typeDims, unsigned pos) : curType(type), curTypeDims(typeDims), curPos(pos) {}

    bool isEnd() { return curPos >= curTypeDims; }

    Dim operator*() {
      assert(!isEnd());
      assert(curPos < curTypeDims);
      return Dim(curType, curPos);
    }

    iterator &operator++/*preincrement*/() {
      assert(!isEnd());
      curPos+=1;
      return *this;
    }

    iterator operator++/*postincrement*/(int) {
      iterator tmp = *this;
      this->operator++();
      return tmp;
    }

  }; // class SpacelikeTypeDimIterator

  template<typename S>
  static inline bool operator==(const SpacelikeTypeDimIterator<S> &lhs, const SpacelikeTypeDimIterator<S> &rhs) {
    assert(lhs.curTypeDims == rhs.curTypeDims);
    ssert(lhs.curType == rhs.curType);
    return lhs.curPos == rhs.curPos;
  }

  template<typename S>
  static inline bool operator!=(const SpacelikeTypeDimIterator<S> &lhs, const SpacelikeTypeDimIterator<S> &rhs) {
    assert(lhs.curTypeDims == rhs.curTypeDims);
    ssert(lhs.curType == rhs.curType);
    return lhs.curPos != rhs.curPos;
  }



  template <typename S>
  class SpacelikeDimIterator : public std::iterator<std::forward_iterator_tag, Dim> {
    typedef S SpaceTy;

    template<typename S1>
    friend bool operator==(const SpacelikeDimIterator<S1> &lhs, const SpacelikeDimIterator<S1> &rhs);
    template<typename S1>
    friend bool operator!=(const SpacelikeDimIterator<S1> &lhs, const SpacelikeDimIterator<S1> &rhs);
  private:
    SpaceTy space;
    DimType typeFilter;

    isl_dim_type curType;
    unsigned curTypeDims;
    unsigned curPos;

    void setType(isl_dim_type type) {
      assert(type <= isl_dim_all);
      curType = type;
      if (1<<curType & typeFilter)
        curTypeDims = space.dim(type);
      else
        curTypeDims = 0; // Do not call space.dim(type) on types that we are not going to iterate
      curPos = 0;
    }

    void jumpToValid() {
      while (true) {
        if (curType >= isl_dim_all)
          break; // end()

        if (curPos >= curTypeDims) {
          setType(isl_dim_type(curType + 1));
          continue;
        }

        break;
      }
    }

  public:
    explicit SpacelikeDimIterator(SpaceTy space, DimType typeFilter) : space(space), typeFilter(typeFilter) { setType(isl_dim_cst); jumpToValid(); }
    explicit SpacelikeDimIterator(SpaceTy space, DimType typeFilter, isl_dim_type type, int pos) : space(space), typeFilter(typeFilter) { setType(type); this->curPos = pos; }

    bool isEnd() { return curType >= isl_dim_all; }

    Dim operator*() {
      assert(!isEnd());
      assert(curPos < curTypeDims);
      return Dim::enwrap(space.getSpacelike(), curType, curPos);
      //return Dim::enwrap(curType, curPos, space.getDimIdOrNull(curType, curPos), curTypeDims, space.getTupleIdOrNull(curType));
    }

    iterator &operator++/*preincrement*/() {
      assert(!isEnd());
      curPos+=1;
      jumpToValid();
      return *this;
    }

    iterator operator++/*postincrement*/(int) {
      iterator tmp = *this;
      curPos+=1;
      jumpToValid();
      return tmp;
    }
  }; // class SpacelikeDimIterator

  template<typename S>
  static inline bool operator==(const SpacelikeDimIterator<S> &lhs, const SpacelikeDimIterator<S> &rhs) {
    //assert(lhs.space == rhs.space);
    return (lhs.curType == rhs.curType) && (lhs.curPos == rhs.curPos);
  }

  template<typename S>
  static inline bool operator!=(const SpacelikeDimIterator<S> &lhs, const SpacelikeDimIterator<S> &rhs) {
    //assert(lhs.space == rhs.space);
    return (lhs.curType != rhs.curType) || (lhs.curPos != rhs.curPos);
  }


  // TODO: Conditionally enable_if Set/Map operations depending on D
  template <typename D>
  class Spacelike3 {
    typedef D SpaceTy;

  private:
    SpaceTy *getDerived() { return static_cast<D*>(this); }
    const SpaceTy *getDerived() const { return static_cast<const D*>(this); }

#pragma region To be implementend by Derived
   //friend class isl::Spacelike3<ObjTy>;
  public:
    // mandatory
    //Space getSpace() const;
    //Space getSpacelike() const;
    //LocalSpace getSpacelike() const;

  protected:
    // mandatory
    //void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER;
    //void setDimId_internal(isl_dim_type type, unsigned, Id &&id) ISLPP_INPLACE_QUALIFIER;

    // optional, usually it doesn't make sense to call this of an isl::(Basic)Map or isl::(Basic)Set; used for assertions
    bool isSet() const { return getDerived()->getSpace().isSetSpace(); }
    bool isMap() const { return getDerived()->getSpace().isMapSpace(); }

  public:
    // mandatory
    //void resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_QUALIFIER { give(isl_space_reset_tuple_id(take(), type)); }
    //void resetDimId_inplace(isl_dim_type type, unsigned pos) ISLPP_INPLACE_QUALIFIER { give(isl_space_reset_dim_id(take(), type, pos)); }

    //void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER;
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER;
    //void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER;

    // optional, default implementations exist
    unsigned dim(isl_dim_type type) const { return getDerived()->getSpacelike().dim(type); }
    int findDimById(isl_dim_type type, const Id &id) const { return getDerived()->getSpacelike().findDimById(type, id); }

    bool hasTupleId(isl_dim_type type) const { return getDerived()->getSpacelike().hasTupleId(type); }
    const char *getTupleName(isl_dim_type type) const { return getDerived()->getSpacelike().getTupleName(type); }
    Id getTupleId(isl_dim_type type) const { return getDerived()->getSpacelike().getTupleId(type); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_inplace(type, getDerived()->getCtx()->createId(s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return getDerived()->getSpacelike().hasDimId(type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return getDerived()->getSpacelike().getDimName(type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return getDerived()->getSpacelike().getDimId(type, pos); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { getDerived()->setDimId_inplace(type, pos, getDerived()->getCtx()->createId(s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { getDerived()->insertDims_inplace(type, dim(type), count); }
#pragma endregion


  protected:
    bool findDim(const Dim &dim, isl_dim_type &type, unsigned &pos) {
      type = dim.getType();

      //TODO: May also check tuple names

      switch (type) {
      case isl_dim_param: {
        // Param dims are identified by id
        if (!dim.hasId())
          return false;
        auto retval = getDerived()->findDimById(type, dim.getId());
        if (retval<0) 
          return false; // Not found
        pos = retval;
        return true;
                          } break;
      case isl_dim_in:
      case isl_dim_out: {
        // Are identified by position
        pos = dim.getPos();

        // Consistency check
        if (getDerived()->dim(type) != dim.getTupleDims())
          return false; // These are different spaces

#ifndef NDEBUG
        auto thatName = dim.getName();
        auto thisName = getDerived()->getDimName(type, pos);
        assert(!!thatName == !!thisName);
        assert(((thatName == thisName) || (strcmp(thatName, thisName) == 0)) && "Give same dimensions the same id/name");

        auto thatId = dim.hasId() ? dim.getId() : Id();
        auto thisId = getDerived()->hasDimId(type, pos) ? getDerived()->getDimId(type, pos) : Id();
        assert(thatId == thisId && "Please give same dimensions the same id");
#endif

        return true;
                        } break;
      default:
        return false;
      }
    }

  public:
    //unsigned dim(isl_dim_type type) const;
    unsigned getParamDimCount() const { return getDerived()->dim(isl_dim_param); }
    unsigned getSetDimCount() const { assert(getDerived()->isSet()); return getDerived()->dim(isl_dim_set); }
    unsigned getInDimCount() const { assert(getDerived()->isMap()); return getDerived()->dim(isl_dim_in); }
    unsigned getOutDimCount() const { assert(getDerived()->isMap()); return getDerived()->dim(isl_dim_out); }
    unsigned getDivDimCount() const { return getDerived()->dim(isl_dim_div); }
    unsigned getAllDimCount() const { 
      auto result = getDerived()->dim(isl_dim_all);
      assert(result = getParamDimCount() + getSetDimCount() +  getInDimCount() + getOutDimCount() + getDivDimCount());
      return result;
    }

    typedef SpacelikeDimIterator<D> dim_iterator;
    dim_iterator dim_begin() const { return dim_iterator(*getDerived(), DimTypeFlags::All); }
    dim_iterator dim_end() const { return dim_iterator(*getDerived(), DimTypeFlags::All, isl_dim_all, 0); }


    Id getTupleIdOrNull(isl_dim_type type) const {
      if (getDerived()->hasTupleId(type))
        return getDerived()->getTupleId(type);
      return Id();
    }

    Id getDimIdOrNull(isl_dim_type type, unsigned pos) const {
      if (getDerived()->hasDimId(type, pos))
        return getDerived()->getDimId(type, pos);
      return Id();
    }

    bool hasInTupleId() const { assert(getDerived()->isMap()); return hasTupleId(isl_dim_in); }
    bool hasOutTupleId() const { assert(getDerived()->isMap()); return hasTupleId(isl_dim_out); }
    bool hasSetTupleId() const { assert(getDerived()->isSet()); return hasTupleId(isl_dim_set); }

    Id getInTupleId() const { return getDerived()->getTupleId(isl_dim_in); }
    Id getOutTupleId() const { return getDerived()->getTupleId(isl_dim_out); }
    Id getSetTupleId() const { return getDerived()->getTupleId(isl_dim_set); }

    void setTupleId_inplace(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(type, std::move(id)); }
    void setTupleId_inplace(isl_dim_type type, const Id &id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(type, id.copy()); }
    SpaceTy setTupleId(isl_dim_type type, Id &&id) { auto result = getDerived()->copy(); result.setTupleId_internal(type, std::move(id)); return std::move(result); }
    SpaceTy setTupleId(isl_dim_type type, const Id &id) { auto result = getDerived()->copy(); result.setTupleId_internal(type, id.copy()); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setTupleId(isl_dim_type type, Id &&id) && { getDerived()->setTupleId_internal(type, std::move(id)); return std::move(*this); }
    SpaceTy setTupleId(isl_dim_type type, const Id &id) && { getDerived()->setTupleId_internal(type, id.copy()); return std::move(*this); }
#endif

    void setInTupleId_inplace(Id &&id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_in, std::move(id)); }
    void setInTupleId_inplace(const Id &id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_in, id.copy()); }
    SpaceTy setInTupleId(Id &&id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_in, std::move(id)); return std::move(result); }
    SpaceTy setInTupleId(const Id &id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_in, id.copy()); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setInTupleId(Id &&id) && { getDerived()->setTupleId_internal(isl_dim_in, std::move(id)); return std::move(*this); }
    SpaceTy setInTupleId(const Id &id) && { getDerived()->setTupleId_internal(isl_dim_in, id.copy()); return std::move(*this); }
#endif

    void setOutTupleId_inplace(Id &&id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_out, std::move(id)); }
    void setOutTupleId_inplace(const Id &id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_out, id.copy()); }
    SpaceTy setOutTupleId(Id &&id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_out, std::move(id)); return std::move(result); }
    SpaceTy setOutTupleId(const Id &id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_out, id.copy()); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setOutTupleId(Id &&id) && { getDerived()->setTupleId_internal(isl_dim_out, std::move(id)); return std::move(*this); }
    SpaceTy setOutTupleId(const Id &id) && { getDerived()->setTupleId_internal(isl_dim_out, id.copy()); return std::move(*this); }
#endif

    void setSetTupleId_inplace(Id &&id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_set, std::move(id)); }
    void setSetTupleId_inplace(const Id &id) ISLPP_INPLACE_QUALIFIER { getDerived()->setTupleId_internal(isl_dim_set, id.copy()); }
    SpaceTy setSetTupleId(Id &&id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_set, std::move(id)); return std::move(result); }
    SpaceTy setSetTupleId(const Id &id) { auto result = getDerived()->copy(); result.setTupleId_internal(isl_dim_set, id.copy()); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setSetTupleId(Id &&id) && { getDerived()->setTupleId_internal(isl_dim_set, std::move(id)); return std::move(*this); }
    SpaceTy setSetTupleId(const Id &id) && { getDerived()->setTupleId_internal(isl_dim_set, id.copy()); return std::move(*this); }
#endif

    SpaceTy setTupleName(isl_dim_type type, const char *s) { auto result = getDerived()->copy(); result.setTupleName_inplace(type, s); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setTupleName(isl_dim_type type, const char *s) && { getDerived()->setTupleName_inplace(type, s); return std::move(*this); }
#endif

    void setDimId_inplace(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { getDerived()->setDimId_internal(type, std::move(id)); }
    void setDimId_inplace(isl_dim_type type, unsigned pos,const Id &id) ISLPP_INPLACE_QUALIFIER { getDerived()->setDimId_internal(type, id.copy()); }
    SpaceTy setDimId(isl_dim_type type, unsigned pos,Id &&id) { auto result = getDerived()->copy(); result.setDimId_internal(type, std::move(id)); return std::move(result); }
    SpaceTy setDimId(isl_dim_type type, unsigned pos,const Id &id) { auto result = getDerived()->copy(); result.setDimId_internal(type, id.copy()); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setDimId(isl_dim_type type, Id &&id) && { getDerived()->setDimId_internal(type, std::move(id)); return std::move(*this); }
    SpaceTy setDimId(isl_dim_type type, const Id &id) && { getDerived()->setDimId_internal(type, id.copy()); return std::move(*this); }
#endif

    SpaceTy setDimName(isl_dim_type type,  unsigned pos,const char *s) { auto result = getDerived()->copy(); result.setDimName_inplace(type, pos, s); return std::move(result); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy setDimName(isl_dim_type type,  unsigned pos,const char *s) && { getDerived()->setDimName_inplace(type, pos, s); return std::move(*this); }
#endif

    SpaceTy addDims(isl_dim_type type, unsigned count) const { auto result = getDerived()->copy(); result.addDims_inplace(type, count); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy addDims(isl_dim_type type, unsigned count) && { getDerived()->addDims_inplace(type, count); return this->move(); }
#endif

    SpaceTy insertDims(isl_dim_type type, unsigned count) const { auto result = getDerived()->copy(); result.insertDims_inplace(type, count); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy insertDims(isl_dim_type type, unsigned count) && { getDerived()->insertDims_inplace(type, count); return this->move(); }
#endif

    SpaceTy moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count)  { auto result = getDerived()->copy(); result.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, count); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) && { getDerived()->moveDims_inplace(dst_type, dst_pos, src_pos, count); return std::move(*this); }
#endif

    SpaceTy removeDims(isl_dim_type type, unsigned first, unsigned count)  { auto result = getDerived()->copy(); result.removeDims_inplace(type, first, count); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    SpaceTy removeDims(isl_dim_type type, unsigned first, unsigned count) && { getDerived()->removeDims_inplace(type, first, count); return std::move(*this); }
#endif

    Dim addDim_inplace(isl_dim_type type) ISLPP_INPLACE_QUALIFIER {
      getDerived()->addDims_inplace(type, 1);
      auto dims = getDerived()->dim(type);
      return Dim::enwrap(getDerived()->getSpacelike(), type, dims-1);
    }

    Dim addOutDim_inplace() ISLPP_INPLACE_QUALIFIER {
      return getDerived()->addDim_inplace(isl_dim_out);
    }
     Dim addSetDim_inplace() ISLPP_INPLACE_QUALIFIER {
      return getDerived()->addDim_inplace(isl_dim_out);
    }
  }; // class Spacelike3


  template<typename D> D setTupleName(Spacelike3<D> &&spacelike, isl_dim_type type, const char *s) { spacelike.setTupleName_inplace(type, s); return std::move(spacelike); }
  template<typename D> D setTupleName(const Spacelike3<D> &spacelike, isl_dim_type type, const char *s) { return spacelike.setTupleName(type, s); }
 
    template<typename D> D setTupleId(Spacelike3<D> &&spacelike, isl_dim_type type, Id &&id) { spacelike.setTupleId_inplace(type, std::move(id)); return std::move(spacelike); }
  template<typename D> D setTupleId(const Spacelike3<D> &spacelike, isl_dim_type type, Id &&id) { return spacelike.setTupleId(type, std::move(id)); }
     template<typename D> D setTupleId(Spacelike3<D> &&spacelike, isl_dim_type type, const Id &id) { spacelike.setTupleId_inplace(type, id.copy()); return std::move(spacelike); }
  template<typename D> D setTupleId(const Spacelike3<D> &spacelike, isl_dim_type type, const Id &id) { return spacelike.setTupleId(type, id.copy()); }
 
    template<typename D> D setDimName(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned pos, const char *s) { spacelike.setDimName_inplace(type, pos, s); return std::move(spacelike); }
      template<typename D> D setDimName(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned pos, const char *s) { return spacelike.setDimName(type, pos, s); }

   template<typename D> D setDimId(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned pos, Id &&id) { spacelike.setDimId_inplace(type, pos, std::move(id)); return std::move(spacelike); }
 template<typename D> D setDimId(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned pos, Id &&id) { return spacelike.setDimId(type, pos, std::move(id)); }
    template<typename D> D setDimId(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned pos, const Id &id) { spacelike.setDimId_inplace(type, pos, id.copy()); return std::move(spacelike); }
 template<typename D> D setDimId(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned pos, const Id &id) { return spacelike.setDimId(type, pos, id.copy()); }

  template<typename D> D addDims(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned count) { spacelike.addDims_inplace(type, count); return std::move(spacelike); }
  template<typename D> D addDims(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned count) { return spacelike.addDims(type, count); }

   template<typename D> D insertDims(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned pos, unsigned count) { spacelike.insertDims_inplace(type, pos, count); return std::move(spacelike); }
  template<typename D> D insertDims(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned pos, unsigned count) { return spacelike.insertDims(type, pos, count); }

   template<typename D> D moveDims(Spacelike3<D> &&spacelike, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) { spacelike.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, count); return std::move(spacelike); }
 template<typename D> D moveDims(const Spacelike3<D> &spacelike, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) { return spacelike.moveDims(dst_type, dst_pos, src_type, src_pos, count); }
 
   template<typename D> D removeDims(Spacelike3<D> &&spacelike, isl_dim_type type, unsigned first, unsigned count) { spacelike.removeDims_inplace(type, first, count); return std::move(spacelike); }
  template<typename D> D removeDims(const Spacelike3<D> &spacelike, isl_dim_type type, unsigned first, unsigned count) { return spacelike.removeDims(type, first, count); }




  template <typename T> // Curiously recursive template pattern
  class Spacelike2 {
  protected :
    virtual T copy() const = 0;
    virtual Ctx *getCtx() const = 0;

  public:
    virtual ~Spacelike2() {} // To silence compiler warnings

    virtual unsigned dim(isl_dim_type type) const = 0;  
    unsigned getParamDimCount() { return dim(isl_dim_param); }
    unsigned getSetDimCount() { assert(dim(isl_dim_in) == 0 && "Space must be set-like"); return dim(isl_dim_set); }
    unsigned getInDimCount() { assert(dim(isl_dim_in) > 0 && "Space must be map-like"); return dim(isl_dim_in); }
    unsigned getOutDimCount() { assert(dim(isl_dim_in) > 0 && "Space must be map-like"); return dim(isl_dim_out); }

    virtual bool hasTupleId(isl_dim_type type) const { return getTupleId(type).isValid(); }
    virtual Id getTupleId(isl_dim_type type) const = 0;
    virtual void setTupleId_inplace(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER = 0;
    void setTupleId_inplace(isl_dim_type type, const Id &id) ISLPP_INPLACE_QUALIFIER { setTupleId_inplace(type, copy(id)); }
    T setTupleId(isl_dim_type type, const Id  &id) const { auto result = copy(); result.setTupleId_inplace(type, copy(id)); return result; }
    T setTupleId(isl_dim_type type,       Id &&id) const { auto result = copy(); result.setTupleId_inplace(type, move(id)); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setTupleId(isl_dim_type type, const Id  &id) && { setTupleId_inplace(type, copy(id)); return *this; }
    T setTupleId(isl_dim_type type,       Id &&id) && { setTupleId_inplace(type, move(id)); return *this; }
#endif

    virtual bool hasTupleName(isl_dim_type type) const { return getTupleName(type); }
    virtual const char *getTupleName(isl_dim_type type) const { return getTupleId(type).getName(); }
    virtual void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { setTupleId_inplace(type, Id::enwrap(isl_id_alloc(getCtx()->keep(), s, nullptr))); }
    T setTupleName(isl_dim_type type, const char *s) const { auto result = copy(); result.setTupleName_inplace(type, s); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setTupleName(isl_dim_type type, const char *s) && { setTupleName_inplace(type, s); return *this; }
#endif

    virtual bool hasDimId(isl_dim_type type, unsigned pos) const { return getDimId(type, pos).isValid(); }
    virtual Id getDimId(isl_dim_type type, unsigned pos) const = 0;
    virtual void setDimId_inplace(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER = 0;
    void setDimId_inplace(isl_dim_type type, unsigned pos, const Id &id) ISLPP_INPLACE_QUALIFIER { setDimId_inplace(type, pos, copy(id)); }
    T setDimId(isl_dim_type type, unsigned pos,const Id  &id) const { auto result = copy(); result.setDimId_inplace(type, pos, copy(id)); return result; }
    T setDimId(isl_dim_type type, unsigned pos,      Id &&id) const { auto result = copy(); result.setDimId_inplace(type, pos, move(id)); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setDimId(isl_dim_type type, unsigned pos,const Id  &id) && { setDimId_inplace(type, pos, copy(id)); return *this; }
    T setDimId(isl_dim_type type, unsigned pos,      Id &&id) && { setDimId_inplace(type, pos, move(id)); return *this; }
#endif

    virtual bool hasDimName(isl_dim_type type, unsigned pos) const { return getDimName(type, pos); }
    virtual const char *getDimName(isl_dim_type type, unsigned pos) const { return getDimId(type, pos).getName(); }
    virtual void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { setDimId_inplace(type, pos, Id::enwrap(isl_id_alloc(getCtx()->keep(), s, nullptr))); }
    T setDimName(isl_dim_type type, unsigned pos,const char *s) const { auto result = copy(); result.setDimName_inplace(type, pos, s); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setDimName(isl_dim_type type, unsigned pos,const char *s) && { setDimName_inplace(type, pos, s); return *this; }
#endif

    virtual void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T insertDims(isl_dim_type type, unsigned pos, unsigned n) const { auto result = copy(); result.insertDims_inplace(type, pos, n); return result; }

    virtual void addDims_inplace(isl_dim_type type, unsigned n) ISLPP_INPLACE_QUALIFIER { insertDims_inplace(type, dim(type), n); }
    T addDims(isl_dim_type type, unsigned n) const { auto result = copy(); result.addDims_inplace(type, n); return result;  }

    virtual void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) const { auto result = copy(); result.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, n); return result; }

    virtual void removeDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T removeDims(isl_dim_type type, unsigned first, unsigned n) const { auto result = copy(); result.removeDims_inplace(type, first, n); return result; }

    void dropDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER  { removeDims_inplace(type, first, n); }
    T dropDims(isl_dim_type type, unsigned first, unsigned n) const { return removeDims(type,first,n); }
  }; 


  class Spacelike : public Obj {
  protected:
    bool findDim(const Dim &dim, isl_dim_type &type, unsigned &pos);
  public:
    virtual ~Spacelike() {}

    virtual Space getSpace() const = 0;

    virtual unsigned dim(isl_dim_type type) const = 0;    
    unsigned dimParam() const { return dim(isl_dim_param); }

    virtual bool hasTupleId(isl_dim_type type) const;
    virtual Id getTupleId(isl_dim_type type) const;
    Id getTupleIdOrNull(isl_dim_type type) { return hasTupleId(type) ? getTupleId(type) : Id(); }
    virtual void setTupleId(isl_dim_type type, Id &&id) = 0;

    virtual bool hasTupleName(isl_dim_type type) const;
    virtual const char *getTupleName(isl_dim_type type) const;
    virtual void setTupleName(isl_dim_type type, const char *s) = 0;

    virtual bool hasDimId(isl_dim_type type, unsigned pos) const;
    virtual Id getDimId(isl_dim_type type, unsigned pos) const;
    Id getDimIdOrNull(isl_dim_type type, unsigned pos) const;
    virtual void setDimId(isl_dim_type type, unsigned pos, Id &&id) = 0;
    virtual int findDimById(isl_dim_type type, const Id &id) const;

    virtual bool hasDimName(isl_dim_type type, unsigned pos) const;
    virtual const char *getDimName(isl_dim_type type, unsigned pos) const;
    virtual void setDimName(isl_dim_type type, unsigned pos, const char *s)  = 0;
    //virtual int findDimByName(isl_dim_type type, const char *name) const;

    virtual void insertDims(isl_dim_type type, unsigned pos, unsigned n) = 0;
    virtual void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) = 0;
    virtual void addDims(isl_dim_type type, unsigned n) = 0;
    Dim addDim(isl_dim_type type);
    Dim addOutDim();

    virtual void removeDims(isl_dim_type type, unsigned first, unsigned n) = 0;
    //virtual void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) = 0;

    void addDims_inplace(isl_dim_type type, unsigned n) ISLPP_INPLACE_QUALIFIER { addDims(type, n); }
  }; // class Spacelike

} // namepspace isl
#endif /* ISLPP_SPACELIKE_H */
