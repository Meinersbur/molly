#ifndef ISLPP_SPACELIKE_H
#define ISLPP_SPACELIKE_H

#include "islpp_common.h"
#include <isl/space.h> // enum isl_dim_type
#include "Obj.h" // class Obj (base of Map)
#include "Id.h" // Id::wrap
#include "Dim.h"
#include "DimRange.h"

namespace isl {
  class Space;
  class Id;
  class Dim;
} // namespace isl


namespace isl {

#pragma region Iterator over dimensions
  template <typename S>
  class SpacelikeTypeDimIterator : public std::iterator<std::forward_iterator_tag, Dim> {
    typedef S SpaceTy;

  private:
    isl_dim_type curType;
    count_t curTypeDims;
    pos_t curPos;

  public:
    explicit SpacelikeTypeDimIterator(isl_dim_type type, count_t typeDims, pos_t pos) : curType(type), curTypeDims(typeDims), curPos(pos) {}

    bool isEnd() { return curPos >= curTypeDims; }

    Dim operator*() {
      assert(!isEnd());
      assert(curPos < curTypeDims);
      return Dim(curType, curPos);
    }

    iterator &operator++/*preincrement*/() {
      assert(!isEnd());
      curPos += 1;
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
    count_t curTypeDims;
    pos_t curPos;

    void setType(isl_dim_type type) {
      assert(type <= isl_dim_all);
      curType = type;
      if (1 << curType & typeFilter)
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
      return Dim::enwrap(curType, curPos, space.getSpace());
      //return Dim::enwrap(curType, curPos, space.getDimIdOrNull(curType, curPos), curTypeDims, space.getTupleIdOrNull(curType));
    }

    iterator &operator++/*preincrement*/() {
      assert(!isEnd());
      curPos += 1;
      jumpToValid();
      return *this;
    }

    iterator operator++/*postincrement*/(int) {
      iterator tmp = *this;
      curPos += 1;
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
#pragma endregion


  bool spacelike_matchesMapSpace(const Space &, const Space &domainSpace, const Space &rangeSpace);
  bool spacelike_matchesSpace(const Space &, const Space &that);
  bool spacelike_matchesSetSpace(const Space &, const Space &);
  bool spacelike_matchesMapSpace(const Space &, const Space &that);

  // TODO: Conditionally enable_if Set/Map operations depending on D
  // Actually, I'd prefer a code generator that generatores complete classes isl::Set, isl::Map without deriving from anything
  // This would avoid problems with incomplete classes and we could hide any definitions in .cpp files. The link-time-optimization has to do the inlining then
  template <typename D>
  class Spacelike {
    typedef D SpaceTy;

    //private:
  public:
    SpaceTy *getDerived() { return static_cast<D*>(this); }
    const SpaceTy *getDerived() const { return static_cast<const D*>(this); }

#pragma region To be implementend by Derived
    //  friend class isl::Spacelike<ObjTy>;
  public:
    //ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION;
    //ISLPP_PROJECTION_ATTRS Space/LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION;

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION { return getDerived()->getSpacelike().isParams(); }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION { return getDerived()->getSpacelike().isSet(); }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION { return getDerived()->getSpacelike().isMap(); }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return getDerived()->getSpacelike().dim(type); }
    ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpace().findDimById(type, id); /* none of the additional dim type have named dimension */ }

    ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().hasTupleName(type); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().getTupleName(type); }
      //ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION;
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().hasTupleId(type); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().getTupleId(type); }
      //ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{ getDerived()->setTupleName_inplace(type, nullptr); }

    ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().hasDimName(type, pos); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().getDimName(type, pos); }
      //ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION;
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().hasDimId(type, pos); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return getDerived()->getSpacelike().getDimId(type, pos); }
      //ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION{ getDerived()->setDimName_inplace(type, pos, nullptr); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ getDerived()->insertDims_internal(type, dim(type), count); }
      //ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_space_insert_dims(take(), type, pos, count)); }
  public:
    //ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    //ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_space_drop_dims(take(), type, first, count)); }
