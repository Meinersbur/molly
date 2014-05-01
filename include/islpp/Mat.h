#ifndef ISLPP_MAT_H
#define ISLPP_MAT_H

#include "islpp_common.h"
#include "Obj.h"
#include "Val.h"
#include "Int.h"

#include <isl/mat.h>
#include <isl/deprecated/mat_int.h>

#include <cassert>



namespace isl {

  class Mat : public Obj<Mat, isl_mat> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_mat_free(takeOrNull()); }
    StructTy *addref() const { return isl_mat_copy(keepOrNull()); }

  public:
    Mat() { }

    /* implicit */ Mat(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Mat(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_mat_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion

    ISLPP_PROJECTION_ATTRS int rows() ISLPP_PROJECTION_FUNCTION{ return checkInt(isl_mat_rows(keep())); }
    ISLPP_PROJECTION_ATTRS int cols() ISLPP_PROJECTION_FUNCTION{ return checkInt(isl_mat_cols(keep())); }

      void getElement(int row, int col, Int &val) {
        checkBool(isl_mat_get_element(keep(), row, col, val.change()));
        val.updated();
    }

    ISLPP_PROJECTION_ATTRS Int getElement(int row, int col) ISLPP_PROJECTION_FUNCTION{
      Int result;
      checkBool(isl_mat_get_element(keep(), row, col, result.change()));
      result.updated();
      return result;
    }

      ISLPP_INPLACE_ATTRS void setElement_inplace(int row, int col, const Int &val)ISLPP_INPLACE_FUNCTION{
      give(isl_mat_set_element(take(), row, col, val.keep()));
    }

      ISLPP_PROJECTION_ATTRS Val getElementVal(int row, int col) ISLPP_PROJECTION_FUNCTION{
      return Val::enwrap(isl_mat_get_element_val(keep(), row, col));
    }

    class MatLvl2Subscript {
      Mat &parent;
      int row;
      int col;

    private:
      MatLvl2Subscript() LLVM_DELETED_FUNCTION;

    public:
      MatLvl2Subscript(Mat &parent, int row, int col) : parent(parent), row(row), col(col) {}

      operator Int() const { return parent.getElement(row, col); }
      void operator=(const Int &arg) const { parent.setElement_inplace(row, col, std::move(arg)); }

      bool isZero() const { return parent.getElement(row, col).isZero(); }
      bool isOne() const { return parent.getElement(row, col).isOne(); }
    }; // class MatLvl2Subscript

    class MatLvl1Subscript {
      Mat &parent;
      int row;

    private:
      MatLvl1Subscript() LLVM_DELETED_FUNCTION;

    public:
      MatLvl1Subscript(Mat &parent, int row) : parent(parent), row(row) {}
      MatLvl2Subscript   operator[](int col) const { return MatLvl2Subscript(parent, row, col); }
    }; // class MatLvl1Subscript


    class ConstMatLvl1Subscript {
      const Mat &parent;
      int row;

    private:
      ConstMatLvl1Subscript() LLVM_DELETED_FUNCTION;

    public:
      ConstMatLvl1Subscript(const Mat &parent, int row) : parent(parent), row(row) {}
      Int operator[](int col) const { return parent.getElement(row,col); }
    }; // class ConstMatLvl1Subscript

    MatLvl1Subscript operator[](int row) { return MatLvl1Subscript(*this, row); }
    ConstMatLvl1Subscript operator[](int row) const { return ConstMatLvl1Subscript(*this, row); }
  }; // class Mat

  static inline Mat enwrap(__isl_take isl_mat *mat) { return Mat::enwrap(mat); }
  static inline Mat enwrapCopy(__isl_keep isl_mat *mat) { return Mat::enwrapCopy(mat); }

} // namespace isl
#endif /* ISLPP_MAT_H */
