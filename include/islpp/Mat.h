#ifndef ISLPP_MAT_H
#define ISLPP_MAT_H

#include <cassert>

struct isl_mat;

namespace llvm {
} // namespace llvm

namespace isl {

} // namespace isl


namespace isl {
  class Mat {
#pragma region Low-level
  private:
    isl_mat *mat;

  public: // Public because otherwise we had to add a lot of friends
    isl_mat *take() { assert(mat); isl_mat *result = mat; mat = nullptr; return result; }
    isl_mat *takeCopy() const;
    isl_mat *keep() const { return mat; }
  protected:
    void give(isl_mat *mat);

    explicit Mat(isl_mat *mat) : mat(mat) { }
  public:
    static Mat enwrap(isl_mat *mat) { return Mat(mat); }
#pragma endregion

  public:
    Mat(void) : mat(nullptr) {}
    /* implicit */ Mat(const Mat &that) : mat(that.takeCopy()) {}
    /* implicit */ Mat(Mat &&that) : mat(that.take()) { }
    ~Mat(void);

    const Mat &operator=(const Mat &that) { give(that.takeCopy()); return *this; }
    const Mat &operator=(Mat &&that) { give(that.take()); return *this; }

  }; // class Mat

  static inline Mat enwrap(__isl_take isl_mat *mat) { return Mat::enwrap(mat); }

} // namespace isl
#endif /* ISLPP_MAT_H */