#pragma endregion


  protected:
    bool findDim(const Dim &dim, isl_dim_type &type, pos_t &pos) {
      type = dim.getType();

      //TODO: May also check tuple names

      switch (type) {
      case isl_dim_param: {
        // Param dims are identified by id
        if (!dim.hasId())
          return false;
        auto retval = getDerived()->findDimById(type, dim.getId());
        if (retval < 0)
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
    //coun_t dim(isl_dim_type type) const;
    count_t getParamDimCount() const { return getDerived()->dim(isl_dim_param); }
    count_t getSetDimCount() const { assert(getDerived()->isSet()); return getDerived()->dim(isl_dim_set); }
    count_t getInDimCount() const { assert(getDerived()->isMap()); return getDerived()->dim(isl_dim_in); }
    count_t getOutDimCount() const { assert(getDerived()->isMap()); return getDerived()->dim(isl_dim_out); }
    count_t getDivDimCount() const { return getDerived()->dim(isl_dim_div); }
    count_t getAllDimCount() const {
      auto result = getDerived()->dim(isl_dim_all);
      assert(result == dim(isl_dim_param) + dim(isl_dim_in) + dim(isl_dim_out) + dim(isl_dim_div));
      return result;
    }

    typedef SpacelikeDimIterator<D> dim_iterator;
    dim_iterator dim_begin() const { return dim_iterator(*getDerived(), DimTypeFlags::All); }
    dim_iterator dim_end() const { return dim_iterator(*getDerived(), DimTypeFlags::All, isl_dim_all, 0); }

    // These would need a dependency on Space.h; Space.h #includes Spacelike.h, so we'd have a circular dependency
    //Space getParamsSpace() const { return getDerived()->getSpace().getParamsSpace(); }
    //Space getDomainSpace() const { return getDerived()->getSpace().getDomainSpace(); }
    //Space getRangeSpace() const { return getDerived()->getSpace().getRangeSpace(); }

    Id getTupleIdOrNull(isl_dim_type type) const {
      if (getDerived()->hasTupleId(type))
        return getDerived()->getTupleId(type);
      return Id();
    }

    Id getDimIdOrNull(isl_dim_type type, pos_t pos) const {
      if (getDerived()->hasDimId(type, pos))
        return getDerived()->getDimId(type, pos);
      return Id();
    }

    bool hasInTupleId() const { assert(getDerived()->isMap()); return getDerived()->hasTupleId(isl_dim_in); }
    bool hasOutTupleId() const { assert(getDerived()->isMap()); return getDerived()->hasTupleId(isl_dim_out); }
    bool hasSetTupleId() const { assert(getDerived()->isSet()); return getDerived()->hasTupleId(isl_dim_set); }

    ISLPP_PROJECTION_ATTRS bool hasInTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->hasTupleName(isl_dim_in); }
    ISLPP_PROJECTION_ATTRS bool hasOutTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->hasTupleName(isl_dim_out); }
    ISLPP_PROJECTION_ATTRS bool hasSetTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isSet()); return getDerived()->hasTupleName(isl_dim_set); }

    Id getInTupleId() const { return getDerived()->getTupleId(isl_dim_in); }
    Id getOutTupleId() const { return getDerived()->getTupleId(isl_dim_out); }
    Id getSetTupleId() const { return getDerived()->getTupleId(isl_dim_set); }

    ISLPP_PROJECTION_ATTRS const char* getInTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isMap());  return getDerived()->getTupleName(isl_dim_in); }
    ISLPP_PROJECTION_ATTRS  const char* getOutTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isMap());  return getDerived()->getTupleName(isl_dim_out); }
    ISLPP_PROJECTION_ATTRS const char* getSetTupleName() ISLPP_PROJECTION_FUNCTION{ assert(getDerived()->isSet());  return getDerived()->getTupleName(isl_dim_set); }

    Id getInTupleIdOrNull() const { assert(getDerived()->isMap()); return getDerived()->hasTupleId(isl_dim_in) ? getDerived()->getTupleId(isl_dim_in) : Id(); }
    Id getOutTupleIdOrNull() const { assert(getDerived()->isMap()); return getDerived()->hasTupleId(isl_dim_out) ? getDerived()->getTupleId(isl_dim_out) : Id(); }
    Id getSetTupleIdOrNull() const { assert(getDerived()->isSet()); return getDerived()->hasTupleId(isl_dim_set) ? getDerived()->getTupleId(isl_dim_set) : Id(); }

    Id getParamDimId(pos_t pos) const { return getDerived()->getDimId(isl_dim_param, pos); }

    SpaceTy setTupleId(isl_dim_type type, Id id) const { auto result = getDerived()->copy(); result.setTupleId_inplace(type, std::move(id)); return result; }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setTupleId(isl_dim_type type, Id id) && { getDerived()->setTupleId_inplace(type, std::move(id)); return std::move(*this); }
