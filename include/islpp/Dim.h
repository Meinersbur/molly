#ifndef ISLPP_DIM_H
#define ISLPP_DIM_H

struct isl_space;
struct isl_local_space;
enum isl_dim_type;


namespace isl {
  class Dim {
    isl_space *ownerSpace;
    isl_local_space *ownerLocalSpace;

    isl_dim_type type;
    int pos;

  protected:
    explicit Dim(isl_space *ownerSpace, isl_local_space *ownerLocalSpace, isl_dim_type type, int pos) : ownerSpace(ownerSpace), ownerLocalSpace(ownerLocalSpace), type(type), pos(pos) {}

  public:
    static Dim wrap(isl_space *ownerSpace, isl_dim_type type, int pos) { return Dim(ownerSpace, NULL, type, pos); }
    static Dim wrap(isl_local_space *ownerSpace, isl_dim_type type, int pos) { return Dim(NULL, ownerSpace, type, pos); }

  }; // class Dim


  class LocalSpaceDim : public Dim {
  };

  class SpaceDim : public Dim {
  };

} // namespace isl
#endif