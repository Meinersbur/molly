
#define ISLPP_STRUCT NAMEUS(isl,multi,ISLPP_EL)

  public:
    typedef Multi<ISLPP_ELPP> MultiType;

#pragma region Low-level
  private:
    ISLPP_STRUCT *el;

  public: // Public because otherwise we had to add a lot of friends
    ISLPP_STRUCT *take() { assert(el); ISLPP_STRUCT *result = el; el = nullptr; return result; }
    ISLPP_STRUCT *takeCopy() const;
    ISLPP_STRUCT *keep() const { return el; }
  protected:
    void give(ISLPP_STRUCT *el);

    explicit Multi(ISLPP_STRUCT *el) : el(el) { }
  public:
    static MultiType wrap(ISLPP_STRUCT *el) { return MultiType(el); }
#pragma endregion

  public:
    Multi() : el(nullptr) {}
    Multi(const MultiType &that) : el(that.takeCopy()) {}
    Multi(MultiType &&that) : el(that.take()) {}
    ~Multi();

    const MultiType &operator=(const MultiType &that) { give(that.takeCopy()); return *this; }
    const MultiType &operator=(MultiType &&that) { give(that.take()); return *this; }

    MultiType copy() const { return wrap(takeCopy()); }

    Ctx *getCtx() const { return Ctx::wrap(NAMEUS(isl_multi,ISLPP_EL,get_ctx)(keep())); }
    Space getSpace() const { return Space::wrap(NAMEUS(isl_multi,ISLPP_EL,get_space)(keep())); }
    Space getDomainSpace() const { return Space::wrap(NAMEUS(isl_multi,ISLPP_EL,get_domain_space)(keep())); }

#pragma region Creational
    static MultiType CONCAT(from,ISLPP_ELPP)(ISLPP_ELPP &&el);
    static MultiType createZero(Space &&space) { return wrap(NAMEUS(isl_multi,ISLPP_EL,zero)(space.take())); }
    static MultiType createIdentity(Space &&space) { return wrap(NAMEUS(isl_multi,ISLPP_EL,identity)(space.take())); }
#pragma endregion


#pragma region Dimensions
    unsigned dim(enum isl_dim_type type) const { return NAMEUS(isl_multi,ISLPP_EL,dim)(keep(), type); }

    //bool hasTupleName(isl_dim_type type) const { return isl_multi_aff_has_tuple_name(keep(), type); } 
    const char *getTupleName(isl_dim_type type) const { return NAMEUS(isl_multi,ISLPP_EL,get_tuple_name)(keep(), type); }
    void setTupleName(isl_dim_type type, const char *s) { give(NAMEUS(isl_multi,ISLPP_EL,set_tuple_name)(take(), type, s)); }

    //bool hasTupleId(isl_dim_type type) const { return isl_multi_aff_has_tuple_id(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::wrap(isl_multi_aff_get_tuple_id(keep(), type)); }
    void setTupleId(isl_dim_type type, Id &&id) { give(NAMEUS(isl_multi,ISLPP_EL,set_tuple_id)(take(), type, id.take())); }

    //bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_name(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_get_dim_name(keep(), type, pos); }
    void setDimName(isl_dim_type type, unsigned pos, const char *s) { give(NAMEUS(isl_multi,ISLPP_EL,set_dim_name)(take(), type, pos, s)); }
    //int findDimByName(isl_dim_type type, const char *name) const { return isl_multi_aff_find_dim_by_name(keep(), type, name); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_id(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::wrap(isl_multi_aff_get_dim_id(keep(), type, pos)); }
    //void setDimId(isl_dim_type type, unsigned pos, Id &&id) { give(isl_multi_aff_set_dim_id(take(), type, pos, id.take())); }
    void setDimId(isl_dim_type type, unsigned pos, Id &&id) { llvm_unreachable("API function missing"); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    void addDims(isl_dim_type type, unsigned n) { give(NAMEUS(isl_multi,ISLPP_EL,add_dims)(take(), type, n)); }
    void insertDims(isl_dim_type type, unsigned pos, unsigned n) { give(NAMEUS(isl_multi,ISLPP_EL,insert_dims)(take(), type, pos, n)); }
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { give(isl_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n)); }
    void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { llvm_unreachable("API function missing"); }
    void dropDims(isl_dim_type type, unsigned first, unsigned n) { give(NAMEUS(isl_multi,ISLPP_EL,drop_dims)(take(), type, first, n)); }

    void removeDims(isl_dim_type type, unsigned first, unsigned n) { dropDims(type, first, n); }
#pragma endregion


#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    ISLPP_ELPP CONCAT(get,ISLPP_ELPP)(int pos) const { return ISLPP_ELPP::wrap(NAMEUS(isl_multi,ISLPP_EL,get,ISLPP_EL)(keep(), pos)); }
    void append(Aff &&aff);
#pragma endregion


    void scale(Int f) { give(NAMEUS(isl_multi,ISLPP_EL,scale)(take(), f.keep())); };
    void scaleVec(Vec &&v) { give(isl_multi_aff_scale_vec(take(), v.take())); }

    void alignParams(Space &&model) { give(isl_multi_aff_align_params(take(), model.take())); }
    void gistParams(Set &&context) { give(isl_multi_aff_gist_params(take(), context.take())); }
    void gist(Set &&context) { give(isl_multi_aff_gist(take(), context.take())); }
    void lift() { give(isl_multi_aff_lift(take(), nullptr)); }
    LocalSpace lift(LocalSpace &context) { 
      isl_local_space *ls = nullptr;
      give(isl_multi_aff_lift(take(), &ls));
      return LocalSpace::wrap(ls);
    }

#undef ISLPP_STRUCT