#endif

    SpaceTy setInTupleId(Id id) const { assert(getDerived()->isMap()); auto result = getDerived()->copy(); result.setTupleId_inplace(isl_dim_in, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setInTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setTupleId_inplace(isl_dim_in, std::move(id)); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setInTupleId(Id id) && { assert(getDerived()->isMap()); getDerived()->setTupleId_internal(isl_dim_out, std::move(id)); return std::move(*this); }
#endif

    SpaceTy setOutTupleId(Id id) const { assert(getDerived()->isMap()); auto result = getDerived()->copy(); result.setTupleId_inplace(isl_dim_out, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setOutTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setTupleId_inplace(isl_dim_out, std::move(id)); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setOutTupleId(Id id) && { assert(getDerived()->isMap()); getDerived()->setTupleId_internal(isl_dim_out, std::move(id)); return std::move(*this); }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy setSetTupleId(Id id) ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isSet()); auto result = getDerived()->copy(); result.setTupleId_inplace(isl_dim_set, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setSetTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isSet()); getDerived()->setTupleId_inplace(isl_dim_set, std::move(id)); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setSetTupleId(Id id) && { assert(getDerived()->isSet()); getDerived()->setTupleId_internal(isl_dim_set, std::move(id)); return std::move(*this); }
#endif

    SpaceTy setTupleName(isl_dim_type type, const char *s) const { auto result = getDerived()->copy(); result.setTupleName_inplace(type, s); return result; }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setTupleName(isl_dim_type type, const char *s) && { getDerived()->setTupleName_inplace(type, s); return std::move(*this); }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy resetTupleId(isl_dim_type type) ISLPP_EXSITU_FUNCTION{ auto result = getDerived()->copy(); result.resetTupleId_inplace(type); return result; }
      //ISLPP_EXSITU_ATTRS SpaceTy resetTupleId(isl_dim_type type) ISLPP_EXSITU_FUNCTION { auto result = getDerived()->copy(); result.resetTupleId_inplace(type); return result; }

    SpaceTy setDimId(isl_dim_type type, pos_t pos, Id id) const { auto result = getDerived()->copy(); result.setDimId_inplace(type, pos, std::move(id)); return result; }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setDimId(isl_dim_type type, Id id) && { getDerived()->setDimId_inplace(type, std::move(id)); return std::move(*this); }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy setInDimId(pos_t pos, Id id) ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isMap()); auto result = getDerived()->copy(); result.setDimId_internal(isl_dim_in, pos, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setInDimId_inplace(pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setDimId_inplace(isl_dim_in, pos, std::move(id)); }
    ISLPP_CONSUME_ATTRS SpaceTy setInDimId_consume(Id id) ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setDimId_inplace(isl_dim_in, std::move(id)); return std::move(*this); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setInDimId(Id id) && { assert(getDerived()->isMap()); getDerived()->setDimId_internal(isl_dim_in, std::move(id)); return std::move(*this); }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy setOutDimId(pos_t pos, Id id) ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isMap()); auto result = getDerived()->copy(); result.setDimId_internal(isl_dim_out, pos, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setOutDimId_inplace(pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setDimId_inplace(isl_dim_out, pos, std::move(id)); }
    ISLPP_CONSUME_ATTRS SpaceTy setOutDimId_consume(Id id) ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isMap()); getDerived()->setDimId_inplace(isl_dim_out, std::move(id)); return std::move(*this); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setOutDimId(Id id) && { assert(getDerived()->isMap()); getDerived()->setDimId_internal(isl_dim_out, std::move(id)); return std::move(*this); }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy setSetDimId(pos_t pos, Id id) ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isSet()); auto result = getDerived()->copy(); result.setDimId_internal(isl_dim_set, pos, std::move(id)); return result; }
    ISLPP_INPLACE_ATTRS void setSetDimId_inplace(pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isSet()); getDerived()->setDimId_inplace(isl_dim_set, pos, std::move(id)); }
    ISLPP_CONSUME_ATTRS SpaceTy setSetDimId_consume(Id id) ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isSet()); getDerived()->setDimId_inplace(isl_dim_set, std::move(id)); return std::move(*this); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setSetDimId(Id id) && { assert(getDerived()->isSet()); getDerived()->setDimId_internal(isl_dim_set, std::move(id)); return std::move(*this); }
