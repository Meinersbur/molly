#ifndef ISLPP_VEC_H
#define ISLPP_VEC_H

#include <isl/vec.h>


namespace isl {

  class Vec {
#pragma region Low-level
  private:
    isl_vec *vec;

  public: // Public because otherwise we had to add a lot of friends
    isl_vec *take() { assert(vec); isl_vec *result = vec; vec = nullptr; return result; }
    isl_vec *takeCopy() const { assert(vec); isl_vec_copy(vec); }
    isl_vec *keep() const { assert(vec); return vec; }
  protected:
    void give(isl_vec *vec) { assert(vec); if (this->vec) isl_vec_free(this->vec); this->vec = vec; }

  public:
    static Vec wrap(isl_vec *vec) { assert(vec); Vec result; result.give(vec); return result; }
#pragma endregion

  public:
    ~Vec() {  
    if (this->vec)
      isl_vec_free(this->vec);
#ifndef NDEBUG
    //TODO: Mark as unitialized under Valgrind and MemorySanitizer
    this->vec = nullptr;
#endif
    }

    Vec() : vec(nullptr) {}
    /* implicit */ Vec(const Vec &that) : vec(that.takeCopy()) {}
    /* implicit */ Vec(Vec &&that) : vec(that.take()) {}

    const Vec &operator=(const Vec &that) { give(that.takeCopy()); return *this; }
    const Vec &operator=(Vec &&that) { give(that.take()); return *this; }
  }; // class Vec
} // namespace isl
#endif /* ISLPP_VEC_H */