#endif

    SpaceTy setParamDimId(pos_t pos, Id id) const { return setDimId(isl_dim_param, pos, std::move(id)); }
    //SpaceTy setInDimId(pos_t pos, Id id) const { assert( isMap()); return setDimId(isl_dim_in, pos, std::move(id)); }
    //SpaceTy setOutDimId(pos_t pos, Id id) const { assert(isMap()); return setDimId(isl_dim_out, pos, std::move(id)); }
    //SpaceTy setSetDimId(pos_t pos, Id id) const { assert(isSet()); return setDimId(isl_dim_set, pos, std::move(id)); }

    SpaceTy setDimName(isl_dim_type type, pos_t pos, const char *s) { auto result = getDerived()->copy(); result.setDimName_inplace(type, pos, s); return result; }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy setDimName(isl_dim_type type,  pos_t pos, const char *s) && { getDerived()->setDimName_inplace(type, pos, s); return std::move(*this); }
#endif

    SpaceTy addDims(isl_dim_type type, count_t count) const { auto result = getDerived()->copy(); result.addDims_internal(type, count); return result; }
    ISLPP_INPLACE_ATTRS DimRange addDims_inplace(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{
      auto pos = dim(type);
      getDerived()->addDims_internal(type, count);
      return DimRange::enwrap(type, pos, count, getDerived()->getSpace());
    }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy addDims(isl_dim_type type, count_t count) && { getDerived()->addDims_internal(type, count); return this->move(); }
#endif

    SpaceTy addInDims(count_t count) const { assert(getDerived()->isMap()); return addDims(isl_dim_in, count); }
    SpaceTy addOutDims(count_t count) const { assert(getDerived()->isMap()); return addDims(isl_dim_in, count); }
    SpaceTy addSetDims(count_t count) const { assert(getDerived()->isSet()); return addDims(isl_dim_set, count); }

    SpaceTy insertDims(isl_dim_type type, pos_t pos, count_t count) const { auto result = getDerived()->copy(); result.insertDims_internal(type, pos, count); return result; }
    ISLPP_INPLACE_ATTRS DimRange insertDims_inplace(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{
      getDerived()->insertDims_internal(type, pos, count);
      return DimRange::enwrap(type, pos, count, getDerived()->getSpace());
    }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy insertDims(isl_dim_type type, pos_t pos, count_t count) && { getDerived()->insertDims_internal(type, pos, count); return this->move(); }
#endif

    SpaceTy moveDims(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) const { auto result = getDerived()->copy(); result.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, count); return result; }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy moveDims(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) && { getDerived()->moveDims_inplace(dst_type, dst_pos, src_pos, count); return std::move(*this); }
#endif

    SpaceTy removeDims(isl_dim_type type, pos_t first, count_t count) const { auto result = getDerived()->copy(); result.removeDims_inplace(type, first, count); return result; }
    SpaceTy removeDims_consume(isl_dim_type type, pos_t first, count_t count) const { return getDerived()->removeDims_inplace(type, first, count); return std::move(*this); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy removeDims(isl_dim_type type, pos_t first, count_t count) && { getDerived()->removeDims_inplace(type, first, count); return std::move(*this); }
#endif

    SpaceTy removeDims(const DimRange &range) const { return getDerived()->removeDims(range.getType(), range.getFirst(), range.getCount()); }
    SpaceTy removeDims_consume(const DimRange &range) { return getDerived()->removeDims_consume(range.getType(), range.getFirst(), range.getCount()); }
    void removeDim_inplace(const DimRange &range) ISLPP_INPLACE_FUNCTION{ getDerived()->removeDims_inplace(range.getType(), range.getFirst(), range.getCount()); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    SpaceTy removeDims(const DimRange &range) && { return getDerived()->removeDims_consume(range.getType(), range.getFirst(), range.getCount()); }
#endif

    Dim addDim_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{
      getDerived()->addDims_inplace(type, 1);
      auto dims = getDerived()->dim(type);
      return Dim::enwrap(type, dims - 1, getDerived()->getSpace());
    }

      Dim addOutDim_inplace() ISLPP_INPLACE_FUNCTION{
      return getDerived()->addDim_inplace(isl_dim_out);
    }
      Dim addSetDim_inplace() ISLPP_INPLACE_FUNCTION{
      return getDerived()->addDim_inplace(isl_dim_out);
    }


      Dim findDim(const Id &id) const {
        auto space = getDerived()->getSpace();
        auto resultParam = isl_space_find_dim_by_id(space.keep(), isl_dim_param, id.keep());
        if (resultParam != -1)
          return Dim::enwrap(isl_dim_param, resultParam, space);
        auto resultDomain = isl_space_find_dim_by_id(space.keep(), isl_dim_out, id.keep());
        if (resultDomain != -1)
          return Dim::enwrap(isl_dim_out, resultDomain, space);
        auto resultRange = isl_space_find_dim_by_id(space.keep(), isl_dim_in, id.keep());
        if (resultRange != -1)
          return Dim::enwrap(isl_dim_in, resultRange, space);
        return Dim();
    }


    Dim findDim(isl_dim_type type, pos_t pos) {
      if (pos < getDerived()->dim(type))
        return Dim::enwrap(type, pos, getDerived()->getSpace());
      return Dim();
    }


    Dim getSetDim(pos_t pos) const { assert(isSet()); return Dim::enwrap(isl_dim_set, pos, getDerived()->getSpacelike()); }



#pragma region Matching spaces
    bool matchesSpace(const Space &that) const {
      return spacelike_matchesSpace(getDerived()->getSpace(), that);
    }


    bool matchesSetSpace(const Id &id) const {
      if (!getDerived()->isSet())
        return false;
      if (getDerived()->getSetTupleId() != id)
        return false;
      return true;
    }


    bool matchesSetSpace(const Space &that) const {
      if (!getDerived()->isSet()) {
        assert(!spacelike_matchesSetSpace(getDerived()->getSpace(), that));
        return false;
      }
      return spacelike_matchesSetSpace(getDerived()->getSpace(), that);
    }


    bool matchesMapSpace(const Id &domainId, const Id &rangeId) const {
      if (!getDerived()->isMap())
        return false;
      if (getDerived()->getInTupleId() != domainId)
        return false;
      if (getDerived()->getOutTupleId() != rangeId)
        return false;
      return true;
    }


    bool matchesMapSpace(const Space &domainSpace, const Space &rangeSpace) const {
      if (!getDerived()->isMap()) {
        assert(!spacelike_matchesMapSpace(getDerived()->getSpace(), domainSpace, rangeSpace));
        return false;
      }

      // Try to remove the requirement of isl::space being complete. This doesn't work. getSpace() returns a temporary for which the compiler needs to know how much space it uses.
      // However, getDerived() makes it a dependent lookup, i.e. isl::Space only needs to be complete in the derived classes and therefore becomes a requirement for using Spacelike
      return spacelike_matchesMapSpace(getDerived()->getSpace(), domainSpace, rangeSpace);
    }


    bool matchesMapSpace(const Space &that) const {
      if (!getDerived()->isMap()) {
        assert(!spacelike_matchesMapSpace(getDerived()->getSpace(), that));
        return false;
      }

      return spacelike_matchesMapSpace(getDerived()->getSpace(), that);
    }


    bool matchesMapSpace(const Space &domainSpace, const Id &rangeId)const {
      if (!getDerived()->isMap())
        return false;
      auto space = getDerived()->getSpace();
      return space.match(isl_dim_in, domainSpace, isl_dim_set) && (getOutTupleId() == rangeId);
    }


    bool matchesMapSpace(const Id &domainId, const Space &rangeSpace) const{
      if (!getDerived()->isMap())
        return false;

      auto space = getDerived()->getSpace();
      return (getDerived()->getInTupleId() == domainId) && space.match(isl_dim_out, rangeSpace, isl_dim_set);
    }
#pragma endregion

    //SpaceTy resetSpace(isl_dim_type type) const { auto result = getDerived()->copy(); result.resetSpace_inplace(type); return result; }

    Dim addParamDim_inplace() ISLPP_INPLACE_FUNCTION{ return addDim_inplace(isl_dim_param); }
      Dim addParamDim_inplace(Id id) ISLPP_INPLACE_FUNCTION{
      assert(findDimById(isl_dim_param, id) == -1);
      auto result = getDerived()->addDim_inplace(isl_dim_param);
      getDerived()->setDimId_inplace(result.getType(), result.getPos(), id.move());
      return result;
    }


      /// reset all dimension identifiers of the given type (usually isl_dim_in or isl_dim_out)
      /// Usually dimension identifiers on these dimensions have no meaning
    ISLPP_EXSITU_ATTRS SpaceTy resetDimIds(isl_dim_type type) ISLPP_EXSITU_FUNCTION{ auto result = getDerived()->copy(); result.resetDimIds_inplace(type); return result; }
      ISLPP_INPLACE_ATTRS void resetDimIds_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{
      if (type == isl_dim_all) {
        resetDimIds_inplace(isl_dim_in);
        resetDimIds_inplace(isl_dim_out);
        return;
      }

      auto nDims = getDerived()->dim(type);
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        getDerived()->resetDimId_inplace(type, i);
      }
    }

    ISLPP_EXSITU_ATTRS SpaceTy resetOutTupleId() ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->resetTupleId(isl_dim_out); }
    ISLPP_INPLACE_ATTRS void resetOutTupleId_inplace() ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->resetTupleId_inplace(isl_dim_out); }
    ISLPP_CONSUME_ATTRS SpaceTy resetOutTupleId_consume() ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->resetTupleId_consume(isl_dim_out); }

    ISLPP_EXSITU_ATTRS SpaceTy resetInTupleId() ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->resetTupleId(isl_dim_in); }
    ISLPP_INPLACE_ATTRS void reseInTupleId_inplace() ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isMap()); getDerived()->resetTupleId_inplace(isl_dim_in); }
    ISLPP_CONSUME_ATTRS SpaceTy resetInTupleId_consume() ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isMap()); return getDerived()->resetTupleId_consume(isl_dim_in); }

    ISLPP_EXSITU_ATTRS SpaceTy resetSetTupleId() ISLPP_EXSITU_FUNCTION{ assert(getDerived()->isSet()); return getDerived()->resetTupleId(isl_dim_set); }
    ISLPP_INPLACE_ATTRS void resetSetTupleId_inplace() ISLPP_INPLACE_FUNCTION{ assert(getDerived()->isSet()); getDerived()->resetTupleId_inplace(isl_dim_set); }
    ISLPP_CONSUME_ATTRS SpaceTy resetSetTupleId_consume() ISLPP_CONSUME_FUNCTION{ assert(getDerived()->isSet()); return getDerived()->resetTupleId_consume(isl_dim_set); }

      ISLPP_PROJECTION_ATTRS bool isValidDim(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{
      switch (type) {
      case isl_dim_cst:
      case isl_dim_param:
      case isl_dim_in:
      case isl_dim_out:
      case isl_dim_div: // Only for local spaces?
        return 0 <= pos && pos < dim(type);
      default:
        return false;
      }
    }
  }; // class Spacelike


  template<typename D> D setTupleName(Spacelike<D> &&spacelike, isl_dim_type type, const char *s) { spacelike.setTupleName_inplace(type, s); return std::move(spacelike); }
  template<typename D> D setTupleName(const Spacelike<D> &spacelike, isl_dim_type type, const char *s) { return spacelike.setTupleName(type, s); }

  template<typename D> D setTupleId(Spacelike<D> &&spacelike, isl_dim_type type, Id &&id) { spacelike.setTupleId_inplace(type, std::move(id)); return std::move(spacelike); }
  template<typename D> D setTupleId(const Spacelike<D> &spacelike, isl_dim_type type, Id &&id) { return spacelike.setTupleId(type, std::move(id)); }
  template<typename D> D setTupleId(Spacelike<D> &&spacelike, isl_dim_type type, const Id &id) { spacelike.setTupleId_inplace(type, id.copy()); return std::move(spacelike); }
  template<typename D> D setTupleId(const Spacelike<D> &spacelike, isl_dim_type type, const Id &id) { return spacelike.setTupleId(type, id.copy()); }

  template<typename D> D setDimName(Spacelike<D> &&spacelike, isl_dim_type type, pos_t pos, const char *s) { spacelike.setDimName_inplace(type, pos, s); return std::move(spacelike); }
  template<typename D> D setDimName(const Spacelike<D> &spacelike, isl_dim_type type, pos_t pos, const char *s) { return spacelike.setDimName(type, pos, s); }

  template<typename D> D setDimId(Spacelike<D> &&spacelike, isl_dim_type type, pos_t pos, Id &&id) { spacelike.setDimId_inplace(type, pos, std::move(id)); return std::move(spacelike); }
  template<typename D> D setDimId(const Spacelike<D> &spacelike, isl_dim_type type, pos_t pos, Id &&id) { return spacelike.setDimId(type, pos, std::move(id)); }
  template<typename D> D setDimId(Spacelike<D> &&spacelike, isl_dim_type type, pos_t pos, const Id &id) { spacelike.setDimId_inplace(type, pos, id.copy()); return std::move(spacelike); }
  template<typename D> D setDimId(const Spacelike<D> &spacelike, isl_dim_type type, pos_t pos, const Id &id) { return spacelike.setDimId(type, pos, id.copy()); }

  template<typename D> D addDims(Spacelike<D> &&spacelike, isl_dim_type type, count_t count) { spacelike.addDims_inplace(type, count); return std::move(spacelike); }
  template<typename D> D addDims(const Spacelike<D> &spacelike, isl_dim_type type, count_t count) { return spacelike.addDims(type, count); }

  template<typename D> D insertDims(Spacelike<D> &&spacelike, isl_dim_type type, pos_t pos, count_t count) { spacelike.insertDims_inplace(type, pos, count); return std::move(spacelike); }
  template<typename D> D insertDims(const Spacelike<D> &spacelike, isl_dim_type type, pos_t pos, count_t count) { return spacelike.insertDims(type, pos, count); }

  template<typename D> D moveDims(Spacelike<D> &&spacelike, isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) { spacelike.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, count); return std::move(spacelike); }
  template<typename D> D moveDims(const Spacelike<D> &spacelike, isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) { return spacelike.moveDims(dst_type, dst_pos, src_type, src_pos, count); }

  template<typename D> D removeDims(Spacelike<D> &&spacelike, isl_dim_type type, pos_t first, count_t count) { spacelike.removeDims_inplace(type, first, count); return std::move(spacelike); }
  template<typename D> D removeDims(const Spacelike<D> &spacelike, isl_dim_type type, pos_t first, count_t count) { return spacelike.removeDims(type, first, count); }

} // namepspace isl
#endif /* ISLPP_SPACELIKE_H */
